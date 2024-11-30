#include "http.h"
#include "utility.h"

#define BUF_SIZE 1024 /* <- 임시 버퍼 크기*/

struct http_header* find_header(const struct http_headers *headers, const char *key) {
    for (int i = 0; i < headers->size; i++) {
        if (strcmp(headers->items[i]->key, key) == 0) 
            return headers->items[i];
    }
    return NULL;
}

struct route *find_route(const struct routes* routes, const char *path, enum http_method method) {
    for (int i = 0; i < routes->size; i++) {
        if (routes->items[i]->method == method && url_path_cmp(path, routes->items[i]->path) == 0)
            return routes->items[i];
    }
    return NULL;
}

struct routes* insert_route(
		struct routes       *route_table,
		const char          *path,
		enum http_method    method,
		struct http_response *(*callback)(struct http_request request)
) {
    if (path[0] != '/')
        return NULL;
    if (find_route(route_table, path, method))
        return NULL;

    if (route_table->capacity == route_table->size) {
        int new_capacity = route_table->capacity * 2;

        struct route** new_route_table = (struct route**)realloc(route_table->items, new_capacity * sizeof(struct route*));

        route_table->items = new_route_table;
        route_table->capacity = new_capacity;            
    }

    struct route *route = (struct route *)malloc(sizeof(struct route));
    if (route == NULL)
        return NULL;

    *route = (struct route) {
        .callback = callback,
        .method = method,
        .path = strdup(path)    
    };
    
    if (!route->path) {        
        free(route);        
        return NULL;
    }

    route_table->items[route_table->size++] = route;
    
    return route_table;
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

void destruct_http_request(struct http_request *request) {
    if (request->body)
        free(request->body);

    if (request->headers.capacity)
        destruct_http_headers(&request->headers);

    if (request->path)
        free(request->path);

    free_query_parameters(&request->query_parameters);    
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

char *http_headers_stringify(struct http_headers *headers) {
    // No headers to process
    if (headers == NULL || headers->size == 0 || headers->items == NULL) {
        return NULL;
    }

    size_t total_length = 0;
    for (int i = 0; i < headers->size; i++) {
        struct http_header *header = headers->items[i];

        // No header to process
        if (header == NULL || header->key == NULL || header->value == NULL) {
            continue;
        }

        // key + ": " + value + "\r\n"
        // 1. ": " (2 bytes) - separator between key and value
        // 2. "\r\n" (2 bytes) - line terminator for HTTP headers
        total_length += strlen(header->key) + strlen(header->value) + 4; 
    }

    char *result = (char *)calloc(total_length + 1, sizeof(char));
    if (result == NULL) {
        return NULL;
    }

    for (int i = 0; i < headers->size; i++) {
        struct http_header *header = headers->items[i];

        // Ensure the header and its fields are valid
        if (header == NULL || header->key == NULL || header->value == NULL) {
            continue;
        }

        // Safely concatenate strings
        strncat(result, header->key, strlen(header->key));
        strncat(result, ": ", 2);
        strncat(result, header->value, strlen(header->value));
        strncat(result, "\r\n", 2);
    }

    return result;
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

struct http_query_parameter* find_query_parameter(struct http_query_parameters* query_parameters, char* param_key) {
    if (!query_parameters || !param_key) {
        return NULL;
    }

    for (int i = 0; i < query_parameters->size; i++) {
        if ((query_parameters->items[i]->key != NULL) && (strcmp(query_parameters->items[i]->key, param_key) == 0)) {
            return query_parameters->items[i];
        }
    }

    return NULL;
}

int init_http_response(
    struct http_response    *response,
    enum http_status_code   status_code,
    struct http_headers     headers,
    enum http_version       version,
    char                    *body
) {
    // 응답 구조체가 NULL이면 실패
    if (response == NULL) {
        return -1;
    }

    response->status_code = status_code;
    response->headers = headers;
    response->http_version = version;
    response->body = body ? strdup(body) : NULL;

    return 0; // 성공
}

char* http_response_stringify(struct http_response http_response) {
    char *header_string = http_headers_stringify(&http_response.headers);

    const char *status_code_string = http_status_code_stringify(
        http_response.status_code
    );

    const char *version_string = http_version_stringify(
        http_response.http_version
    );

    const char *body = http_response.body ? http_response.body : "";

    //  최종 문자열의 길이를 계산합니다.
    int required_len = snprintf(
        NULL, 0, "%s %d %s\r\n%s\r\n%s",
        version_string,
        http_response.status_code,
        status_code_string,
        header_string != NULL ? header_string : "",
        body 
    );

    if (required_len < 0) {
        if (header_string != NULL) {
            free(header_string);
        }

        return NULL;
    }

    // 최종 문자열 생성
    char *response_string = (char *)malloc(required_len + 1);
    if (!response_string) {
         if (header_string != NULL) {
            free(header_string);
        }

        return NULL;
    }

    snprintf(response_string, required_len + 1, "%s %d %s\r\n%s\r\n%s",
             version_string,
             http_response.status_code,
             status_code_string,
             header_string != NULL ? header_string : "",
             body);

    if (header_string != NULL) {
        free(header_string);
    }

    return response_string;
}

enum http_method parse_http_method(const char *method) {
    if (strcmp(method, "GET") == 0) return HTTP_GET;
    if (strcmp(method, "POST") == 0) return HTTP_POST;
    if (strcmp(method, "PUT") == 0) return HTTP_PUT;
    if (strcmp(method, "DELETE") == 0) return HTTP_DELETE;
    if (strcmp(method, "OPTIONS") == 0) return HTTP_OPTIONS;
    return HTTP_METHOD_UNKNOWN;
}

char* http_method_stringify(const enum http_method method) {
    if (method == HTTP_GET) return "GET";
    if (method == HTTP_POST) return "POST";
    if (method == HTTP_PUT) return "PUT";
    if (method == HTTP_DELETE) return "DELETE";
    if (method == HTTP_OPTIONS) return "OPTIONS";
    return "UNKNOWN";
}

enum http_version parse_http_version(const char *version) {
    if (strcmp(version, "HTTP/1.0") == 0) return HTTP_1_0;
    if (strcmp(version, "HTTP/1.1") == 0) return HTTP_1_1;
    if (strcmp(version, "HTTP/2.0") == 0) return HTTP_2_0;
    if (strcmp(version, "HTTP/3.0") == 0) return HTTP_3_0;
    return HTTP_VERSION_UNKNOWN;
}

char* http_version_stringify(const enum http_version version) {
    if (version == HTTP_1_0) return "HTTP/1.0";
    if (version == HTTP_1_1) return "HTTP/1.1";
    if (version == HTTP_2_0) return "HTTP/2.0";
    if (version == HTTP_3_0) return "HTTP/3.0";
    return "Unknown";
}

char* http_status_code_stringify(const enum http_status_code code) {
    // 2xx Success
    if (code == HTTP_OK) return "OK";
    if (code == HTTP_CREATED) return "Created";
    if (code == HTTP_ACCEPTED) return "Accepted";
    if (code == HTTP_NO_CONTENT) return "No Content";

    // 3xx Redirection
    if (code == HTTP_MOVED_PERMANENTLY) return "Moved Permanently";
    if (code == HTTP_FOUND) return "Found";
    if (code == HTTP_NOT_MODIFIED) return "Not Modified";
    if (code == HTTP_TEMPORARY_REDIRECT) return "Temporary Redirect";
    if (code == HTTP_PERMANENT_REDIRECT) return "Permanent Redirect";

    // 4xx Client Errors
    if (code == HTTP_BAD_REQUEST) return "Bad Request";
    if (code == HTTP_UNAUTHORIZED) return "Unauthorized";
    if (code == HTTP_FORBIDDEN) return "Forbidden";
    if (code == HTTP_NOT_FOUND) return "Not Found";
    if (code == HTTP_METHOD_NOT_ALLOWED) return "Method Not Allowed";
    if (code == HTTP_REQUEST_TIMEOUT) return "Request Timeout";
    if (code == HTTP_CONFLICT) return "Conflict";
    if (code == HTTP_GONE) return "Gone";
    if (code == HTTP_LENGTH_REQUIRED) return "Length Required";
    if (code == HTTP_PAYLOAD_TOO_LARGE) return "Payload Too Large";
    if (code == HTTP_URI_TOO_LONG) return "URI Too Long";
    if (code == HTTP_UNSUPPORTED_MEDIA_TYPE) return "Unsupported Media Type";
    if (code == HTTP_TOO_MANY_REQUESTS) return "Too Many Requests";

    // 5xx Server Errors
    if (code == HTTP_INTERNAL_SERVER_ERROR) return "Internal Server Error";
    if (code == HTTP_NOT_IMPLEMENTED) return "Not Implemented";
    if (code == HTTP_BAD_GATEWAY) return "Bad Gateway";
    if (code == HTTP_SERVICE_UNAVAILABLE) return "Service Unavailable";
    if (code == HTTP_GATEWAY_TIMEOUT) return "Gateway Timeout";
    if (code == HTTP_HTTP_VERSION_NOT_SUPPORTED) return "HTTP Version Not Supported";

    return "Unknown Status";
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

struct http_request *parse_http_request(const char *request) {
    struct http_request *http_request = (struct http_request *)malloc(sizeof(struct http_request));
    struct http_headers http_headers = {};
    struct http_query_parameters http_query_parameters = {};

    memset(http_request, 0, sizeof(struct http_request));

    char *request_buffer = strdup(request);
    const char *header_start;
    
    /* end of http start line */
    char *request_line = strstr(request_buffer, "\r\n");
    if (request_line == NULL) {
        free(http_request);
        return NULL;
    }
    *request_line = '\0';
    header_start = request + (int)(request_line - request_buffer);

    char *method = request_buffer;     
    if (!method) {
        free(http_request);
        return NULL;
    }
    char *path_with_query = strstr(request_buffer, " ");    
    if (!path_with_query) {
        free(http_request);
        return NULL;
    }
    *path_with_query = '\0';
    path_with_query++;

    char *version = strstr(path_with_query, " ");
    if (!version) {
        free(http_request);
        return NULL;
    }
    *version = '\0';
    version++;


    enum http_method parsed_method = parse_http_method(method);
    enum http_version parsed_version = parse_http_version(version);

    // 경로와 쿼리 파라미터 분리
    char *query = NULL;
    char *query_start = strchr(path_with_query + 1, '?');

    if (query_start) {
        *query_start = '\0'; // '?'를 '\0'으로 변경하여 경로와 쿼리 분리
        query = strdup(query_start + 1);
    }

    char *path = strdup(path_with_query);

    // 헤더와 본문 파싱
    char *header = NULL;
    char *body = NULL;

    if (header_start) {
        header_start += 2; // "\r\n" 건너뛰기
        char *body_start = strstr(header_start, "\r\n\r\n");
        if (!body_start) {
            free(http_request);
            return NULL;
        }

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
        http_request, 
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

int run_web_server(struct web_server server) {
    const size_t KB = 1024;

    struct sockaddr_in addr;
    
    int                     server_fd;
    int                     addrlen = sizeof(addr);
    int                     buffer_size_kb = 16;
    char                    *buffer = NULL;

    struct http_response    response_500;
    struct http_response    response_404;
    struct http_response    response_204;

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

    // 버퍼 할당
    buffer = (char *)malloc(buffer_size_kb * KB);
    if (!buffer) {
        DLOGV("Failed to allocate buffer memory");
        return errno;
    }

    // 소켓 생성
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket"); 
        free(buffer);
        return errno;
    }

    // 소켓 재사용 옵션 설정
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        DLOGV("setsockopt failed: %s", strerror(errno)); 
        close(server_fd);
        free(buffer);
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


    DLOGV("[Server] port: %d, backlog: %d\n", server.port_num, server.backlog);  

    while (1) {
        struct route            *found_route;
        struct http_response    *response = NULL;
        struct http_request     *request;
        char                    *response_str;
        int                     client_socket;
        ssize_t                 bytes_read;
        ssize_t                 total_read;

        if ((client_socket = accept(server_fd, (struct sockaddr *)&addr, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            // @TODO server log print
            DLOGV("Failed to accept client socket: %s", strerror(errno));
            continue;
        }

        // 수신 버퍼 초기화
        memset(buffer, 0, buffer_size_kb * KB);

        // 강의자료에 있는 while 문 read 참고해서 구현 : START
        total_read = 0;
        bytes_read = read(client_socket, buffer, (size_t)KB * buffer_size_kb - 1);
        total_read += bytes_read;
        // 강의자료에 있는 while 문 read 참고해서 구현 : END


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
        found_route = find_route(server.route_table, request->path, request->method);

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

        // 클라이언트에 응답 전송
        write(client_socket, response_str, strlen(response_str));
        // @TODO 에러 시 서버 로그 추가
        // @TODO 로깅 함수 utilty에 추가

        // 메모리 정리
        free(response_str);

        if (request) {
            destruct_http_request(request);
            free(request);
        }
        close(client_socket);  // 클라이언트 소켓 닫기        
    }

    free(buffer);  // 버퍼 메모리 해제
    close(server_fd);  // 서버 소켓 닫기
    return 0;
}

char* extract_source_code_from_body(const char* body) {
    if (!body) {
        return NULL;
    }

    // "source_code" 키 찾기
    const char* key = "\"source_code\"";
    char* key_pos = strstr(body, key);
    if (!key_pos) {
        return NULL;
    }

    // 키 이후 위치로 포인터 이동
    key_pos += strlen(key);

    // 키 다음의 콜론 찾기
    char* colon_pos = strchr(key_pos, ':');
    if (!colon_pos) {
        return NULL;
    }

    // 콜론 다음의 공백 문자 건너뛰기
    char* value_start = colon_pos + 1;
    while (*value_start == ' ' || *value_start == '\t' || *value_start == '\n' || *value_start == '\r') {
        value_start++;
    }

    // 값이 따옴표로 시작하는지 확인
    if (*value_start != '"') {
        return NULL;
    }
    value_start++; // 시작 따옴표 건너뛰기

    // 종료 따옴표 찾기
    char* value_end = value_start;
    while (*value_end && *value_end != '"') {
        if (*value_end == '\\' && *(value_end + 1)) {
            value_end += 2; // 이스케이프된 문자 건너뛰기
        } else {
            value_end++;
        }
    }

    if (*value_end != '"') {
        return NULL;
    }

    // 결과 문자열 길이 계산 및 메모리 할당
    size_t value_length = value_end - value_start;
    char* result = (char*)malloc(value_length + 1);
    if (!result) {
        return NULL;
    }

    // 값 복사 및 이스케이프 문자 처리
    char* dest = result;
    const char* src = value_start;
    while (src < value_end) {
        if (*src == '\\' && *(src + 1)) {
            src++;
            switch (*src) {
                case 'n': *dest = '\n'; break;
                case 'r': *dest = '\r'; break;
                case 't': *dest = '\t'; break;
                case '\\': *dest = '\\'; break;
                case '"': *dest = '"'; break;
                default: *dest = *src;
            }
        } else {
            *dest = *src;
        }
        dest++;
        src++;
    }
    *dest = '\0';

    return result;
}