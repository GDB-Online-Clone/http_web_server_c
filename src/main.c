#include "http.h"

struct http_query_parameter* insert_qurey_parameter(char* key, char* value){
    
}

/**
 * @brief Parses URL query parameters string into a structured format
 * @param parameters_string The raw query parameter string (format: "key1=value1&key2=value2...")
 * @return struct http_query_parameters A structure containing all parsed query parameters
 * 
 * @details This function takes a URL query parameter string and breaks it down into individual
 * key-value pairs. The string should be in the format "key1=value1&key2=value2&...".
 * Each pair is separated by '&' and each key and value within a pair is separated by '='.
 * 
 * The function:
 * - Allocates memory for up to 10 query parameters
 * - Splits the string on '&' characters to separate parameters
 * - For each parameter, splits on '=' to separate key and value
 * - Creates a new query_parameter structure for each pair
 * - Stores all parameters in the returned http_query_parameters structure
 * 
 * @warning The caller is responsible for freeing the allocated memory
 * @note The maximum number of parameters is limited to 10
 */
struct http_query_parameters parse_qurey_parameters(char* parameters_string){
    
    struct http_query_parameters query_parameters;
    query_parameters.size = 0;
    query_parameters.parameters = (struct http_query_parameter**)malloc(sizeof(struct http_query_parameter*) * 10);
    
    char* token = strtok(parameters_string, "&");
    
    while(token != NULL){
        char* key = strtok(token, "=");
        char* value = strtok(NULL, "=");

        struct http_query_parameter* query_parameter = insert_qurey_parameter(key, value);
        
        query_parameters.parameters[query_parameters.size++] = query_parameter;
        token = strtok(NULL, "&");
    }
    return query_parameters;
}

/**
 * @brief Build Test 용
 * 
 * @return int 프로그램 종료 상태
 */
int main() {
    printf("Hello World\n");
    return 0;
}