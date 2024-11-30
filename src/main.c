#include "http.h"
#include "utility.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-function"

/**
 * @test curl -X POST http://localhost:10010/stop -H "Content-Type: application/json" -d '{"key": "value"}'
 */
static struct http_response *stop_callback(struct http_request request) {
    struct http_response *response = (struct http_response *)malloc(sizeof(struct http_response));

    response->http_version = HTTP_1_1;
    response->status_code = HTTP_OK;
    response->body = strdup("{'is_success': true}");
    response->headers = (struct http_headers) {
        .size = 0,
        .capacity = 0, 
        .items = NULL
    };

    return response;
}

/**
 * @test curl -X POST http://localhost:10010/input -H "Content-Type: application/json" -d '{"key": "input_value"}'
 */
static struct http_response *input_callback(struct http_request request) 
{
    struct http_response *response = (struct http_response *)malloc(sizeof(struct http_response));

    response->http_version = HTTP_1_1;
    response->status_code = HTTP_OK;
    response->body = strdup("{'is_success': true}");
    response->headers = (struct http_headers) {
        .size = 0,
        .capacity = 0,
        .items = NULL
    };

    return response;
}

/**
 * @test curl -X GET http://localhost:10010/program
 */
static struct http_response *program_callback(struct http_request request) {
    struct http_response *response = (struct http_response *)malloc(sizeof(struct http_response));

    response->http_version = HTTP_1_1;
    response->status_code = HTTP_OK;
    response->body = strdup("{'is_success': true}");
    response->headers = (struct http_headers) {
        .size = 0,
        .capacity = 0,
        .items = NULL
    };

    return response;
}

/**
 * @brief Handle POST requests to /run/text-mode endpoint
 *
 * @param request HTTP request containing JSON body and query parameters
 * @return struct http_response* Response containing process ID or error
 */
static struct http_response* handle_text_mode(struct http_request request) {
    // 1. 응답 구조체 초기화
    struct http_response* response = malloc(sizeof(struct http_response));
    if (!response) {
        return NULL;
    }

    struct http_headers headers = {
        .capacity = 8,
        .size = 0,
        .items = malloc(8 * sizeof(struct http_header*))
    };
    if (!headers.items) {
        free(response);
        return NULL;
    }

    // 2. Content-Type 헤더 검증
    struct http_header* content_type = find_header(&request.headers, "Content-Type");
    if (!content_type || strcmp(content_type->value, "application/json") != 0) {
        response->status_code = HTTP_BAD_REQUEST;
        response->body = strdup("Content-Type must be application/json");
        response->headers = headers;
        response->http_version = HTTP_1_1;
        return response;
    }

    // 3. 필수 쿼리 파라미터 'language' 검증
    struct http_query_parameter* language = find_query_parameter(
        &request.query_parameters, 
        "language"
    );
    if (!language || !language->value || strlen(language->value) == 0) {
        response->status_code = HTTP_BAD_REQUEST;
        response->body = strdup("Missing required query parameter: language");
        response->headers = headers;
        response->http_version = HTTP_1_1;
        return response;
    }

    // 3. 선택 파라미터 검증
    struct http_query_parameter* compile_opt = 
        find_query_parameter(&request.query_parameters, "compile_option");
    struct http_query_parameter* args = 
        find_query_parameter(&request.query_parameters, "argument");
    

    // 4. request body 검증
    if (!request.body || strlen(request.body) == 0) {
        response->status_code = HTTP_BAD_REQUEST;
        response->body = strdup("Missing request body");
        response->headers = headers;
        response->http_version = HTTP_1_1;
        return response;
    }

    // TODO: 본문을 파싱하고 코드 실행
    // 현재는 임시 PID 반환
    int pid = 12345; 

    // 5. 성공 응답 생성
    char response_body[32];
    snprintf(response_body, sizeof(response_body), "{\"pid\": %d}", pid);

    insert_header(&headers, "Content-Type", "application/json");
    
    response->status_code = HTTP_OK;
    response->body = strdup(response_body);
    response->headers = headers;
    response->http_version = HTTP_1_1;

    return response;
}

/**
 * @brief Handle POST requests to /run/interactive-mode endpoint
 * 
 * @param request HTTP request containing JSON body and query parameters
 * @return struct http_response* Response containing process ID or error
 */
static struct http_response* handle_interactive_mode(struct http_request request) {
   struct http_response* response = malloc(sizeof(struct http_response));
   if (!response) {
       return NULL;
   }

   struct http_headers headers = {
       .capacity = 8,
       .size = 0,
       .items = malloc(8 * sizeof(struct http_header*))
   };

   if (!headers.items) {
       free(response);
       return NULL;
   }

   // 1. Content-Type 헤더 유효성 검사
   struct http_header* content_type = find_header(&request.headers, "Content-Type");
   if (!content_type || strcmp(content_type->value, "application/json") != 0) {
       response->status_code = HTTP_BAD_REQUEST;
       response->body = strdup("Content-Type must be application/json");
       response->headers = headers;
       response->http_version = HTTP_1_1;
       return response;
   }

