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

    // 구성 저장
    config->language = language->value;
    config->compiler_type = compiler_type->value;
    config->source_code = extract_source_code_from_body(request.body);
    config->compile_options = compile_opt ? trim_string(compile_opt->value) : NULL;
    config->command_line_args = args ? trim_string(args->value) : NULL;

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
        response->status_code = HTTP_BAD_REQUEST;
        response->body = strdup("Invalid language specified");
        response->headers = headers;
        response->http_version = HTTP_1_1;
        return response;
    }

    FILE *fp = fopen(source_code_file, "w");
    if (!fp) {
        response->status_code = HTTP_INTERNAL_SERVER_ERROR;
        response->body = strdup("Failed to create source code file");
        response->headers = headers;
        response->http_version = HTTP_1_1;
        return response;
    }

    if (fprintf(fp, "%s", config->source_code) < 0 || fclose(fp) != 0) {
        remove(source_code_file);
        response->status_code = HTTP_INTERNAL_SERVER_ERROR;
        response->body = strdup("Failed to write source code to file");
        response->headers = headers;
        response->http_version = HTTP_1_1;
        return response;
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

    // 2. 필수 쿼리 파라미터 'pid' 검증
    struct http_query_parameter *pid_query_parameter = find_query_parameter(&request.query_parameters, "pid");
    DLOG("%s\n", pid_query_parameter->value);

    if (!pid_query_parameter || !pid_query_parameter->value || strlen(pid_query_parameter->value) == 0) {
        response->status_code = HTTP_BAD_REQUEST;
        response->body = strdup("Missing required query parameter: language");
        response->headers = response_headers;
        response->http_version = HTTP_1_1;

        return response;
    }

    // 3. 로직
    int pid = atoi(pid_query_parameter->value);

    // 4. 성공 응답 생성
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
 * @brief Handle POST requests to /run/text-mode endpoint
 *
 * @param request HTTP request containing JSON body and query parameters
 * @return struct http_response* Response containing process ID or error
 */
static struct http_response *handle_text_mode(struct http_request request) {
    DLOG("Enter '/run/text-mode' route\n");

    int result = -2; // build_and_run 반환값 저장 변수

    // 1. 응답 구조체 초기화
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

    // 2. Content-Type 헤더 검증
    struct http_header *content_type = find_header(&request.headers, "Content-Type");
    if (!content_type || strcmp(content_type->value, "application/json") != 0) {
        response->status_code = HTTP_BAD_REQUEST;
        response->body = strdup("Content-Type must be application/json");
        response->headers = headers;
        response->http_version = HTTP_1_1;
        return response;
    }

    // // 3. 필수 쿼리 파라미터 'language' 검증
    // struct http_query_parameter *language = find_query_parameter(
    //     &request.query_parameters,
    //     "language");
    // if (!language || !language->value || strlen(language->value) == 0) {
    //     response->status_code = HTTP_BAD_REQUEST;
    //     response->body = strdup("Missing required query parameter: language");
    //     response->headers = headers;
    //     response->http_version = HTTP_1_1;
    //     return response;
    // }

    // // 4. 필수 쿼리 파라미터 'compiler_type' 검증
    // struct http_query_parameter *compiler_type = find_query_parameter(
    //     &request.query_parameters,
    //     "compiler_type");
    // if (!compiler_type || !compiler_type->value || strlen(compiler_type->value) == 0) {
    //     response->status_code = HTTP_BAD_REQUEST;
    //     response->body = strdup("Missing required query parameter: compiler_type");
    //     response->headers = headers;
    //     response->http_version = HTTP_1_1;
    //     return response;
    // }

    // // 5. 선택 파라미터 검증 -> 없으면 NULL 값 반환
    // struct http_query_parameter *compile_opt =
    //     find_query_parameter(&request.query_parameters, "compile_option");
    // struct http_query_parameter *args =
    //     find_query_parameter(&request.query_parameters, "argument");

    // Test용 코드
    struct http_query_parameter *language = malloc(sizeof(struct http_query_parameter));
    if (!language) {
        response->status_code = HTTP_INTERNAL_SERVER_ERROR;
        response->body = strdup("Failed to allocate memory");
        response->headers = headers;
        response->http_version = HTTP_1_1;
        return response;
    }
    language->key = strdup("language");
    language->value = strdup("c");

    struct http_query_parameter *compiler_type = malloc(sizeof(struct http_query_parameter));
    if (!compiler_type) {
        free(language->key);
        free(language->value);
        free(language);
        response->status_code = HTTP_INTERNAL_SERVER_ERROR;
        response->body = strdup("Failed to allocate memory");
        response->headers = headers;
        response->http_version = HTTP_1_1;
        return response;
    }
    compiler_type->key = strdup("compiler_type");
    compiler_type->value = strdup("gcc");

    struct http_query_parameter *compile_opt = malloc(sizeof(struct http_query_parameter));
    if (!compile_opt) {
        free(language->key);
        free(language->value);
        free(language);
        free(compiler_type->key);
        free(compiler_type->value);
        free(compiler_type);
        response->status_code = HTTP_INTERNAL_SERVER_ERROR;
        response->body = strdup("Failed to allocate memory");
        response->headers = headers;
        response->http_version = HTTP_1_1;
        return response;
    }
    compile_opt->key = strdup("compile_option");
    compile_opt->value = NULL;

    struct http_query_parameter *args = malloc(sizeof(struct http_query_parameter));
    if (!args) {
        free(language->key);
        free(language->value);
        free(language);
        free(compiler_type->key);
        free(compiler_type->value);
        free(compiler_type);
        free(compile_opt->key);
        free(compile_opt);
        response->status_code = HTTP_INTERNAL_SERVER_ERROR;
        response->body = strdup("Failed to allocate memory");
        response->headers = headers;
        response->http_version = HTTP_1_1;
        return response;
    }
    args->key = strdup("argument");
    args->value = NULL;

    // 5-1. 선택 파라미터 trim
    char *trimmed_compile_opt = trim_string(compile_opt ? compile_opt->value : NULL);
    char *trimmed_args = trim_string(args ? args->value : NULL);

    // 6. request body 검증
    if (!request.body || strlen(request.body) == 0) {
        response->status_code = HTTP_BAD_REQUEST;
        response->body = strdup("Missing request body");
        response->headers = headers;
        response->http_version = HTTP_1_1;
        return response;
    }

    /**
     * @brief body 내용을 소스코드 파일로 만든다.
     * 만드는 파일 이름은 temp/{source_code_count}.txt로 만든다.
     * 단, 소스코드 파일로 만드는 중 오류가 생기면 response에 500 에러를 담아 반환한다.
     *
     * */
    char source_code_file[32];
    snprintf(source_code_file, sizeof(source_code_file), "./temp/%d.txt", atomic_fetch_add(&source_code_count, 1));

    FILE *fp = fopen(source_code_file, "w");

    if (!fp) {
        response->status_code = HTTP_INTERNAL_SERVER_ERROR;
        response->body = strdup("Failed to create source code file");
        response->headers = headers;
        response->http_version = HTTP_1_1;

        return response;
    }

    if (fprintf(fp, "%s", extract_source_code_from_body(request.body)) < 0 || fclose(fp) != 0) {
        remove(source_code_file);
        response->status_code = HTTP_INTERNAL_SERVER_ERROR;
        response->body = strdup("Failed to write source code to file");
        response->headers = headers;
        response->http_version = HTTP_1_1;

        return response;
    }

    /**
     * languae 이 c, cpp 인지 판단 후, compiler_type이 gcc인지 clang인지 판단하여 build_and_run에 넘겨준다.
     *
     */
    if (strcmp(language->value, "c") == 0) {
        if (strcmp(compiler_type->value, "gcc") == 0) {
            // gcc로 컴파일
            result = build_and_run(source_code_file, gcc_c, trimmed_compile_opt, trimmed_args);
            if (result < 0) {
                response->status_code = HTTP_INTERNAL_SERVER_ERROR;
                response->body = strdup("Failed to build and run program");

                free(trimmed_compile_opt);
                free(trimmed_args);
                return response;
            }
        } else if (strcmp(compiler_type->value, "clang") == 0) {
            // clang으로 컴파일
            result = build_and_run(source_code_file, clang_c, trimmed_compile_opt, trimmed_args);
            if (result < 0) {
                response->status_code = HTTP_INTERNAL_SERVER_ERROR;
                response->body = strdup("Failed to build and run program");

                free(trimmed_compile_opt);
                free(trimmed_args);
                return response;
            }
        }
    } else if (strcmp(language->value, "cpp") == 0) {
        if (strcmp(compiler_type->value, "gcc") == 0) {
            // gcc로 컴파일
            result = build_and_run(source_code_file, gcc_cpp, trimmed_compile_opt, trimmed_args);
            if (result < 0) {
                response->status_code = HTTP_INTERNAL_SERVER_ERROR;
                response->body = strdup("Failed to build and run program");

                free(trimmed_compile_opt);
                free(trimmed_args);
                return response;
            }
        } else if (strcmp(compiler_type->value, "clang") == 0) {
            // clang으로 컴파일
            result = build_and_run(source_code_file, clang_cpp, trimmed_compile_opt, trimmed_args);
            if (result < 0) {
                response->status_code = HTTP_INTERNAL_SERVER_ERROR;
                response->body = strdup("Failed to build and run program");

                free(trimmed_compile_opt);
                free(trimmed_args);
                return response;
            }
        }
    }

    // @TODO 에러 핸들링 필요할 것으로 보임 (임시로 assert 를 걸었습니다)
    assert(result != -2);
    // PID 반환
    int pid = result;

    // 5. 성공 응답 생성
    char response_body[32];
    snprintf(response_body, sizeof(response_body), "{\"pid\": %d}", pid);

    // 응답 헤더 설정
    insert_header(&headers, "Content-Type", "application/json");
    insert_header(&headers, "Access-Control-Allow-Origin", "*");
    insert_header(&headers, "Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    insert_header(&headers, "Access-Control-Allow-Headers", "*");

    response->status_code = HTTP_OK;
    response->body = strdup(response_body);
    response->headers = headers;
    response->http_version = HTTP_1_1;

    // Test 용 코드 메모리 해제
    if (language) {
        if (language->key)
            free(language->key);
        if (language->value)
            free(language->value);
        free(language);
    }
    if (compiler_type) {
        if (compiler_type->key)
            free(compiler_type->key);
        if (compiler_type->value)
            free(compiler_type->value);
        free(compiler_type);
    }
    if (compile_opt) {
        if (compile_opt->key)
            free(compile_opt->key);
        if (compile_opt->value)
            free(compile_opt->value);
        free(compile_opt);
    }
    if (args) {
        if (args->key)
            free(args->key);
        if (args->value)
            free(args->value);
        free(args);
    }

    // // 쿼리 파라미터 메모리 해제
    // if (language) {
    //     if (language->key)
    //         free(language->key);
    //     if (language->value)
    //         free(language->value);
    //     free(language);
    // }

    // if (compiler_type) {
    //     if (compiler_type->key)
    //         free(compiler_type->key);
    //     if (compiler_type->value)
    //         free(compiler_type->value);
    //     free(compiler_type);
    // }

    // if (compile_opt) {
    //     if (compile_opt->key)
    //         free(compile_opt->key);
    //     if (compile_opt->value)
    //         free(compile_opt->value);
    //     free(compile_opt);
    // }

    // if (args) {
    //     if (args->key)
    //         free(args->key);
    //     if (args->value)
    //         free(args->value);
    //     free(args);
    // }

    // trimmed 문자열 해제
    if (trimmed_compile_opt)
        free(trimmed_compile_opt);
    if (trimmed_args)
        free(trimmed_args);

    return response;
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