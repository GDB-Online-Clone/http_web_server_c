#define _GNU_SOURCE 
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <dirent.h>
#include <sys/errno.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/times.h>
#include <sys/time.h>
#include <limits.h>
#include <spawn.h>
#include <pthread.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

enum http_status_code;
enum http_method;

struct route;
struct http_header;
struct http_headers;
struct http_query_parameters;
struct http_request;
struct http_response;

/**
 * @brief http status name and its code
 * @note enum value has `int` type
 */
enum http_status_code {
    HTTP_OK = 200,
    HTTP_NOT_FOUND = 404,
    HTTP_BAD_REQUEST = 400,
    HTTP_INTERNAL_SERVER_ERROR = 500,
    HTTP_FORBIDDEN = 403,
    HTTP_UNAUTHORIZED = 401
};

/**
 * @brief http method
 * @note enum value has `int` type
 */
enum http_method {
    HTTP_GET,
    HTTP_POST
};

/**
 * @brief route struct to define each route, mapped by URL path.
 *       Each path doesn't include parameter (such as ?param1=5&param2=6 or /{docs id})
 */
struct route {
   /**
    * @brief Each path doesn't include query parameter (such as ?param1=5&param2=6 or /{docs id})
    */
    char *path;
    /**
     * @brief http request method, such as GET, POST.
     */
    enum http_method method;
    /**
     * @brief callback function performed when http request received
     * 
     */
    struct http_response (*callback)(struct http_request request);
};

/**
 * @brief http_header 
 * @note key-value pair
 */
struct http_header {
    /**
    * @brief key of a header mapped to value
    */
    char *key; 
    /**
     * @brief value of a header mapped by key
     */
    char *value;
};


/**
 * @brief http_headers 
 * @note size is the number of header
 * 
 */
struct http_headers {
    /**
     * @brief size is the number of header
     */
    int size;
    /**
     * @brief headers is the array of http_header
     */
    struct http_header **headers;
};

/**
 * @brief http_query_parameter
 * @note key-value pair
 * 
 */
struct http_query_parameter {
    /**
     * @brief key of a query parameter mapped to value
     */
    char *key;
    /**
    * @brief value of a query parameter mapped by key
    */
    char *value;
};

/**  
 * @brief http_query_parameters
 * @note size is the number of query parameter
*/
struct http_query_parameters {
    /**
     * @brief size is the number of query parameter
     */
    int size;
    /**
     * @brief parameters is the array of http_query_parameter
     */
    struct http_query_parameter **parameters;
};

/**
 * @brief a http request struct
 */
struct http_request {
    /**
     * @brief headers of http reqeust
     */
    struct http_headers headers;
    /**
     * @brief content body of http request. If content body is empty, body is NULL.
     */
    char *body;
    /**
     * @brief path of http request, excluding parameters
     */
    char *path;
    /**
     * @brief query_parameters in URL
     */
    struct http_query_parameters query_parameters;
    /**
     * @brief client_socket returned by "accept(2)"
     */
    int client_socket;
};

/**
* @brief http response struct
*/
struct http_response {
    /**
     * @brief headers of http response
     */
    struct http_headers headers;
    /**
     * @brief status code of http response
     */
    enum http_status_code status_code;
    /**
     * @brief content body of http response. If content body is empty, body is NULL.
     */
    char* body;
    /**
     * @brief client socket returned by "accept(2)"
     */
    int client_socket;
};

/**
 * @brief Build Test 용
 */
int main() {
    printf("Hello World\n");
    return 0;
}