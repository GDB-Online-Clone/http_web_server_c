#ifdef __clang__
#define _POSIX_C_SOURCE 200809L
#endif

#include <assert.h>
#include <ctype.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <webserver/http.h>
#include <webserver/utility.h>
#include <webserver/json.h>

#include "service.h"

static atomic_int source_code_count = 0;

/**
 * @brief Trim whitespace from the beginning and end of a string
 * @param str String to trim
 * @return Trimmed string (must be freed by caller)
 * @note Returns NULL if input is NULL
 */
static char *trim_string(const char *str) {
    if (!str)
        return NULL;

    // Skip leading whitespace
    while (*str && isspace(*str)) {
        str++;
    }
    // Handle empty string
    if (*str == '\0') {
        return strdup("");
    }

    // Find end of string
    const char *end = str + strlen(str) - 1;
    while (end > str && isspace(*end)) {
        end--;
    }

    // Copy trimmed string
    int len = end - str + 1;
    char *trimmed = malloc(len + 1);
    if (!trimmed) {
        return NULL;
    }

    strncpy(trimmed, str, len);
    trimmed[len] = '\0';

    return trimmed;
}

/**
 * @brief Common configuration for handling program execution requests
 */
struct run_handler_config {
    const char *source_code;       /**< Source code to compile and run */
    const char *language;          /**< Programming language (c/cpp) */
    const char *compiler_type;     /**< Compiler to use (gcc/clang) */
    const char *compile_options;   /**< Additional compiler options */
    const char *command_line_args; /**< Command line arguments for the program */
    const json_object_t *parsed_body /**< parsed request body by `parse_json()` */
};

/**
 * @brief Validate and process program execution request
 *
 * @param request HTTP request containing execution parameters
 * @param config Output parameter to store validated configuration
 * @return struct http_response* Response on validation failure, NULL on success
 */
static struct http_response *validate_run_request(
    struct http_request request,
    struct run_handler_config *config) {
    struct http_response *response = malloc(sizeof(struct http_response));
    if (!response) {
        return NULL;
    }

    struct http_headers headers = {
        .capacity = 8,
        .size = 0,
        .items = malloc(8 * sizeof(struct http_header *))};

    if (!headers.items) {
        free(response);
        return NULL;
    }

    // 1. Content-Type 헤더 검증
    struct http_header *content_type = find_header(&request.headers, "Content-Type");
    DLOGV("%u\n", (unsigned int)content_type);
    
    if (!content_type || strcmp(content_type->value, "application/json") != 0) {
        response->status_code = HTTP_BAD_REQUEST;
        response->body = strdup("Content-Type must be application/json");
        response->headers = headers;
        response->http_version = HTTP_1_1;
        return response;
    }

    // 2. language 파라미터 검증
    struct http_query_parameter *language =
        find_query_parameter(&request.query_parameters, "language");
    if (!language || !language->value || strlen(language->value) == 0) {
        response->status_code = HTTP_BAD_REQUEST;
        response->body = strdup("Missing required query parameter: language");
        response->headers = headers;
        response->http_version = HTTP_1_1;
        return response;
    }

    // 3. compiler_type 파라미터 검증
    struct http_query_parameter *compiler_type =
        find_query_parameter(&request.query_parameters, "compiler_type");
    if (!compiler_type || !compiler_type->value || strlen(compiler_type->value) == 0) {
        response->status_code = HTTP_BAD_REQUEST;
        response->body = strdup("Missing required query parameter: compiler_type");
        response->headers = headers;
        response->http_version = HTTP_1_1;
        return response;
    }

    // 4. 요청 본문 검증
    if (!request.body || strlen(request.body) == 0) {
        response->status_code = HTTP_BAD_REQUEST;
        response->body = strdup("Missing request body");
        response->headers = headers;
        response->http_version = HTTP_1_1;
        return response;
    }

    // 5. 선택적 파라미터 처리
    struct http_query_parameter *compile_opt =
        find_query_parameter(&request.query_parameters, "compile_option");
    struct http_query_parameter *args =
        find_query_parameter(&request.query_parameters, "argument");


    json_object_t *request_body = parse_json(request.body);

    if (request_body == NULL) {
        DLOGV("informed `parse failed`\n");
        destruct_http_headers(&headers);
        free(response);
        return NULL;
    }

    // 구성 저장
    config->language = language->value;
    config->compiler_type = compiler_type->value;
    config->source_code = find_json_element(request_body, "source_code")->value;
    config->compile_options = compile_opt ? trim_string(compile_opt->value) : NULL;
    config->command_line_args = args ? trim_string(args->value) : NULL;
    config->parsed_body = request_body;

    // 임시 응답 구조체 정리
    destruct_http_headers(&headers);
    free(response);

    return NULL;
}

