#include "http.h"
#include "utility.h"


struct route *find_route(const struct routes* routes, const char *path, enum http_method method) {
    for (int i = 0; i < routes->size; i++) {
        if (routes->items[i]->method == method && url_path_cmp(path, routes->items[i]->path) == 0)
            return routes->items[i];
    }
    return NULL;
}

struct routes* init_routes(struct routes *route_table) {
    const int INITIAL_CAPACITY = 8;

    *route_table = (struct routes) {
        .capacity = INITIAL_CAPACITY,
        .items = (struct route **)malloc(INITIAL_CAPACITY * sizeof(struct route *)),
        .size = 0
    };

    if (route_table->items == NULL) {
        route_table->capacity = 0;
        return NULL;
    }
    return route_table;
}

struct http_headers* insert_header(struct http_headers *headers, char *key, char *value) {
    if (headers->capacity == headers->size) {
        int new_capacity = headers->capacity * 2;

        struct http_header** new_headers = realloc(headers->items, new_capacity * sizeof(struct http_header*));

        headers->items = new_headers;
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
    headers->items[headers->size++] = header;
    
    return headers;
}

void destruct_http_headers(struct http_headers *headers) {
    for (int i = 0; i < headers->size; i++) {
        struct http_header *parsed_header = headers->items[i];
        free(parsed_header->key);
        free(parsed_header->value);
        free(parsed_header);
    }        
    free(headers->items);
    headers->capacity = 0;
    headers->size = 0;
}

struct http_header *parse_http_header(char *header_string) {

    char *end_of_header = strstr(header_string, "\r\n");
    int offset = 0;
    struct http_header *header = (struct http_header*)malloc(sizeof(struct http_header));
    char *start_of_key;
    char *start_of_value;

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
    if (header->value)
        free(header->value);
    free(header);

    return NULL;
}

struct http_headers parse_http_headers(char *headers_string) {    

    const int INITIAL_CAPACITTY = 8;

    struct http_headers headers_ret = {
        .size = 0,
        .capacity = INITIAL_CAPACITTY,
        .items = malloc(INITIAL_CAPACITTY * sizeof(struct http_header))
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
        

        struct http_header *parsed_header = parse_http_header(headers_string + offset);
        

        if (parsed_header == NULL) { /* parse failed */
            destruct_http_headers(&headers_ret);
            break;
        }

        /* insert parsed_header into http_headers::headers */

        if (headers_ret.capacity == headers_ret.size) {
            int new_capacity = headers_ret.capacity * 2;

            struct http_header** new_headers = realloc(headers_ret.items, new_capacity * sizeof(struct http_header*));

            headers_ret.items = new_headers;
            headers_ret.capacity = new_capacity;            
        }

        headers_ret.items[headers_ret.size++] = parsed_header;
        offset = (int)(CRLF_pointer - headers_string) + 2;
    }
        
    return headers_ret;
}


struct http_query_parameter parse_http_query_parameter(char* parameter_string){

    struct http_query_parameter query_parameter = {};
    
    if (!parameter_string) {
        return query_parameter;
    }

    char* param_string_copy = strdup(parameter_string);
    if (!param_string_copy) {
        return query_parameter;
    }

    /* '=' 구분자 처리 */
    char *save_ptr2;

    char* key = strtok_r(param_string_copy, "=", &save_ptr2);
    char* value = strtok_r(NULL, "=", &save_ptr2);

    if (key) {
        query_parameter.key = strdup(key);
        if (!query_parameter.key) {
            free(param_string_copy);
            return query_parameter; // 메모리 할당 실패
        }
    }
    if (value) {
        query_parameter.value = strdup(value);
        if (!query_parameter.value) {
            free(query_parameter.key);
            free(param_string_copy);
            return query_parameter; // 메모리 할당 실패
        }
    }

    free(param_string_copy);
    return query_parameter;
}



struct http_query_parameters* insert_query_parameter(struct http_query_parameters *query_parameters, char* parameter_string){

