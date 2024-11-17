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
enum http_version; 

// GET /index.html HTTP/1.1\r\nHost: localhost:4221\r\nUser-Agent: curl/7.64.1\r\nAccept: */*\r\n\r\n

struct route;
struct http_header;
struct http_headers;
struct http_query_parameters;
struct http_request;
struct http_response;

int init_http_request(
    struct http_request             *request,
    struct http_headers             headers,
    enum http_method                method,
    enum http_version               version,
    char                            *body,
    char                            *path,
    struct http_query_parameters    query_parameters
);

/**
 * @brief Parse an HTTP request string into a struct http_request.
 */
struct http_request parse_http_request(char *request);

/**
 * @brief Parse the HTTP method string and return its enum representation.
 */
enum http_method parse_http_method(const char *method);

/**
 * @brief Parse the HTTP version string and return its enum representation.
 */
enum http_version parse_http_version(const char *version);


/**
 * @brief Cleanup `struct http_headers` instance.
 * 
 * @param headers target to cleanup
 */
void destruct_http_headers(struct http_headers *headers);

/**
 * @brief Parses the key and value from a substring of HTTP headers and returns a struct http_header*.
 * If a non-NULL address is provided as input for http_header, 
 * the parsed key and value are stored at the location pointed to by the address; otherwise, they are stored using **malloc**.
 * 
 * @param http_header Pointer to a struct http_header where key and value will be stored. If **NULL**, it will be created internally using **malloc**.
 * @param header_string An HTTP header string to parse, separated by CRLF
 * @return Returns the address of the variable where the header is stored if parsing is successful. Returns **NULL** if it fails.
 * @retval 
 * @note If you're unsure what this does, use `parse_http_headers` instead.
 * @warning Don't pass null-terminated string.
 */
struct http_header* parse_http_header(
	struct http_header	*http_header,
	char				*header_string
);

/**
 * @brief Parse HTTP headers string and build a `struct http_headers` instance.
 * 
 * @param headers_string HTTP headers as string, ended by CRLF
 * @return Constructed `struct http_headers` instance. If `struct http_headers::capacity` is **zero**, it means parsing **failed**.
 */
struct http_headers parse_http_headers(char* headers_string);

/**
 * @brief Insert new header into `headers`.
 * 
 * @param headers a `struct http_headers` where wants to store given header having 'key' and 'value'.
 * @param key a key of new header.
 * @param value a value of new header.
 * @return The `struct http_headers` given as headers. In any situation, failing to store new header, return **NULL**.
 * @note Internally, this function uses `malloc` and `strcpy` to store `key` and `value` into new header. (Actually, uses `strdup`)
 */
struct http_headers* insert_header(struct http_headers *headers, char* key, char* value);

struct http_header* find_header(char* key);

// 쿼리 파라미터
struct http_query_parameter parse_http_query_parameter(
	char		*key,
	char		*value
);
struct http_query_parameter* insert_qurey_parameter(char* key, char* value);
struct http_query_parameter* find_query_parameter(char* key);
struct http_query_parameters parse_query_parameters(char* parameters_string);
void free_query_parameters(struct http_query_parameters* query_parameters);


char* http_response_stringfy(struct http_response http_response);

/* http 웹서버 라이브러리 구조체 */

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
    HTTP_POST,
    HTTP_METHOD_UNKNOWN, // Unknown method handling
};

/**
 * @brief http version
 * @note enum value has `int` type
 */
enum http_version {
    HTTP_1_0, // HTTP/1.0
    HTTP_1_1, // HTTP/1.1
    HTTP_2_0, // HTTP/2.0
    HTTP_3_0, // HTTP/3.0
    HTTP_VERSION_UNKNOWN // Unknown version handling
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
     * @brief capacity of headers array
     */
    int capacity;

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
     * @brief http request method, such as GET, POST.
     */
    enum http_method method;
    /**
     * @brief http version.
     */
    enum http_version version;
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