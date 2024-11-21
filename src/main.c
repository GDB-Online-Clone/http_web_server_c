#include "http.h"
#include "utility.h"


/**
 * @brief Build Test 용
 * 
 * @return int 프로그램 종료 상태
 */
int main() {
      // HTTP 요청 파싱
    char *http_request =
        "GET /search?q=example&lang=en HTTP/1.1\r\n"
        "Host: www.example.com\r\n"
        "User-Agent: TestClient/1.0\r\n"
        "Accept: text/html\r\n"
        "\r\n";    

    struct http_request request = parse_http_request(http_request);
    
    
    DLOG("[headers]\n");
    for (int i = 0; i < request.headers.size; i++) {
        DLOG("%s: %s\n", request.headers.items[i]->key, request.headers.items[i]->value);        
    }
    DLOG("[method]\n%d\n", request.method);
    DLOG("[version]\n%d\n", request.version);
    DLOG("[body]\n%s\n", request.body);
    DLOG("[path]\n%s\n", request.path);

     DLOG("[query parameters]\n");
    for (int i = 0; i < request.query_parameters.size; i++) {
        DLOG("%s: %s\n", request.query_parameters.items[i]->key, request.query_parameters.items[i]->value);        
    }
    return 0;
}