    char * param_string_copy = strdup(parameter_string);

    if (!query_parameters || !param_string_copy) {
        return NULL;
    }

    // 쿼리 파라미터가 10개 이상이면 실패
    if (query_parameters->size >= 10){  
        return NULL;
    }

    struct http_query_parameter parsed_param = parse_http_query_parameter(param_string_copy);

    if (!parsed_param.key || !parsed_param.value) {
        if (parsed_param.key) free(parsed_param.key);
        if (parsed_param.value) free(parsed_param.value);
        return NULL;
    }

    struct http_query_parameter* new_param = 
        (struct http_query_parameter*)malloc(sizeof(struct http_query_parameter));
    
    if (!new_param){
        free(parsed_param.key);
        free(parsed_param.value);
        return NULL;
    }

    new_param->key = parsed_param.key;
    new_param->value = parsed_param.value;
    
    query_parameters->items[query_parameters->size++] = new_param;
    
    return query_parameters;
}



struct http_query_parameters parse_query_parameters(char* parameters_string){
    char *parameters = strdup(parameters_string);

    struct http_query_parameters query_parameters;
    query_parameters.size = 0;
    query_parameters.items = 
        (struct http_query_parameter**)malloc(sizeof(struct http_query_parameter*) * 10);
    
    /* '&' 구분자 처리 */
    char *save_ptr;

    char* token = strtok_r(parameters, "&",&save_ptr);
    
    while(token != NULL){
        
        if (insert_query_parameter(&query_parameters, token) == NULL){
            // 오류 발생 시 이미 할당된 메모리 정리
            free_query_parameters(&query_parameters);
            return (struct http_query_parameters){0};
        }
        token = strtok_r(NULL, "&", &save_ptr);
    }

    free(parameters);
    return query_parameters;
}

void free_query_parameters(struct http_query_parameters* query_parameters) {
    
    if (!query_parameters) {
        return;
    }

    if (query_parameters->items){
        for (int i = 0; i < query_parameters->size; i++){
            if (query_parameters->items[i]) {

                if (query_parameters->items[i]->key) {
                    free(query_parameters->items[i]->key);
                }

                if (query_parameters->items[i]->value) {
                    free(query_parameters->items[i]->value);
                }
                free(query_parameters->items[i]);
            }
        }

        free(query_parameters->items);
        query_parameters->items = NULL;
    }
    
    query_parameters->size = 0;
}

enum http_method parse_http_method(const char *method) {
    if (strcmp(method, "GET") == 0) return HTTP_GET;
    if (strcmp(method, "POST") == 0) return HTTP_POST;
    return HTTP_METHOD_UNKNOWN;
}

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

struct http_request parse_http_request(const char *request) {
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
    enum http_version parsed_version = parse_http_version(version);

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
        
        size_t header_len = body_start - header_start + 2;
        header = malloc(header_len + 1);

        if (header) {
            memcpy(header, header_start, header_len); // 헤더 복사
            header[header_len] = '\0';
        }

        body_start += 4;
        body = strdup(body_start); // 내용 복사
    }

    http_headers = header != NULL
        ? parse_http_headers(header)
        : http_headers;

    DLOG("In request parser\n");
    for (int i = 0; i < http_headers.size; i++) {
        DLOG("%s: %s\n", http_headers.items[i]->key, http_headers.items[i]->value);        
    }

    http_query_parameters = query != NULL
        ? parse_query_parameters(query)
        : http_query_parameters;

    DLOG("DEBUG INFO:\n");
    DLOG("Parsed Method: %d\n", parsed_method);
    DLOG("Parsed Version: %d\n", parsed_version);
    DLOG("Path String: %s\n", path ? path : "(null)");
    DLOG("Query String: %s\n", query ? query : "(null)");
    DLOG("Header String: %s\n", header ? header : "(null)");
    DLOG("Body String: %s\n", body ? body : "(null)");
     DLOG("\n");

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