/**
 * @brief Execute program based on validated configuration
 *
 * @param config Validated run configuration
 * @return struct http_response* Response containing execution results or error
 */
static struct http_response *execute_program(const struct run_handler_config *config) {
    struct http_response *response = malloc(sizeof(struct http_response));
    if (!response) {
        return NULL;
    }

    struct http_headers headers = {
        .capacity = 8,
        .size = 0,
        .items = malloc(8 * sizeof(struct http_header *))};

    if (!headers.items) {
        free(response);
        return NULL;
    }

    // 응답 헤더 설정
    insert_header(&headers, "Content-Type", "application/json");
    insert_header(&headers, "Access-Control-Allow-Origin", "*");
    insert_header(&headers, "Access-Control-Allow-Methods",
                  "GET, POST, PUT, DELETE, OPTIONS");
    insert_header(&headers, "Access-Control-Allow-Headers", "*");

    // 소스 파일 생성
    char source_code_file[32];
    if(strcmp(config->language, "c") == 0){
        snprintf(source_code_file, sizeof(source_code_file),
             "./temp/%d.c", atomic_fetch_add(&source_code_count, 1));
    } else if(strcmp(config->language, "cpp") == 0){
        snprintf(source_code_file, sizeof(source_code_file),
             "./temp/%d.cpp", atomic_fetch_add(&source_code_count, 1));
    } else {
        assert(0);
        return NULL;
    }

    FILE *fp = fopen(source_code_file, "w");
    if (!fp) {
        response->status_code = HTTP_INTERNAL_SERVER_ERROR;
        response->body = strdup("Failed to create source code file");
        response->headers = headers;
        response->http_version = HTTP_1_1;
        return response;
    }

    if (fprintf(fp, "%s", config->source_code) < 0) {
        if (fclose(fp) != 0) {
            perror("fclose");            
        }
        remove(source_code_file);
        response->status_code = HTTP_INTERNAL_SERVER_ERROR;
        response->body = strdup("Failed to write source code to file");
        response->headers = headers;
        response->http_version = HTTP_1_1;
        return response;
    }
    
    if (fclose(fp) != 0) {
        perror("fclose");            
    }

    // 컴파일러 선택 및 실행
    int result = -2;
    if (strcmp(config->language, "c") == 0) {
        if (strcmp(config->compiler_type, "gcc") == 0) {
            result = build_and_run(source_code_file, gcc_c,
                                   config->compile_options, config->command_line_args);
        } else if (strcmp(config->compiler_type, "clang") == 0) {
            result = build_and_run(source_code_file, clang_c,
                                   config->compile_options, config->command_line_args);
        } else {
            response->status_code = HTTP_BAD_REQUEST;
            response->body = strdup("Invalid compiler type specified");
            response->headers = headers;
            response->http_version = HTTP_1_1;
            return response;
        }
    } else if (strcmp(config->language, "cpp") == 0) {
        if (strcmp(config->compiler_type, "gcc") == 0) {
            result = build_and_run(source_code_file, gcc_cpp,
                                   config->compile_options, config->command_line_args);
        } else if (strcmp(config->compiler_type, "clang") == 0) {
            result = build_and_run(source_code_file, clang_cpp,
                                   config->compile_options, config->command_line_args);
        } else {
            response->status_code = HTTP_BAD_REQUEST;
            response->body = strdup("Invalid compiler type specified");
            response->headers = headers;
            response->http_version = HTTP_1_1;
            return response;
        }
    } else {
        response->status_code = HTTP_BAD_REQUEST;
        response->body = strdup("Invalid language specified");
        response->headers = headers;
        response->http_version = HTTP_1_1;
        return response;
    }

