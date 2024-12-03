#include <stdbool.h>

#include "http.h"
#include "utility.h"
#include "threadpool.h"
#include "runner.h"

static const size_t            KB      = 1024;
static const int               N_KB    = 16;

static struct sockaddr_in      addr;
    
static int                     server_fd;
static int                     addrlen = sizeof(addr);


static struct http_response    response_500;
static struct http_response    response_404;
static struct http_response    response_204;

static struct routes           *route_table;

static struct threadpool       *pool;

static char                    **buffer_list;
static atomic_bool             *buffer_used;


static void handle_http_request(void* arg) {

    struct route            *found_route;
    struct http_response    *response = NULL;
    struct http_request     *request = NULL;
    char                    *response_str;
    int                     client_socket = (int)arg;
    ssize_t                 bytes_read;
    ssize_t                 total_read;
    char                    *buffer = NULL;
    int                     buffer_idx;

    for (int i = 0; i < pool->max_threads; i++) {
        bool expected = false;
        if (atomic_compare_exchange_strong(&buffer_used[i], &expected, true)) {
            buffer = buffer_list[i];
            buffer_idx = i;
            break;
        }
    }

    if (buffer == NULL) {
        DLOGV("UNEXPECTED\n");
        response = &response_500;
        goto label_send_response;
    }

    // @TODO need to handle long message    
    total_read = 0;
    bytes_read = read(client_socket, buffer, (size_t)KB * N_KB - 1);
    total_read += bytes_read;

    if (total_read < 0) {
        perror("read");
        close(client_socket);
        free(buffer);
        // @TODO server log print
        DLOGV("Socket read error: %s", strerror(errno));
        response = &response_500;
        goto label_send_response;
    }
    
    if (total_read == 0) {
        perror("read");
        close(client_socket);
        // TODO(server log print)
        DLOGV("Client disconnected - socket=%d", client_socket); 
        response = &response_500;
        goto label_send_response;
    } 
    
    // @TODO parse_http_request 반환 값을 포인터로 변경
    request = parse_http_request(buffer);

    if (request == NULL) {
        response = &response_500;
        goto label_send_response;
    }

    if (request->method == HTTP_OPTIONS) {
        response = &response_204;
        goto label_send_response;
    }

    // 라우트 찾기
    found_route = find_route(route_table, request->path, request->method);

    if (found_route == NULL) {
        response = &response_404;
        goto label_send_response;
    } 
    
    response = found_route->callback(*request);
    
    if (response == NULL) {
        response = &response_500;
        goto label_send_response;
    }

    label_send_response:
    response_str = http_response_stringify(*response);

    /* response 가 null 일 경우는 없다고 가정 */
    if (response != &response_404 && response != &response_500 && response != &response_204) {
        if (response->body)
            free(response->body);

        if (response->headers.capacity)
            destruct_http_headers(&response->headers);

        free(response);
    }

    // 응답 전송
    ssize_t total_written = 0;
    size_t response_len = strlen(response_str);

    while (total_written < response_len) {
        ssize_t written = write(client_socket,
                                response_str + total_written,
                                response_len - total_written);
        if (written <= 0) {
            if (errno == EINTR)
                continue;
            break;
        }
        total_written += written;
    }

    // 응답이 완전히 전송되도록 보장
    shutdown(client_socket, SHUT_WR);

    // 메모리 정리
    free(response_str);

    if (request) {
        destruct_http_request(request);
        free(request);
    }

    close(client_socket);

    atomic_store(&buffer_used[buffer_idx], false);
}

void cleanup(void) {
    close(server_fd);
    threadpool_destroy(pool);
}

int run_web_server(struct web_server server) {    
    response_500 = (struct http_response) {
        .body = NULL,
        .headers = (struct http_headers) {
            .capacity = 8,
            .items = malloc(8 * sizeof(struct http_header*)),
            .size = 0
        },
        .http_version = HTTP_1_1,
        .status_code = HTTP_INTERNAL_SERVER_ERROR
    };

    insert_header(&response_500.headers, "Access-Control-Allow-Origin", "*");
    insert_header(&response_500.headers, "Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    insert_header(&response_500.headers, "Access-Control-Allow-Headers", "*");

    response_404 = (struct http_response) {
        .body = NULL,
        .headers = (struct http_headers) {
            .capacity = 8,
            .items = malloc(8 * sizeof(struct http_header*)),
            .size = 0
        },
        .http_version = HTTP_1_1,
        .status_code = HTTP_NOT_FOUND
    };

    insert_header(&response_404.headers, "Access-Control-Allow-Origin", "*");
    insert_header(&response_404.headers, "Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    insert_header(&response_404.headers, "Access-Control-Allow-Headers", "*");

    response_204 = (struct http_response) {
        .body = NULL,
        .headers = (struct http_headers) {
            .capacity = 8,
            .items = malloc(8 * sizeof(struct http_header*)),
            .size = 0
        },
        .http_version = HTTP_1_1,
        .status_code = HTTP_NO_CONTENT
    };

    insert_header(&response_204.headers, "Access-Control-Allow-Origin", "*");
    insert_header(&response_204.headers, "Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    insert_header(&response_204.headers, "Access-Control-Allow-Headers", "*");


    // 소켓 생성
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket"); 
        return errno;
    }

    // 소켓 재사용 옵션 설정
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        DLOGV("setsockopt failed: %s", strerror(errno)); 
        close(server_fd);
        return errno;
    }

    // 소켓 설정
    addr.sin_family = AF_INET;  // IPv4
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(server.port_num);

    // 소켓에 주소 할당
    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(server_fd);
        return errno;
    }

    // 연결 대기 시작
    if (listen(server_fd, server.backlog) < 0) {
        perror("listen");
        close(server_fd);
        return errno;
    }

    route_table = server.route_table;

    pool = threadpool_create(server.threadpool_size);
    
    buffer_list = malloc(pool->max_threads * sizeof(char *));
    for (int i = 0; i < pool->max_threads; i++) {
        buffer_list[i] = malloc(N_KB * KB);
    }
    buffer_used = malloc(pool->max_threads * sizeof(atomic_bool));
    for (int i = 0 ; i < pool->max_threads; i++) {
        buffer_used[i] = false;
    }

    DLOGV("[Server] port: %d, backlog: %d\n", server.port_num, server.backlog);  

    while (1) {
        int client_socket;
        if ((client_socket = accept(server_fd, (struct sockaddr *)&addr, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            // @TODO server log print
            DLOGV("Failed to accept client socket: %s", strerror(errno));
            continue;
        }
        threadpool_add_job(pool, handle_http_request, (void*)client_socket);
    }

    return 0;
}