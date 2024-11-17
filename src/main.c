#include "http.h"

/**
 * @brief Parse the HTTP method string and return its enum representation.
 */
enum http_method parse_http_method(const char *method) {
    if (strcmp(method, "GET") == 0) return HTTP_GET;
    if (strcmp(method, "POST") == 0) return HTTP_POST;
    return HTTP_METHOD_UNKNOWN;
}

/**
 * @brief Parse the HTTP version string and return its enum representation.
 */
enum http_version parse_http_version(const char *version) {
    if (strcmp(version, "HTTP/1.0") == 0) return HTTP_1_0;
    if (strcmp(version, "HTTP/1.1") == 0) return HTTP_1_1;
    if (strcmp(version, "HTTP/2.0") == 0) return HTTP_2_0;
    if (strcmp(version, "HTTP/3.0") == 0) return HTTP_3_0;
    return HTTP_VERSION_UNKNOWN;
}

int init_http_request(
    struct http_request             *request,
    struct http_headers             headers,
    enum http_method                method,
    enum http_version               version,
    char                            *body,
    char                            *path,
    struct http_query_parameters    query_parameters
) {
    // 요청 구조체가 NULL이면 실패
    if (request == NULL) {
        return -1;
    }

    request->headers = headers;
    request->method = method;
    request->version = version;
    request->body = body ? strdup(body) : NULL;
    request->path = path ? strdup(path) : NULL;
    request->query_parameters = query_parameters;

    return 0; // 성공
}

/**
 * @brief Parse an HTTP request string into a struct http_request.
 */
struct http_request parse_http_request(char *request) {    
    struct http_request http_request;    
    struct http_headers http_headers = {};
    struct http_query_parameters http_query_parameters = {};

    memset(&http_request, 0, sizeof(struct http_request));

    char *request_buffer = strdup(request);
    char *request_line = strtok(request_buffer, "\r\n");

    // 요청 라인을 파싱 (예: "GET /path HTTP/1.1")
    char *method = strtok(request_line, " ");     
    char *path_with_query = strtok(NULL, " ");        
    char *version = strtok(NULL, " ");

    enum http_method parsed_method = parse_http_method(method);
    enum http_version parsed_version = parse_http_version(method);

    // 경로와 쿼리 파라미터 분리
    char *query = NULL;
    char *query_start = strchr(path_with_query, '?');

    if (query_start) {
        *query_start = '\0'; // '?'를 '\0'으로 변경하여 경로와 쿼리 분리
        query = strdup(query_start + 1);
    }

    char *path = strdup(path_with_query);

    // 헤더와 본문 파싱
    char *header = NULL;
    char *body = NULL;

    char *header_start = strstr(request, "\r\n");

    if (header_start) {
        header_start += 2; // "\r\n" 건너뛰기
        char *body_start = strstr(header_start, "\r\n\r\n");

        if (body_start) {
            size_t header_len = body_start - header_start;
            header = malloc(header_len + 1);

            if (header) {
                memcpy(header, header_start, header_len); // 헤더 복사
                header[header_len] = '\0';
            }

            body_start += 4;
            body = strdup(body_start); // 내용 복사
        } else {
    
            header = strdup(header_start);
        }
    }

    // Http Request 초기화
    init_http_request(
        &http_request, 
        http_headers, 
        parsed_method,
        parsed_version, 
        body, 
        path, 
        http_query_parameters
    );

    // 동적 메모리 해제
    free(request_buffer);
    free(query);
    free(header);
    free(path);
    free(body);

    return http_request;
}

/**
 * @brief Build Test 용
 */
int main() {
    printf("Hello World\n");
    return 0;
}