   // 2. 필수 쿼리 파라미터 'language' 유효성 검사
   struct http_query_parameter* language = find_query_parameter(
       &request.query_parameters, 
       "language"
   );
   if (!language || !language->value || strlen(language->value) == 0) {
       response->status_code = HTTP_BAD_REQUEST;
       response->body = strdup("Missing required query parameter: language");
       response->headers = headers;
       response->http_version = HTTP_1_1;
       return response;
   }

   // 3. 선택적 파라미터(compile_option, argument)는 유효성 검사가 필요 없음
   struct http_query_parameter* compile_option = find_query_parameter(
       &request.query_parameters, 
       "compile_option"
   );
   struct http_query_parameter* argument = find_query_parameter(
       &request.query_parameters, 
       "argument"
   );

   // 4. 요청 본문 유효성 검사
   if (!request.body || strlen(request.body) == 0) {
       response->status_code = HTTP_BAD_REQUEST;
       response->body = strdup("Missing request body");
       response->headers = headers;
       response->http_version = HTTP_1_1;
       return response;
   }

   // TODO: 본문을 파싱하고 코드 실행
   // 현재는 임시 PID 반환
   int pid = 12345;

   // 5. 성공 응답 생성
   char response_body[32];
   snprintf(response_body, sizeof(response_body), "{\"pid\": %d}", pid);

   // 응답 헤더 설정
   insert_header(&headers, "Content-Type", "application/json");
   
   // 응답 설정
   response->status_code = HTTP_OK;
   response->body = strdup(response_body);
   response->headers = headers;
   response->http_version = HTTP_1_1;

   return response;
}

/**
 * @brief Handle POST requests to /run/debugger endpoint
 * 
 * @param request HTTP request containing JSON body and query parameters
 * @return struct http_response* Response containing process ID or error
 */
static struct http_response* handle_debugger(struct http_request request) {
    struct http_response* response = malloc(sizeof(struct http_response));
    if (!response) {
        return NULL;
    }

    struct http_headers headers = {
        .capacity = 8,
        .size = 0,
        .items = malloc(8 * sizeof(struct http_header*))
    };

    if (!headers.items) {
        free(response);
        return NULL;
    }

    // 1. Content-Type 헤더 유효성 검사
    struct http_header* content_type = find_header(&request.headers, "Content-Type");
    if (!content_type || strcmp(content_type->value, "application/json") != 0) {
        response->status_code = HTTP_BAD_REQUEST;
        response->body = strdup("Content-Type must be application/json");
        response->headers = headers;
        response->http_version = HTTP_1_1;
        return response;
    }

    // 2. language 파라미터 유효성 검사
    struct http_query_parameter* language = find_query_parameter(
        &request.query_parameters, 
        "language"
    );
    if (!language || !language->value || strlen(language->value) == 0) {
        response->status_code = HTTP_BAD_REQUEST;
        response->body = strdup("Missing required query parameter: language");
        response->headers = headers;
        response->http_version = HTTP_1_1;
        return response;
    }

    // 3. 선택적 파라미터 처리
    struct http_query_parameter* compile_option = find_query_parameter(
        &request.query_parameters, 
        "compile_option"
    );
    struct http_query_parameter* argument = find_query_parameter(
        &request.query_parameters, 
        "argument"
    );

    // 4. 요청 본문 검증
    if (!request.body || strlen(request.body) == 0) {
        response->status_code = HTTP_BAD_REQUEST;
        response->body = strdup("Missing request body");
        response->headers = headers;
        response->http_version = HTTP_1_1;
        return response;
    }

    // TODO: 실제 디버거 실행 코드 구현
    int pid = 12345;  // 임시 PID

    // 5. 성공 응답 생성
    char response_body[32];
    snprintf(response_body, sizeof(response_body), "{\"pid\": %d}", pid);

    // 응답 헤더 설정
    insert_header(&headers, "Content-Type", "application/json");
    
    // 응답 설정
    response->status_code = HTTP_OK;
    response->body = strdup(response_body);
    response->headers = headers;
    response->http_version = HTTP_1_1;

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

    insert_route(&route_table, "/stop", HTTP_POST, stop_callback);
    insert_route(&route_table, "/input", HTTP_POST, input_callback);
    insert_route(&route_table, "/program", HTTP_GET, program_callback);
   
    insert_route(&route_table, "/run/text-mode", HTTP_POST, handle_text_mode);
    insert_route(&route_table, "/run/interactive-mode", HTTP_POST, handle_interactive_mode);
    insert_route(&route_table, "/run/debugger", HTTP_POST, handle_debugger);


    struct web_server app = (struct web_server) {
        .route_table = &route_table,
        .port_num = 10010,
        .backlog = 10
    };
  
    run_web_server(app);
    return 0;
}

#pragma GCC diagnostic pop