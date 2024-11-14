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

/**
 * @brief Allocates memory and stores a single HTTP query parameter.
 * 
 * This function parses a key and value into a struct http_query_parameter,
 * allocates memory for it, and returns a pointer to the allocated memory.
 * 
 * @param key The key of the query parameter.
 * @param value The value of the query parameter.
 * @return struct http_query_parameter* Pointer to the allocated query parameter.
 */
struct http_query_parameter* insert_query_parameter(char* key, char* value){

    struct http_query_parameter parsed = parse_http_query_parameter(key, value);

    struct http_query_parameter* query_parameter = 
        (struct http_query_parameter*)malloc(sizeof(struct http_query_parameter));
    
    if (!query_parameter) {
        return NULL;
    }

    query_parameter->key = parsed.key;
    query_parameter->value = parsed.value;
    
    return query_parameter;
}


/**
 * @brief Parses an entire query string into multiple query parameters.
 * 
 * This function takes a query string, parses it into individual key-value pairs,
 * and stores them in a struct http_query_parameters. It allocates memory for 
 * each query parameter and returns the struct containing all parameters.
 * 
 * @param parameters_string The query string to be parsed.
 * @return struct http_query_parameters The parsed query parameters.
 */
struct http_query_parameters parse_query_parameters(char* parameters_string){
    
    struct http_query_parameters query_parameters;
    query_parameters.size = 0;
    query_parameters.parameters = 
        (struct http_query_parameter**)malloc(sizeof(struct http_query_parameter*) * 10);
    
    char* token = strtok(parameters_string, "&");
    
    while(token != NULL){
        char* key = strtok(token, "=");
        char* value = strtok(NULL, "=");

        struct http_query_parameter* query_parameter = 
            insert_query_parameter(key, value);
        
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