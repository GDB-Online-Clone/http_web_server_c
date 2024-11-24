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
    
    CU_ASSERT_STRING_EQUAL(http_headers.items[0]->key,    "no_quotes");
    CU_ASSERT_STRING_EQUAL(http_headers.items[0]->value,  "value quotes with whitespace");

    CU_ASSERT_STRING_EQUAL(http_headers.items[1]->key,    "key quotes with whitespace");
    CU_ASSERT_STRING_EQUAL(http_headers.items[1]->value,  "value no quotes but whitespace");

    CU_ASSERT_STRING_EQUAL(http_headers.items[2]->key,    "quote with colon : ");
    CU_ASSERT_STRING_EQUAL(http_headers.items[2]->value,  "trimmed_value");

    CU_ASSERT_STRING_EQUAL(http_headers.items[3]->key,    "quote; \" is in key");
    CU_ASSERT_STRING_EQUAL(http_headers.items[3]->value,  "value4");

    CU_ASSERT_STRING_EQUAL(http_headers.items[4]->key,    "start_with_whitespace");
    CU_ASSERT_STRING_EQUAL(http_headers.items[4]->value,  "     \" \"  val\"\"\"ue5 ");
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
    
    CU_ASSERT_STRING_EQUAL(request.headers.items[0]->key,     "Host");
    CU_ASSERT_STRING_EQUAL(request.headers.items[0]->value,   "www.example.com");

    CU_ASSERT_STRING_EQUAL(request.headers.items[0]->key,     "User-Agent");
    CU_ASSERT_STRING_EQUAL(request.headers.items[0]->value,   "TestClient/1.0");

    CU_ASSERT_STRING_EQUAL(request.headers.items[0]->key,     "Accept");
    CU_ASSERT_STRING_EQUAL(request.headers.items[0]->value,   "text/html");

    CU_ASSERT_STRING_EQUAL(request.query_parameters.items[0]->key,     "q");
    CU_ASSERT_STRING_EQUAL(request.query_parameters.items[0]->value,   "example");

    CU_ASSERT_STRING_EQUAL(request.query_parameters.items[1]->key,     "lang");
    CU_ASSERT_STRING_EQUAL(request.query_parameters.items[1]->value,   "en");
}

/**
 * @brief Test for `init_routes`. Test that members of routes are correctly set and allocated.
 */
void test_init_routes_1() {
    struct routes routes;
    routes.items = NULL;
    init_routes(&routes);
    CU_ASSERT(routes.size == 0);
    CU_ASSERT(routes.capacity > 0);
    /* Segment faults Test (or test some other faults) */    
    CU_ASSERT(routes.items != NULL);
    free(routes.items);
}

void test_url_path_cmp() {
    char *path = "/path/to/api";
    char *same_path1 = "/path/to/api/";
    char *same_path2 = "/path/to/api";
    char *diff_path = "/path/to/api1";

    CU_ASSERT(url_path_cmp(path, same_path1) == 0);
    CU_ASSERT(url_path_cmp(path, same_path2) == 0);
    CU_ASSERT(url_path_cmp(path, diff_path) != 0);
}

struct http_response empty_callback(struct http_request request) {
    struct http_headers headers = {};
    return (struct http_response) {
        .body = "hello",
        .headers = headers,
        .http_version = HTTP_1_0,
        .status_code = HTTP_OK
    };
}

/**
 * @brief Test for `insert_route`. Test that `routes` successfully stores the arguments which passed. 
 *      This also contains a stress test for testing realloc.
 * @warning **[Dependency of tests]**
 * `init_routes`
 * `parse_http_request`
 */
void test_insert_route() {
    struct routes routes;    
    char *http_request_string = 
            "GET /path/to/api HTTP/1.1\r\n"
            "Host: www.example.com\r\n"
            "\r\n";
    struct http_request request_temp = parse_http_request(http_request_string);

    init_routes(&routes);
    insert_route(&routes, "/path/to/api", HTTP_GET, empty_callback);
    CU_ASSERT_STRING_EQUAL(routes.items[0]->path, "/path/to/api");
    CU_ASSERT(routes.size == 1);    
    CU_ASSERT_STRING_EQUAL(routes.items[0]->callback(request_temp).body, "hello");
    CU_ASSERT(routes.items[0]->method == HTTP_GET);

    insert_route(&routes, "/path/to/api", HTTP_GET, empty_callback);
    CU_ASSERT(routes.size == 1);

    for (int i = 1; i < 100; i++) {
        char path[64];
        sprintf(path, "/path/to/api%d", i);
        insert_route(&routes, path, HTTP_GET, empty_callback);
        CU_ASSERT_STRING_EQUAL(routes.items[i]->path, path);
        CU_ASSERT(routes.size == i + 1);    
        CU_ASSERT(routes.size <= routes.capacity); 
        CU_ASSERT_STRING_EQUAL(routes.items[i]->callback(request_temp).body, "hello");
        CU_ASSERT(routes.items[i]->method == HTTP_GET);
    }
}

/**
 * @brief Test for `test_find_route`.
 * @warning **[Dependency of tests]**   
 * `init_routes`   
 * `insert_route`   
 * `url_path_cmp`   
 */
void test_find_route() {
    struct routes routes;    

    init_routes(&routes);
    insert_route(&routes, "/path/to/api", HTTP_GET, empty_callback);
    insert_route(&routes, "/path/to/api", HTTP_GET, empty_callback);

    for (int i = 1; i < 5; i++) {
        char path[64];
        sprintf(path, "/path/to/api%d", i);
        insert_route(&routes, path, HTTP_GET, empty_callback);
    }

    CU_ASSERT(find_route(&routes, "/path/to/api0", HTTP_GET) == 0);
    CU_ASSERT(find_route(&routes, "/path/to/api1", HTTP_GET) != 0);
    CU_ASSERT(find_route(&routes, "/path/to/api1/", HTTP_GET) != 0);
    CU_ASSERT(find_route(&routes, "/path/to/api1/", HTTP_POST) == 0);
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
    if (NULL == CU_add_test(suite, "test of init_routes", test_init_routes_1)) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    if (NULL == CU_add_test(suite, "test of url_path_cmp", test_url_path_cmp)) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    if (NULL == CU_add_test(suite, "test of insert_route", test_insert_route)) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    if (NULL == CU_add_test(suite, "test of find_route", test_find_route)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    int failed_cnt = CU_get_number_of_tests_failed();
    CU_cleanup_registry();
    return failed_cnt;
}