    if (result == -1) {
        response->status_code = HTTP_INTERNAL_SERVER_ERROR;
        response->body = strdup("Failed to build and run program");
        response->headers = headers;
        response->http_version = HTTP_1_1;
        return response;
    } else if(result == -2) {
        response->status_code = HTTP_BAD_REQUEST;
        response->body = strdup("Invaild request");
        response->headers = headers;
        response->http_version = HTTP_1_1;
        return response;
    }

    char *input_data = find_json_element(config->parsed_body, "stdin");
    if (input_data) {
        int pass_input_res = pass_input_to_child(result, input_data);
        assert(pass_input_res != -2);
        if (pass_input_res == -1) {
            DLOGV("ERROR OCCURED (pass input)\n");
        }
    }

    // 성공 응답 생성
    char response_body[32];
    snprintf(response_body, sizeof(response_body), "{\"pid\": %d}", result);

    response->status_code = HTTP_OK;
    response->body = strdup(response_body);
    response->headers = headers;
    response->http_version = HTTP_1_1;

    return response;
}

/**
 * @brief Common handler for program execution requests
 *
 * @param request HTTP request containing execution parameters
 * @return struct http_response* Response containing execution results or error
 */
static struct http_response *handle_run_request(struct http_request request) {
    struct run_handler_config config = {0};

    // 요청 검증
    struct http_response *validation_response = validate_run_request(request, &config);
    if (validation_response != NULL) {
        return validation_response;
    }

    // 프로그램 실행
    struct http_response *response = execute_program(&config);

    // 리소스 정리
    if (config.compile_options)
        free((void *)config.compile_options);
    if (config.command_line_args)
        free((void *)config.command_line_args);
    if (config.parsed_body)
        free((void *)config.parsed_body);

    return response;
}

/**
 * @test curl -X POST http://localhost:10010/stop?pid=10 -H "Content-Type: application/json" -d '{"key": "value"}'
 */
static struct http_response *stop_callback(struct http_request request) {
    DLOG("Enter '/stop' route\n");

    // 1. 응답 구조체 초기화
    struct http_response *response = malloc(sizeof(struct http_response));

    if (!response) {
        return NULL;
    }

    struct http_headers response_headers = {
        .capacity = 8,
        .size = 0,
        .items = malloc(8 * sizeof(struct http_header *))};

    if (!response_headers.items) {
        free(response);
        return NULL;
    }

    // 응답 헤더 설정
    insert_header(&response_headers, "Content-Type", "application/json");
    insert_header(&response_headers, "Access-Control-Allow-Origin", "*");
    insert_header(&response_headers, "Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    insert_header(&response_headers, "Access-Control-Allow-Headers", "*");

    // 2. Content-Type 헤더 검증
    struct http_header *content_type_header = find_header(&request.headers, "Content-Type");

    if (!content_type_header || strcmp(content_type_header->value, "application/json") != 0) {
        response->status_code = HTTP_BAD_REQUEST;
        response->body = strdup("Content-Type must be application/json");
        response->headers = response_headers;
        response->http_version = HTTP_1_1;

        return response;
    }

    // 3. 필수 쿼리 파라미터 'pid' 검증
    struct http_query_parameter *pid_query_parameter = find_query_parameter(&request.query_parameters, "pid");

    if (!pid_query_parameter || !pid_query_parameter->value || strlen(pid_query_parameter->value) == 0) {
        response->status_code = HTTP_BAD_REQUEST;
        response->body = strdup("Missing required query parameter: pid");
        response->headers = response_headers;
        response->http_version = HTTP_1_1;

        return response;
    }

    // 4. 로직
    int pid = atoi(pid_query_parameter->value);
    stop_process(pid);

    // 5. 성공 응답 생성
    char response_body[32];
    snprintf(response_body, sizeof(response_body), "{\"pid\": %d}", pid);

    response->http_version = HTTP_1_1;
    response->status_code = HTTP_OK;
    response->body = strdup(response_body);
    response->headers = response_headers;

    return response;
}

/**
 * @test curl -X POST http://localhost:10010/input?pid=10 -H "Content-Type: application/json" -d '{"key": "input_value"}'
 */
static struct http_response *input_callback(struct http_request request) {
    DLOG("Enter '/input' route\n");

