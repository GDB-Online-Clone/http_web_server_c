#include "http.h"
#include "utility.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-function"

/**
 * @brief Build Test 용
 * 
 * @return int 프로그램 종료 상태
 */
int main() {
    struct routes route_table = {};
    init_routes(&route_table);

    struct web_server app = (struct web_server) {
        .route_table = &route_table,
        .port_num = 10010,
        .backlog = 10
    };
  
    run_web_server(app);
    return 0;
}

#pragma GCC diagnostic pop