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
 * @brief Build Test 용
 */
int main() {
    printf("Hello World\n");
    return 0;
}