    // 1. 응답 구조체 초기화
    struct http_response *response = malloc(sizeof(struct http_response));

    if (!response) {
        return NULL;
    }

    struct http_headers response_headers = {
        .capacity = 8,
        .size = 0,
        .items = malloc(8 * sizeof(struct http_header *))};

    if (!response_headers.items) {
        free(response);
        return NULL;
    }

    // 2. Content-Type 헤더 검증
    struct http_header *content_type_header = find_header(&request.headers, "Content-Type");

    if (!content_type_header || strcmp(content_type_header->value, "application/json") != 0) {
        response->status_code = HTTP_BAD_REQUEST;
        response->body = strdup("Content-Type must be application/json");
        response->headers = response_headers;
        response->http_version = HTTP_1_1;

        return response;
    }

    // 3. 필수 쿼리 파라미터 'pid' 검증
    struct http_query_parameter *pid_query_parameter = find_query_parameter(&request.query_parameters, "pid");
    DLOG("%s\n", pid_query_parameter->value);

    if (!pid_query_parameter || !pid_query_parameter->value || strlen(pid_query_parameter->value) == 0) {
        response->status_code = HTTP_BAD_REQUEST;
        response->body = strdup("Missing required query parameter: language");
        response->headers = response_headers;
        response->http_version = HTTP_1_1;

        return response;
    }

    // 4. 로직
    int pid = atoi(pid_query_parameter->value);

    // 5. 성공 응답 생성
    char response_body[32];
    snprintf(response_body, sizeof(response_body), "{\"pid\": %d}", pid);

    // 응답 헤더 설정
    insert_header(&response_headers, "Content-Type", "application/json");
    insert_header(&response_headers, "Access-Control-Allow-Origin", "*");
    insert_header(&response_headers, "Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    insert_header(&response_headers, "Access-Control-Allow-Headers", "*");

    response->http_version = HTTP_1_1;
    response->status_code = HTTP_OK;
    response->body = strdup(response_body);
    response->headers = response_headers;

    return response;
}

/**
 * @test curl -X GET http://localhost:10010/program?pid=10
 */
static struct http_response *program_callback(struct http_request request) {
    DLOG("Enter '/program' route\n");

    // 1. 응답 구조체 초기화
    struct http_response *response = malloc(sizeof(struct http_response));
    if (!response) return NULL;

    struct http_headers response_headers = {
        .capacity = 8,
        .size = 0,
        .items = malloc(8 * sizeof(struct http_header *))
    };

    if (!response_headers.items) {
        free(response);
        return NULL;
    }

