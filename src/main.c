#include "http.h"
#include "utility.h"

struct http_header* insert_header(char *key, char *value);


void destruct_http_headers(struct http_headers *headers) {
    for (int i = 0; i < headers->size; i++) {
        struct http_header *parsed_header = headers->headers[i];
        free(parsed_header->key);
        free(parsed_header->value);
        free(parsed_header);
    }        
    free(headers->headers);
    headers->capacity = 0;
    headers->size = 0;
}

struct http_header *parse_http_header(struct http_header *http_header, char *header_string) {

    char *end_of_header = strstr(header_string, "\r\n");
    int offset = 0;
    struct http_header *header;
    char *start_of_key;
    char *start_of_value;

    if (!http_header) {
        header = (struct http_header*)malloc(sizeof(struct http_header));
    } else {
        header = http_header;
    }
    header->key = NULL;
    header->value = NULL;

    /* trim prefix */
    start_of_key = find_non_space(header_string);  

    offset = (int)(start_of_key - header_string);

    /* if not quoted string */
    if (header_string[offset] != '\"') {
        char *end_of_key = start_of_key;
        int length;

        while (end_of_key != end_of_header && *end_of_key != ':')
            end_of_key++;
        if (end_of_key == end_of_header)
            goto parse_header_error;

        offset = (int)(end_of_key - start_of_key) + 1;

        end_of_key--;
        /* trim suffix */
        while (is_non_space(*end_of_key)) 
            end_of_key--;

        length = (int)(end_of_key - start_of_key) + 1;
        
        header->key = (char*)malloc(length + 1);
        strncpy(header->key, start_of_key, length);

    } else {
        /* Case of quoted string. now we cannnot perform like above, because ':' can be inside of key name. */
        /* At now, our target is to find ` ": `, double quotes followed by ':'. */
        
        char *end_of_key = start_of_key + 1;
        int length;
        /* find first non-whitespace except for double quotes at start */
        char *last_non_space_at = find_non_space(start_of_key + 1);
        

        end_of_key = last_non_space_at + 1;

        for (; end_of_key != end_of_header && (*end_of_key != ':' || *last_non_space_at != '"'); end_of_key++) {
            if (!is_non_space(*end_of_key))
                last_non_space_at = end_of_key;
        }
        
        if (end_of_key == end_of_header) {
            goto parse_header_error;
        }

        offset = (int)(end_of_key - start_of_key) + 1;        
        end_of_key = last_non_space_at - 1;
        start_of_key++;

        length = (int)(end_of_key - start_of_key) + 1;
        
        header->key = (char*)malloc(length + 1);
        strncpy(header->key, start_of_key, length);
    }
    

    /* step to parse value */

    /* trim prefix */
    start_of_value = find_non_space(header_string + offset);  

    offset = (int)(start_of_value - header_string);

    /* if not quoted string */
    if (header_string[offset] != '\"') {
        char *end_of_value = start_of_value;
        int length;

        while (end_of_value != end_of_header)
            end_of_value++;

        offset = (int)(end_of_value - start_of_value) + 1;

        end_of_value--;
        /* trim suffix */
        while (is_non_space(*end_of_value)) 
            end_of_value--;

        length = (int)(end_of_value - start_of_value) + 1;
        
        header->value = (char*)malloc(length + 1);
        strncpy(header->value, start_of_value, length);

    } else {
        /* Case of quoted string. now we cannnot perform like above, because ':' can be inside of value string. */
        /* At now, our target is to find ` "<CRLF> `, double quotes followed by "\r\n". */
        
        char *end_of_value = start_of_value + 1;
        int length;
        /* find first non-whitespace except for double quotes at start */
        char *last_non_space_at = find_non_space(start_of_value + 1);
        

        end_of_value = last_non_space_at + 1;

        for (; end_of_value != end_of_header; end_of_value++) {
            if (!is_non_space(*end_of_value))
                last_non_space_at = end_of_value;
        }
        
        if (*last_non_space_at != '"') {
            goto parse_header_error;
        }

        offset = (int)(end_of_value - start_of_value) + 1;
        end_of_value = last_non_space_at - 1;
        start_of_value++;

        length = (int)(end_of_value - start_of_value) + 1;
        
        header->value = (char*)malloc(length + 1);
        strncpy(header->value, start_of_value, length);
    }
 
    return header;

parse_header_error:
    if (header->key)
        free(header->key);

    if (http_header == NULL) {
        free(header);
    }
    return NULL;
}

struct http_headers parse_http_headers(char *headers_string) {    

    const int INITIAL_CAPACITTY = 8;

    struct http_headers headers_ret = {
        .size = 0,
        .capacity = INITIAL_CAPACITTY,
        .headers = malloc(INITIAL_CAPACITTY * sizeof(struct http_header))
    };
    
    int offset = 0;
    while (headers_string[offset] != '\0') {
        char *CRLF_pointer = strstr(headers_string + offset, "\r\n");

        
        if (CRLF_pointer == NULL) { /* parse failed */
            destruct_http_headers(&headers_ret);
            break;
        } else if (CRLF_pointer == (headers_string + offset)) { /* (normal break condition) there is no more header to parse */
            break;
        }
        

        struct http_header *parsed_header = parse_http_header(NULL, headers_string + offset);
        

        if (parsed_header == NULL) { /* parse failed */
            destruct_http_headers(&headers_ret);
            break;
        }

        /* insert parsed_header into http_headers::headers */

        if (headers_ret.capacity == headers_ret.size) {
            int new_capacity = headers_ret.capacity * 2;

            struct http_header** new_headers = realloc(headers_ret.headers, new_capacity * sizeof(struct http_header*));

            headers_ret.headers = new_headers;
            headers_ret.capacity = new_capacity;            
        }

        headers_ret.headers[headers_ret.size++] = parsed_header;
        offset = (int)(CRLF_pointer - headers_string) + 2;
    }
        
    return headers_ret;
}



/**
 * @brief Build Test 용
 */
int main() {
    printf("Hello World\n");
    return 0;
}