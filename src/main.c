#include "http.h"
#include "utility.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-function"

static struct http_response *hello_world(struct http_request request) {
    struct http_response *response = (struct http_response *)malloc(sizeof(struct http_response));
    
    response->http_version = HTTP_1_1;
    response->status_code = HTTP_OK;
    response->body = "Hello, World!";
    response->headers = (struct http_headers) {
        .size = 0,
        .capacity = 0,
        .items = NULL
    };

    return response;
}

static struct http_response *find_words(struct http_request request) {
    struct http_response *response = (struct http_response *)malloc(sizeof(struct http_response));
    char *found = request.body;
    int cnt = 0;
    while ((found = strstr(found, "aaba"))) {
        cnt++;
    }

    response->http_version = HTTP_1_1;
    response->status_code = HTTP_OK;
    char *buf = malloc(1024);
    sprintf(buf, "%d\n", cnt);
    response->body = buf;
    response->headers = (struct http_headers) {
        .size = 0,
        .capacity = 0,
        .items = NULL
    };

    return response;
}

static struct http_response *mirror(struct http_request request) {
    struct http_response *response = (struct http_response *)malloc(sizeof(struct http_response));
    char *found = request.body;
    
    response->http_version = HTTP_1_1;
    response->status_code = HTTP_OK;
    response->body = request.body;
    response->headers = (struct http_headers) {
        .size = 0,
        .capacity = 0,
        .items = NULL
    };

    return response;
}



/**
 * @brief Build Test 용
 * 
 * @return int 프로그램 종료 상태
 */
int main() {
    struct routes route_table = {};
    init_routes(&route_table);
    
    insert_route(&route_table, "/hello-world", HTTP_GET, hello_world);

    struct web_server app = (struct web_server) {
        .route_table = &route_table,
        .port_num = 10010,
        .backlog = 10
    };
  
    run_web_server(app);

    

    //   // HTTP 요청 파싱
    // char *http_request =
    //     "GET /search?q=example&lang=en HTTP/1.1\r\n"
    //     "Host: www.example.com\r\n"
    //     "User-Agent: TestClient/1.0\r\n"
    //     "Accept: text/html\r\n"
    //     "\r\n";    

    // struct http_request request = parse_http_request(http_request);
    
    
    // DLOG("[headers]\n");
    // for (int i = 0; i < request.headers.size; i++) {
    //     DLOG("%s: %s\n", request.headers.items[i]->key, request.headers.items[i]->value);        
    // }
    // DLOG("[method]\n%d\n", request.method);
    // DLOG("[version]\n%d\n", request.version);
    // DLOG("[body]\n%s\n", request.body);
    // DLOG("[path]\n%s\n", request.path);

    //  DLOG("[query parameters]\n");
    // for (int i = 0; i < request.query_parameters.size; i++) {
    //     DLOG("%s: %s\n", request.query_parameters.items[i]->key, request.query_parameters.items[i]->value);        
    // }
    return 0;
}

#pragma GCC diagnostic pop