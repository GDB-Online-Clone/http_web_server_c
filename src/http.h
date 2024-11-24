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
struct http_request parse_http_request(const char *request);

/**
 * @brief Parse the HTTP method string and return its enum representation.
 */
enum http_method parse_http_method(const char *method);

/**
 * @brief Convert the HTTP method enum to its corresponding string representation.
 */
char* http_method_stringify(const enum http_method method);

/**
 * @brief Parse the HTTP version string and return its enum representation.
 */
enum http_version parse_http_version(const char *version);

/**
 * @brief Convert an HTTP version enum to its string representation.
 */
char* http_version_stringify(const enum http_version version);

/**
 * @brief Converts an HTTP status code to its reason phrase.
 */
char* http_status_code_stringfy(const enum http_status_code code);

/**
 * @brief Cleanup `struct http_headers` instance.
 * 
 * @param headers target to cleanup
 */
void destruct_http_headers(struct http_headers *headers);

/**
 * @brief Parses the key and value from a substring of HTTP headers and returns a struct http_header*, allocated with malloc.
 * 
 * @param header_string An HTTP header string to parse, separated by CRLF
 * @return Returns the address of the variable where the header is stored if parsing is successful. Returns **NULL** if it fails.
 * @note If you're unsure what this does, use `parse_http_headers` instead.
 * @warning Don't pass null-terminated string.
 */
struct http_header* parse_http_header(char *header_string);

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

/**
 * @brief Parses a query parameter string into key-value pair
 *
 * @details This function takes a parameter string in the format "key=value" and splits it
 *          into separate key and value components. The function creates copies of both
 *          the key and value strings using strdup().
 *
 * @param parameter_string String containing the parameter in "key=value" format
 *
 * @return http_query_parameter structure containing:
 *         - key: Pointer to allocated string containing the parameter key
 *         - value: Pointer to allocated string containing the parameter value
 *
 * @retval Returns structure with NULL pointers if:
 *         - parameter_string is NULL
 *         - Memory allocation fails
 *         - No '=' separator is found
 *
 * @note 
 * - Uses strtok_r() for thread-safe string tokenization
 * - Creates new memory allocations for both key and value
 *
 * @warning
 * - Caller is responsible for freeing the memory of both key and value
 * - Assumes parameter_string format is "key=value"
 */
struct http_query_parameter parse_http_query_parameter(
	char		*parameter_string
);

/**
 * @brief Parses and inserts a query parameter into the parameters list
 *
 * @details This function parses the input parameter string into a key-value pair
 *          and adds it to the parameters array in the query_parameters structure.
 *          The structure can store up to a maximum of 10 parameters.
 *
 * @param query_parameters Pointer to the structure storing query parameters
 * @param parameter_string Query parameter string to be parsed
 *
 * @return Pointer to the updated query_parameters structure on success,
 *         NULL on failure
 *
 * @retval NULL Returned in following cases:
 *              - If query_parameters is NULL
 *              - If parameter_string is NULL
 *              - If number of parameters is already 10 or more
 *              - If parameter parsing fails
 *              - If memory allocation fails
 *
 * @note Parameter parsing is performed using parse_http_query_parameter() function
 * @warning Memory allocated within the returned structure must be freed after use
 */
struct http_query_parameters* insert_query_parameter(struct http_query_parameters *query_parameters, char* parameter_string);

/**
 * @brief Parse the query parameters string and return a struct http_query_parameters.
 * 
 * This function parses the query parameters string and returns a struct http_query_parameters
 * containing an array of struct http_query_parameter. The query parameters string should be
 * in the format "key1=value1&key2=value2&key3=value3".
 * 
 * @param parameters_string The query parameters string to be parsed.
 * @return struct http_query_parameters The parsed query parameters.
 */
struct http_query_parameters parse_query_parameters(char* parameters_string);

struct http_query_parameter* find_query_parameter(char* key);

/**
* @brief Deallocates all memory associated with query parameters
*
* @details This function performs a complete cleanup of the query_parameters structure:
*          - Frees memory for each parameter's key and value strings
*          - Frees memory for each parameter structure
*          - Frees the parameters array
*          - Resets size to 0
*          The function handles NULL pointers safely at each level.
*
* @param query_parameters Pointer to the query_parameters structure to be freed
*
* @note
* - Safely handles NULL pointers at all levels of the structure
* - Sets the parameters pointer to NULL after freeing
* - Resets the size counter to 0
* - Does not free the query_parameters structure itself
*
* @warning
* - The query_parameters structure itself is not freed by this function
* - After this function call, query_parameters->parameters will be NULL
* - Assumes that query_parameters->size accurately reflects the number of allocated parameters
*
* Memory deallocation order:
* 1. Individual key and value strings within each parameter
* 2. Parameter structures
* 3. Parameters array
*/
void free_query_parameters(struct http_query_parameters* query_parameters);


char* http_response_stringify(struct http_response http_response);

/* http 웹서버 라이브러리 구조체 */

/**
 * @brief http status name and its code
 * @note enum value has `int` type
 */
enum http_status_code {
    // 2xx Success
    HTTP_OK = 200,
    HTTP_CREATED = 201,
    HTTP_ACCEPTED = 202,
    HTTP_NO_CONTENT = 204,

    // 3xx Redirection
    HTTP_MOVED_PERMANENTLY = 301,
    HTTP_FOUND = 302,
    HTTP_NOT_MODIFIED = 304,
    HTTP_TEMPORARY_REDIRECT = 307,
    HTTP_PERMANENT_REDIRECT = 308,

    // 4xx Client Errors
    HTTP_BAD_REQUEST = 400,
    HTTP_UNAUTHORIZED = 401,
    HTTP_FORBIDDEN = 403,
    HTTP_NOT_FOUND = 404,
    HTTP_METHOD_NOT_ALLOWED = 405,
    HTTP_REQUEST_TIMEOUT = 408,
    HTTP_CONFLICT = 409,
    HTTP_GONE = 410,
    HTTP_LENGTH_REQUIRED = 411,
    HTTP_PAYLOAD_TOO_LARGE = 413,
    HTTP_URI_TOO_LONG = 414,
    HTTP_UNSUPPORTED_MEDIA_TYPE = 415,
    HTTP_TOO_MANY_REQUESTS = 429,

    // 5xx Server Errors
    HTTP_INTERNAL_SERVER_ERROR = 500,
    HTTP_NOT_IMPLEMENTED = 501,
    HTTP_BAD_GATEWAY = 502,
    HTTP_SERVICE_UNAVAILABLE = 503,
    HTTP_GATEWAY_TIMEOUT = 504,
    HTTP_HTTP_VERSION_NOT_SUPPORTED = 505
};

/**
 * @brief http method
 * @note enum value has `int` type
 */
enum http_method {
    HTTP_GET,
    HTTP_POST,
    HTTP_PUT,
    HTTP_DELETE,
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
     * @brief the array of http_header
     */
    struct http_header **items;
};

/**
 * @brief http_query_parameter
 * @note key-value pair
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
     * @brief items is the array of http_query_parameter
     */
    struct http_query_parameter **items;
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
    struct http_headers     headers;
    /**
     * @brief status code of http response
     */
    enum http_status_code   status_code;
    /**
     * @brief http protocol version
     */
    enum http_version       http_version;
    /**
     * @brief content body of http response. If content body is empty, body is NULL.
     */
    char* body;
};