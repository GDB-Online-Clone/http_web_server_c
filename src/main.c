#include "http.h"
#include "utility.h"

struct http_headers* insert_header(struct http_headers *headers, char *key, char *value) {
    if (headers->capacity == headers->size) {
        int new_capacity = headers->capacity * 2;

        struct http_header** new_headers = realloc(headers->headers, new_capacity * sizeof(struct http_header*));

        headers->headers = new_headers;
        headers->capacity = new_capacity;            
    }

    struct http_header *header = (struct http_header *)malloc(sizeof(struct http_header));
    if (header == NULL)
        return NULL;

    char *new_key = strdup(key);
    char *new_value = strdup(value);

    if (!new_key || !new_value) {
        if (!new_key)
            free(new_key);
        if (!new_value)
            free(new_value);
        free(header);
        return NULL;
    }

    *header = (struct http_header) {
        .key = new_key,
        .value = new_value
    };
    headers->headers[headers->size++] = header;
    
    return headers;
}


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

struct http_header *parse_http_header(struct http_header *http_header, char *header_string) {

    char *end_of_header = strstr(header_string, "\r\n");
    int offset = 0;
    struct http_header *header;
    char *start_of_key;
    char *start_of_value;

    if (!http_header) {
        header = (struct http_header*)malloc(sizeof(struct http_header));
    } else {
        header = http_header;
    }
    header->key = NULL;
    header->value = NULL;

    /* trim prefix */
    start_of_key = find_non_space(header_string);  

    offset = (int)(start_of_key - header_string);

    /* if not quoted string */
    if (header_string[offset] != '\"') {
        char *end_of_key = start_of_key;
        int length;

        while (end_of_key != end_of_header && *end_of_key != ':')
            end_of_key++;
        if (end_of_key == end_of_header)
            goto parse_header_error;

        offset = (int)(end_of_key - start_of_key) + 1;

        end_of_key--;
        /* trim suffix */
        while (is_non_space(*end_of_key)) 
            end_of_key--;

        length = (int)(end_of_key - start_of_key) + 1;
        
        header->key = (char*)malloc(length + 1);
        strncpy(header->key, start_of_key, length);

    } else {
        /* Case of quoted string. now we cannnot perform like above, because ':' can be inside of key name. */
        /* At now, our target is to find ` ": `, double quotes followed by ':'. */
        
        char *end_of_key = start_of_key + 1;
        int length;
        /* find first non-whitespace except for double quotes at start */
        char *last_non_space_at = find_non_space(start_of_key + 1);
        

        end_of_key = last_non_space_at + 1;

        for (; end_of_key != end_of_header && (*end_of_key != ':' || *last_non_space_at != '"'); end_of_key++) {
            if (!is_non_space(*end_of_key))
                last_non_space_at = end_of_key;
        }
        
        if (end_of_key == end_of_header) {
            goto parse_header_error;
        }

        offset = (int)(end_of_key - start_of_key) + 1;        
        end_of_key = last_non_space_at - 1;
        start_of_key++;

        length = (int)(end_of_key - start_of_key) + 1;
        
        header->key = (char*)malloc(length + 1);
        strncpy(header->key, start_of_key, length);
    }
    

    /* step to parse value */

    /* trim prefix */
    start_of_value = find_non_space(header_string + offset);  

    offset = (int)(start_of_value - header_string);

    /* if not quoted string */
    if (header_string[offset] != '\"') {
        char *end_of_value = start_of_value;
        int length;

        while (end_of_value != end_of_header)
            end_of_value++;

        offset = (int)(end_of_value - start_of_value) + 1;

        end_of_value--;
        /* trim suffix */
        while (is_non_space(*end_of_value)) 
            end_of_value--;

        length = (int)(end_of_value - start_of_value) + 1;
        
        header->value = (char*)malloc(length + 1);
        strncpy(header->value, start_of_value, length);

    } else {
        /* Case of quoted string. now we cannnot perform like above, because ':' can be inside of value string. */
        /* At now, our target is to find ` "<CRLF> `, double quotes followed by "\r\n". */
        
        char *end_of_value = start_of_value + 1;
        int length;
        /* find first non-whitespace except for double quotes at start */
        char *last_non_space_at = find_non_space(start_of_value + 1);
        

        end_of_value = last_non_space_at + 1;

        for (; end_of_value != end_of_header; end_of_value++) {
            if (!is_non_space(*end_of_value))
                last_non_space_at = end_of_value;
        }
        
        if (*last_non_space_at != '"') {
            goto parse_header_error;
        }

        offset = (int)(end_of_value - start_of_value) + 1;
        end_of_value = last_non_space_at - 1;
        start_of_value++;

        length = (int)(end_of_value - start_of_value) + 1;
        
        header->value = (char*)malloc(length + 1);
        strncpy(header->value, start_of_value, length);
    }
 
    return header;

parse_header_error:
    if (header->key)
        free(header->key);

    if (http_header == NULL) {
        free(header);
    }
    return NULL;
}

struct http_headers parse_http_headers(char *headers_string) {    

    const int INITIAL_CAPACITTY = 8;

    struct http_headers headers_ret = {
        .size = 0,
        .capacity = INITIAL_CAPACITTY,
        .headers = malloc(INITIAL_CAPACITTY * sizeof(struct http_header))
    };
    
    int offset = 0;
    while (headers_string[offset] != '\0') {
        char *CRLF_pointer = strstr(headers_string + offset, "\r\n");

        
        if (CRLF_pointer == NULL) { /* parse failed */
            destruct_http_headers(&headers_ret);
            break;
        } else if (CRLF_pointer == (headers_string + offset)) { /* (normal break condition) there is no more header to parse */
            break;
        }
        

        struct http_header *parsed_header = parse_http_header(NULL, headers_string + offset);
        

        if (parsed_header == NULL) { /* parse failed */
            destruct_http_headers(&headers_ret);
            break;
        }

        /* insert parsed_header into http_headers::headers */

        if (headers_ret.capacity == headers_ret.size) {
            int new_capacity = headers_ret.capacity * 2;

            struct http_header** new_headers = realloc(headers_ret.headers, new_capacity * sizeof(struct http_header*));

            headers_ret.headers = new_headers;
            headers_ret.capacity = new_capacity;            
        }

        headers_ret.headers[headers_ret.size++] = parsed_header;
        offset = (int)(CRLF_pointer - headers_string) + 2;
    }
        
    return headers_ret;
}



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