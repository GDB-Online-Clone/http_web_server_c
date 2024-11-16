#include "http.h"
#include "utility.h"

struct http_header* insert_header(char *key, char *value);


void destruct_http_headers(struct http_headers *headers) {
    for (int i = 0; i < headers->size; i++) {
        struct http_header *parsed_header = headers->headers[i];
        free(parsed_header->key);
        free(parsed_header->value);
        free(parsed_header);
    }        
    free(headers->headers);
    headers->capacity = 0;
    headers->size = 0;
}



/**
 * @brief Build Test 용
 */
int main() {
    printf("Hello World\n");
    return 0;
}