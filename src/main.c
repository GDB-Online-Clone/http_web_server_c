#include "http.h"

/**
 * @brief Parses a single HTTP query parameter.
 * 
 * This function takes a key and a value as input and returns a 
 * struct http_query_parameter containing the parsedParam key and value.
 * 
 * @param key The key of the query parameter.
 * @param value The value of the query parameter.
 * @return struct http_query_parameter The parsedParam query parameter.
 */
struct http_query_parameter parse_http_query_parameter(char *key, char *value){

    struct http_query_parameter query_parameter;
    
    query_parameter.key = key;
    query_parameter.value = value;
    
    return query_parameter;
}

/**
 * @brief Allocates memory and stores a single HTTP query parameter.
 * 
 * This function parses a key and value into a struct http_query_parameter,
 * allocates memory for it, and returns a pointer to the allocated memory.
 * 
 * @param key The key of the query parameter.
 * @param value The value of the query parameter.
 * @return struct http_query_parameter* Pointer to the allocated query parameter.
 */
struct http_query_parameter* insert_query_parameter(char* key, char* value){

    struct http_query_parameter parsedParam = parse_http_query_parameter(key, value);

    struct http_query_parameter* query_parameter = 
        (struct http_query_parameter*)malloc(sizeof(struct http_query_parameter));
    
    if (!query_parameter) {
        return NULL;
    }

    query_parameter->key = strdup(parsedParam.key);
    query_parameter->value = strdup(parsedParam.value);
    
    return query_parameter;
}


/**
 * @brief Parses an entire query string into multiple query parameters.
 * 
 * This function takes a query string, parses it into individual key-value pairs,
 * and stores them in a struct http_query_parameters. It allocates memory for 
 * each query parameter and returns the struct containing all parameters.
 * 
 * @param parameters_string The query string to be parsedParam.
 * @return struct http_query_parameters The parsedParam query parameters.
 */
struct http_query_parameters parse_query_parameters(char* parameters_string){
    
    struct http_query_parameters query_parameters;
    query_parameters.size = 0;
    query_parameters.parameters = 
        (struct http_query_parameter**)malloc(sizeof(struct http_query_parameter*) * 10);
    
    /* '&' 구분자 처리 */
    char *save_ptr1;
    /* '=' 구분자 처리 */
    char *save_ptr2;

    char* token = strtok_r(parameters_string, "&",&save_ptr1);
    
    while(token != NULL){
        char* key = strtok_r(token, "=", &save_ptr2);
        char* value = strtok_r(NULL, "=", &save_ptr2);

        struct http_query_parameter* query_parameter = 
            insert_query_parameter(key, value);
        
        query_parameters.parameters[query_parameters.size++] = query_parameter;
        token = strtok_r(NULL, "&", &save_ptr1);
    }
    return query_parameters;
}

/**
 * @brief Frees memory allocated for query parameters.
 * 
 * This function frees the memory allocated for each query parameter's key and value,
 * as well as the memory allocated for the struct http_query_parameter itself. It also
 * frees the memory allocated for the array of query parameters and resets the size.
 * 
 * @param query_parameters Pointer to the struct http_query_parameters to be freed.
 */
void free_query_parameters(struct http_query_parameters* query_parameters){
    
    if (!query_parameters) {
        return;
    }

    if (query_parameters->parameters){
        for (int i = 0; i < query_parameters->size; i++){
            if (query_parameters->parameters[i]) {

                // key와 value 메모리 해제
                free(query_parameters->parameters[i]->key);
                free(query_parameters->parameters[i]->value);
                
                // parameter 구조체 자체 해제
                free(query_parameters->parameters[i]);
            }
        }

        free(query_parameters->parameters);
        query_parameters->parameters = NULL;
    }
    
    query_parameters->size = 0;
}

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
 * 
 * @return int 프로그램 종료 상태
 */
int main() {
    printf("Hello World\n");
    return 0;
}