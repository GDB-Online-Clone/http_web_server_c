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

    struct web_server app = (struct web_server) {
        .route_table = &route_table,
        .port_num = 10010,
        .backlog = 10
    };
  
    run_web_server(app);
    return 0;
}

#pragma GCC diagnostic pop