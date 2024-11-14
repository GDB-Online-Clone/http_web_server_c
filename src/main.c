#include "http.h"

/**
 * @brief Parses a single HTTP query parameter.
 * 
 * This function takes a key and a value as input and returns a 
 * struct http_query_parameter containing the parsed key and value.
 * 
 * @param key The key of the query parameter.
 * @param value The value of the query parameter.
 * @return struct http_query_parameter The parsed query parameter.
 */
struct http_query_parameter parse_http_query_parameter(char *key, char *value){

    struct http_query_parameter query_parameter;
    
    query_parameter.key = key;
    query_parameter.value = value;
    
    return query_parameter;
}
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
 * @brief Frees the memory allocated for an http_query_parameters structure.
 * 
 * @param query_parameters The http_query_parameters structure to free.
 */
void free_query_parameters(struct http_query_parameters* query_parameters){
    for (int i = 0; i < query_parameters->size; i++) {
        free(query_parameters->parameters[i]);
    }
    free(query_parameters->parameters);
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