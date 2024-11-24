#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <webserver/http.h>
#include <webserver/utility.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUF_SIZE 1024

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
 * @brief test for `find_header()`
 * 
 */
void test_find_header() {
    struct http_headers headers = (struct http_headers) {
        .capacity = 8,
        .items = (struct http_header **)malloc(8 * sizeof(int *)),
        .size = 0
    };

    insert_header(&headers, "k1", "v1");
    insert_header(&headers, "k2", "v2");
    insert_header(&headers, "k3", "v3");
    insert_header(&headers, "k4", "v4");

    CU_ASSERT_STRING_EQUAL(find_header(&headers, "k1")->value, "v1");
    CU_ASSERT_STRING_EQUAL(find_header(&headers, "k4")->value, "v4");
    CU_ASSERT(find_header(&headers, "k0") == NULL);
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

  
/**
 * @brief Test case for handling multiple concurrent client connections
 */
void test_run_web_server_multiple_clients(void) {
    // Set up test server configuration
    struct route_table route_table;
    route_table.size = 1;
    route_table.routes = malloc(sizeof(struct route));
    route_table.routes[0].path = strdup("/test");
    route_table.routes[0].method = HTTP_GET;
    route_table.routes[0].callback = test_callback;

    struct web_server server = {
        .route_table = &route_table,
        .port_num = 9001
    };

    // Start server in separate thread
    pthread_t server_thread_id;
    pthread_create(&server_thread_id, NULL, server_thread, (void*)&server);
    
    // Allow server to start
    sleep(1);

    // Test multiple concurrent clients
    #define NUM_CLIENTS 3
    pthread_t client_threads[NUM_CLIENTS];
    int client_results[NUM_CLIENTS] = {0};

    for(int i = 0; i < NUM_CLIENTS; i++) {
        int* result = &client_results[i];
        pthread_create(&client_threads[i], NULL, client_thread, (void*)result);
    }

    // Wait for all clients to complete
    for(int i = 0; i < NUM_CLIENTS; i++) {
        pthread_join(client_threads[i], NULL);
        CU_ASSERT_EQUAL(client_results[i], 1); // Check each client succeeded
    }

    // Cleanup
    pthread_cancel(server_thread_id);
    pthread_join(server_thread_id, NULL);
    
    free(route_table.routes[0].path);
    free(route_table.routes);
}

/**
 * @brief Test case for handling invalid requests
 */
void test_run_web_server_invalid_request(void) {
    // Set up test server configuration
    struct route_table route_table;
    route_table.size = 1;
    route_table.routes = malloc(sizeof(struct route));
    route_table.routes[0].path = strdup("/test");
    route_table.routes[0].method = HTTP_GET;
    route_table.routes[0].callback = test_callback;

    struct web_server server = {
        .route_table = &route_table,
        .port_num = 9002
    };

    // Start server in separate thread
    pthread_t server_thread_id;
    pthread_create(&server_thread_id, NULL, server_thread, (void*)&server);
    
    // Allow server to start
    sleep(1);

    // Create client socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    CU_ASSERT(sock >= 0);

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(9002);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    int conn_result = connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    CU_ASSERT(conn_result >= 0);

    // Send invalid HTTP request
    char* invalid_request = "INVALID REQUEST\r\n\r\n";
    send(sock, invalid_request, strlen(invalid_request), 0);

    // Receive response
    char buffer[BUF_SIZE] = {0};
    int valread = recv(sock, buffer, BUF_SIZE, 0);
    CU_ASSERT(valread > 0);

    // Check for error response
    CU_ASSERT(strstr(buffer, "400 Bad Request") != NULL);

    // Cleanup
    close(sock);
    pthread_cancel(server_thread_id);
    pthread_join(server_thread_id, NULL);
    
    free(route_table.routes[0].path);
    free(route_table.routes);
}

/**
 * @brief Helper function to simulate client behavior in a separate thread
 * @param arg Pointer to result storage
 * @return NULL
 */
void* client_thread(void* arg) {
    int* result = (int*)arg;
    *result = 0;
    
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0) return NULL;

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(9001);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        close(sock);
        return NULL;
    }

    // Send valid HTTP request
    char* request = "GET /test HTTP/1.1\r\nHost: localhost\r\n\r\n";
    send(sock, request, strlen(request), 0);

    // Receive response
    char buffer[BUF_SIZE] = {0};
    if(recv(sock, buffer, BUF_SIZE, 0) > 0) {
        if(strstr(buffer, "200 OK") != NULL) {
            *result = 1; // Success
        }
    }

    close(sock);
    return NULL;
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
    if (NULL == CU_add_test(suite, "test of find_header", test_find_header)) {
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

    if (NULL == CU_add_test(suite, "test multiple clients", test_run_web_server_multiple_clients)) {
    CU_cleanup_registry();
    return CU_get_error();
    }
    
    if (NULL == CU_add_test(suite, "test invalid request", test_run_web_server_invalid_request)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    int failed_cnt = CU_get_number_of_tests_failed();
    CU_cleanup_registry();
    return failed_cnt;
}