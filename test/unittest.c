#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <webserver/http.h>
#include <webserver/utility.h>
// Setup 함수 - 유닛 테스트 환경 설정에 사용
int setup(void) {
    // 필요에 따라 초기화 작업 추가
    return 0; // 성공 시 0 반환
}
// Teardown 함수 - 유닛 테스트 환경 정리에 사용
int teardown(void) {
    // 필요에 따라 정리 작업 추가
    return 0; // 성공 시 0 반환
}


// unit test 예시
void test_example(void) {
    CU_ASSERT(2 + 5 == 7);
    CU_ASSERT(1 + -1 == 0);
    CU_ASSERT(-2 + -3 == -5);
}

/**
 * @brief parse_http_headers() test code. Input need to be parsed successfully.
 * 
 */
void test_parse_http_headers_1() {
    char *http_headers_string = 
        "no_quotes:   \"value quotes with whitespace\"\r\n"
        "\"key quotes with whitespace\": value no quotes but whitespace\r\n"
        "\"quote with colon : \": trimmed_value\r\n"
        "\"quote; \" is in key\" :value4\r\n"
        "start_with_whitespace : \"     \" \"  val\"\"\"ue5 \"\r\n"
        "\r\n";

    struct http_headers http_headers = parse_http_headers(http_headers_string);
    
    CU_ASSERT_STRING_EQUAL(http_headers.headers[0]->key,    "no_quotes");
    CU_ASSERT_STRING_EQUAL(http_headers.headers[0]->value,  "value quotes with whitespace");

    CU_ASSERT_STRING_EQUAL(http_headers.headers[1]->key,    "key quotes with whitespace");
    CU_ASSERT_STRING_EQUAL(http_headers.headers[1]->value,  "value no quotes but whitespace");

    CU_ASSERT_STRING_EQUAL(http_headers.headers[2]->key,    "quote with colon : ");
    CU_ASSERT_STRING_EQUAL(http_headers.headers[2]->value,  "trimmed_value");

    CU_ASSERT_STRING_EQUAL(http_headers.headers[3]->key,    "quote; \" is in key");
    CU_ASSERT_STRING_EQUAL(http_headers.headers[3]->value,  "value4");

    CU_ASSERT_STRING_EQUAL(http_headers.headers[4]->key,    "start_with_whitespace");
    CU_ASSERT_STRING_EQUAL(http_headers.headers[4]->value,  "     \" \"  val\"\"\"ue5 ");
}

/**
 * @brief parse_http_headers() test code. Input need to be failed to parsed.
 * 
 */
void test_parse_http_headers_2() {
    char *http_headers_string = "\"error key :   \"value1 one\"\r\n\r\n";

    struct http_headers http_headers = parse_http_headers(http_headers_string);
    
    CU_ASSERT(http_headers.capacity == 0);
}

/**
 * @brief parse_http_request() test code. Input need to be parsed successfully.
 * Input is GET method and has no body.
 * 
 */
void test_parse_http_request_1() {
    char *http_request =
        "GET /search?q=example&lang=en HTTP/1.1\r\n"
        "Host: www.example.com\r\n"
        "User-Agent: TestClient/1.0\r\n"
        "Accept: text/html\r\n"
        "\r\n";    

    struct http_request request = parse_http_request(http_request);

    CU_ASSERT(request.method    == HTTP_GET);
    CU_ASSERT(request.version   == HTTP_1_1);
    // CU_ASSERT(request.body      == ?);    
    
    CU_ASSERT_STRING_EQUAL(request.path, "/search?q=example&lang=en");
    
    CU_ASSERT_STRING_EQUAL(request.headers.headers[0]->key,     "Host");
    CU_ASSERT_STRING_EQUAL(request.headers.headers[0]->value,   "www.example.com");

    CU_ASSERT_STRING_EQUAL(request.headers.headers[0]->key,     "User-Agent");
    CU_ASSERT_STRING_EQUAL(request.headers.headers[0]->value,   "TestClient/1.0");

    CU_ASSERT_STRING_EQUAL(request.headers.headers[0]->key,     "Accept");
    CU_ASSERT_STRING_EQUAL(request.headers.headers[0]->value,   "text/html");

    CU_ASSERT_STRING_EQUAL(request.query_parameters.parameters[0]->key,     "q");
    CU_ASSERT_STRING_EQUAL(request.query_parameters.parameters[0]->value,   "example");

    CU_ASSERT_STRING_EQUAL(request.query_parameters.parameters[1]->key,     "lang");
    CU_ASSERT_STRING_EQUAL(request.query_parameters.parameters[1]->value,   "en");
}


int main() {
    if (CUE_SUCCESS != CU_initialize_registry()) {
        return CU_get_error();
    }
    CU_pSuite suite = CU_add_suite("Console Test Suite", setup, teardown);
    if (NULL == suite) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    if (NULL == CU_add_test(suite, "test of test_example()", test_example)) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    if (NULL == CU_add_test(suite, "test of parse_http_headers(): valid headers", test_parse_http_headers_1)) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    if (NULL == CU_add_test(suite, "test of parse_http_headers(): invalid headers", test_parse_http_headers_2)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    int failed_cnt = CU_get_number_of_tests_failed();
    CU_cleanup_registry();
    return failed_cnt;
}