    // 응답 헤더 설정
    insert_header(&response_headers, "Content-Type", "application/json");
    insert_header(&response_headers, "Access-Control-Allow-Origin", "*");
    insert_header(&response_headers, "Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    insert_header(&response_headers, "Access-Control-Allow-Headers", "*");

    // 2. 필수 쿼리 파라미터 'pid' 검증
    struct http_query_parameter *pid_query_parameter = 
        find_query_parameter(&request.query_parameters, "pid");
    
    if (!pid_query_parameter || !pid_query_parameter->value || 
        strlen(pid_query_parameter->value) == 0) {
        response->status_code = HTTP_BAD_REQUEST;
        response->body = strdup("Missing required query parameter: pid");
        response->headers = response_headers;
        response->http_version = HTTP_1_1;
        return response;
    }

    // 3. 로직
    int pid = atoi(pid_query_parameter->value);

    // 4. 성공 응답 생성
    //char response_body[32];
    //snprintf(response_body, sizeof(response_body), "{\"pid\": %d}", pid);
    char *output = get_output_from_child(pid);
    if (!output) {
        response->status_code = HTTP_INTERNAL_SERVER_ERROR;
        response->body = strdup("Failed to get output from child process");
        response->headers = response_headers;
        response->http_version = HTTP_1_1;

        return response;
    }

    json_object_t response_json_body = (json_object_t) {
        .capacity = 2,
        .items = 0,
        .items = malloc(2 * sizeof(sizeof(struct json_element *)))
    };

    char pid_str[16];
    sprintf(pid_str, "%d", pid);
    insert_json_element(&response_json_body, "pid", pid_str, JSON_NUMBER);
    insert_json_element(&response_json_body, "output", output, JSON_STRING);
    
    char *response_body = json_object_stringify(&response_json_body);
    int response_body_size = strlen(response_body) + 1;

    free(output);

    response->http_version = HTTP_1_1;
    response->status_code = HTTP_OK;
    response->body = response_body;
    response->headers = response_headers;

    return response;
}

/**
 * @brief Handle POST requests to /run/text-mode endpoint
 *
 * @param request HTTP request containing JSON body and query parameters
 * @return struct http_response* Response containing process ID or error
 */
static struct http_response *handle_text_mode(struct http_request request) {
    DLOG("Enter '/run/text-mode' route\n");

    return handle_run_request(request);
}

/**
 * @brief Handle POST requests to /run/interactive-mode endpoint
 *
 * @param request HTTP request containing JSON body and query parameters
 * @return struct http_response* Response containing process ID or error
 */
static struct http_response *handle_interactive_mode(struct http_request request) {
    DLOG("Enter '/run/interactive-mode' route\n");
    return handle_run_request(request);
}

/**
 * @brief Handle POST requests to /run/debugger endpoint
 *
 * @param request HTTP request containing JSON body and query parameters
 * @return struct http_response* Response containing process ID or error
 */
static struct http_response *handle_debugger(struct http_request request) {
    DLOG("Enter '/run/debugger' route\n");
    return handle_run_request(request);
}

/**
 * @brief Build Test 용
 *
 * @return int 프로그램 종료 상태
 */
int main() {

    // temp 디렉토리 생성
    struct stat st = {0};

    if (stat("./temp", &st) == -1) {
        // temp 디렉토리가 없으면 생성 (권한: 0755)
        if (mkdir("./temp", 0755) == -1) {
            perror("Failed to create temp directory");
            return 1;
        }
    }

    // bins 디렉토리 생성
    if (stat("./bins", &st) == -1) {
        // bins 디렉토리가 없으면 생성 (권한: 0755)
        if (mkdir("./bins", 0755) == -1) {
            perror("Failed to create bins directory");
            return 1;
        }
    }

    struct routes route_table = {};
    init_routes(&route_table);

    insert_route(&route_table, "/stop", HTTP_POST, stop_callback);
    insert_route(&route_table, "/input", HTTP_POST, input_callback);
    insert_route(&route_table, "/program", HTTP_GET, program_callback);

    insert_route(&route_table, "/run/text-mode", HTTP_POST, handle_text_mode);
    insert_route(&route_table, "/run/interactive-mode", HTTP_POST, handle_interactive_mode);
    insert_route(&route_table, "/run/debugger", HTTP_POST, handle_debugger);

    struct web_server app = (struct web_server){
        .route_table = &route_table,
        .port_num = 10010,
        .backlog = 10};

    run_web_server(app);
    return 0;
}