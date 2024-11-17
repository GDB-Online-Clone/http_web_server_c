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
 * @brief Build Test 용
 */
int main() {
    printf("Hello World\n");
    return 0;
}