#include <ctype.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#include "utility.h"
#include "json.h"

// @TODO Need to meet standard json requirements

static inline int is_hex_digit(char ch) {
    switch (ch) {
    case 'a':    
    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'f':
    case 'A':
    case 'B':
    case 'C':
    case 'D':
    case 'E':
    case 'F':        
        return 1;    
    default:
        return isdigit(ch);
    }
}

void destruct_json_object(struct json_object* json_object) {
    for (int i = 0; i < json_object->size; i++) {
        struct json_element *element = json_object->items[i];
        free(element->key);
        free(element->value);
        free(element);
    }        
    free(json_object->items);
    json_object->capacity = 0;
    json_object->size = 0;
}

optional_t parse_string_token(const char *string_token) {
    /* <string> need to be stated with " */
    if (*string_token != '"') {
        DLOGV("not valud\n");
        return (optional_t) {.stat = 0, .value = NULL};
    }
    int i = 1;
    int len = 1;
    while (string_token[len] != '"' && string_token[len] != '\0') {
        len++;
    }
    /* parse failed */
    if (string_token[len] == '\0') {
        DLOGV("parse failed\n");
        return (optional_t) {.stat = 0, .value = NULL};
    }
    len++;

    /* At least, parsed string is shoter than `len` */
    char *ret = malloc(len);
    int ret_len = 0;

    for (i = 1; i < len - 1; i++) {
        if (string_token[i] == '\\') {
            i++;
            if (string_token[i] == '"') {
                ret[ret_len++] = '"';
            } else if (string_token[i] == '\\') {
                ret[ret_len++] = '\\';
            } else if (string_token[i] == '/') {
                ret[ret_len++] = '/';
            } else if (string_token[i] == 'b') {
                ret[ret_len++] = '\b';
            } else if (string_token[i] == 'f') {
                ret[ret_len++] = '\f';
            } else if (string_token[i] == 'n') {
                ret[ret_len++] = '\n';
            } else if (string_token[i] == 'r') {
                ret[ret_len++] = '\r';
            } else if (string_token[i] == 't') {
                ret[ret_len++] = '\t';
            } else if (string_token[i] == 'u') {
                int is_valid_unicode = 1;
                /* <UTF-16> := u<hex><hex><hex><hex> */

                i++;
                if (!is_hex_digit(string_token[i])) {
                    is_valid_unicode = 0;
                }
                i++;
                if (!is_hex_digit(string_token[i])) {
                    is_valid_unicode = 0;
                }
                i++;
                if (!is_hex_digit(string_token[i])) {
                    is_valid_unicode = 0;
                }
                i++;
                if (!is_hex_digit(string_token[i])) {
                    is_valid_unicode = 0;
                }
                if (!is_valid_unicode) {
                    DLOGV("not valud\n");
                    free(ret);
                    return (optional_t) {.stat = 0, .value = NULL};
                }
                
                // @TODO At now UTF-16 is parsed into whitespace.
                ret[ret_len++] = ' ';
            } else {
                DLOGV("not valud\n");
                    free(ret);
                    return (optional_t) {.stat = 0, .value = NULL};
            }
            continue;
        }

        /* All characters are allowed (different from standard json) */
        ret[ret_len++] = string_token[i];
    }

    ret[ret_len] = '\0';

    return (optional_t) {
        .stat = len,
        .value = (void *)ret
    };
}

optional_t parse_json_element(const char *json_element_string) {
   
    int offset = 0;
    struct json_element *json_element = (struct json_element*)malloc(sizeof(struct json_element));
    char *start_of_key;
    char *start_of_value;

    json_element->key = NULL;
    json_element->value = NULL;

    /* trim prefix */
    start_of_key = find_non_space(json_element_string);  
    offset = (int)(start_of_key - json_element_string);

    optional_t name_str = parse_string_token(start_of_key);
    if (!name_str.stat) {
        goto parse_json_element_error;
    }
    json_element->key = name_str.value;
    offset += name_str.stat;
    
    
    /* trim suffix */
    start_of_value = find_non_space(json_element_string + offset);
    if (!start_of_value || *start_of_value != ':') {
        DLOGV("parse failed");
        goto parse_json_element_error;
    }

    /* trim prefix */
    start_of_value = find_non_space(start_of_value + 1);

    /* step to parse value */
    offset = (int)(start_of_value - json_element_string);

    /* if not <string> */
    if (json_element_string[offset] != '\"') { 
        json_element->value_type = JSON_NUMBER;
        char *end_of_value = start_of_value;
        int length;
        while (*end_of_value && *end_of_value != ',' && *end_of_value != '}')
            end_of_value++;
        offset += (int)(end_of_value - start_of_value);
        end_of_value--;
        /* trim suffix */
        while (is_non_space(*end_of_value)) 
            end_of_value--;
        length = (int)(end_of_value - start_of_value) + 1;
        
        json_element->value = (char*)malloc(length + 1);
        strncpy(json_element->value, start_of_value, length);
        json_element->value[length] = '\0';
    } else {              
        optional_t value_str = parse_string_token(start_of_value);
        if (!value_str.stat) {
            goto parse_json_element_error;
        }
        json_element->value_type = JSON_STRING;
        json_element->value = value_str.value;
        offset += value_str.stat;
    }
 
    return (optional_t) { .stat = offset, .value = (void *)json_element };
    
parse_json_element_error:
    if (json_element->key)
        free(json_element->key);
    if (json_element->value)
        free(json_element->value);
    free(json_element);
    return (optional_t) { .stat = 0, .value = NULL };
}

struct json_object *parse_json(const char *json_string) {    
    if (json_string[0] != '{') {
        return NULL;
    }
    const int INITIAL_CAPACITTY = 8;
    struct json_object *json_object_ret = (struct json_object *)malloc(sizeof(struct json_object));
    json_object_ret->size = 0;
    json_object_ret->capacity = INITIAL_CAPACITTY;
    json_object_ret->items = (struct json_element **)malloc(INITIAL_CAPACITTY * sizeof(struct json_element*));
    
    int offset = 1;
    while (json_string[offset] != '\0') {
        const char *first_non_space = find_non_space(json_string + offset);
        if (first_non_space == NULL) { /* parse failed */
            destruct_json_object(json_object_ret);
            break;
        }     
        
        /* json end */
        if (*first_non_space == '}') {            
            break;
        }

        /* parse failed: key needs to be <string> */
        if (*first_non_space != '"') {
            DLOGV("parse failed\n");
            destruct_json_object(json_object_ret);
            break;
        }

        // get a `<string>:<value>` pair
        optional_t parse_ret = parse_json_element(first_non_space);
        struct json_element *parsed_element = parse_ret.value;
        
        if (parsed_element == NULL) { /* parse failed */
            destruct_json_object(json_object_ret);
            break;
        }

        /* insert parsed_element into json_object::items */
        if (json_object_ret->capacity == json_object_ret->size) {
            int new_capacity = json_object_ret->capacity * 2;
            struct json_element** new_headers = realloc(json_object_ret->items, new_capacity * sizeof(struct json_element*));
            json_object_ret->items = new_headers;
            json_object_ret->capacity = new_capacity;
        }

        json_object_ret->items[json_object_ret->size++] = parsed_element;
        offset = (int)(first_non_space - json_string) + parse_ret.stat;

        offset = (int)(find_non_space(json_string + offset) - json_string);
        /* If no more json name-value pair, then '}' is expected. */
        if (json_string[offset] != ',') {
            if (*find_non_space(json_string + offset) != '}') {
                destruct_json_object(json_object_ret);
                break;
            }
            break;
        }
        // else
        offset++;
    }
        
    if (json_object_ret->capacity == 0) {
        free(json_object_ret);
        return NULL;
    }
    return json_object_ret;
}

struct json_object* insert_json_element(struct json_object *json_object, const char *key, const char *value, enum json_value_type value_type) {
    for (int i = 0; i < json_object->size; i++) {
        if (strcmp(json_object->items[i]->key, key) == 0) {
            return NULL;
        }
    }

    if (json_object->capacity == json_object->size) {
        int new_capacity = json_object->capacity * 2;

        struct json_element** new_headers = realloc(json_object->items, new_capacity * sizeof(struct json_element*));

        json_object->items = new_headers;
        json_object->capacity = new_capacity;            
    }

    struct json_element *element = (struct json_element *)malloc(sizeof(struct json_element));
    if (element == NULL)
        return NULL;

    char *new_key = strdup(key);
    char *new_value = strdup(value);

    if (!new_key || !new_value) {
        if (!new_key)
            free(new_key);
        if (!new_value)
            free(new_value);
        free(element);
        return NULL;
    }
    
    element->value_type = value_type;
    element->key = new_key;
    element->value = new_value;

    json_object->items[json_object->size++] = element;
    
    return json_object;
}

struct json_element* find_json_element(const struct json_object *object, const char *key) {
    for (int i = 0; i < object->size; i++) {
        if (strcmp(object->items[i]->key, key) == 0) 
            return object->items[i];
    }
    return NULL;
}

char *json_object_stringify(const struct json_object *object) {
    char *ret = NULL;
    int ret_len = 0;

    for (int idx = 0; idx < object->size; idx++) {
        const struct json_element *item = object->items[idx];
        for (int i = 0; item->key[i]; i++) {
            /* Excluding UTF-16, characters that require escape processing 
             * should be stored as 2 bytes, including the escape character. 
             */
            switch (item->key[i]) {            
            case '"':
            case '\\':
            case '/':
            case '\b':
            case '\f':
            case '\n':
            case '\r':
            case '\t':            
                ret_len++;
            default:
                ret_len++;
            }
        }
        /* need to wrap with quotation marks */
        ret_len += 2;

        /* need to insert a colon */
        ret_len += 1;
        
        if (item->value_type == JSON_STRING) {
            for (int i = 0; item->value[i]; i++) {
                /* Excluding UTF-16, characters that require escape processing 
                * should be stored as 2 bytes, including the escape character. 
                */
                switch (item->value[i]) {            
                case '"':
                case '\\':
                case '/':
                case '\b':
                case '\f':
                case '\n':
                case '\r':
                case '\t':            
                    ret_len++;
                default:
                    ret_len++;
                }
            }
            /* need to wrap with quotation marks */
            ret_len += 2;
        } else {
            ret_len += strlen(item->value);
        }            
    }

    /* two culry brackets and commas for seperating each name/value pair */
    ret_len += 2 + (object->size - 1);


    /* actual serializing step */
    ret = malloc(ret_len + 1);
    int reti = 0;
    ret[reti++] = '{';
    
    for (int idx = 0; idx < object->size; idx++) {
        const struct json_element *item = object->items[idx];

        /* name */
        ret[reti++] = '"';
        for (int i = 0; item->key[i]; i++) {
            /* if needed, do escaping */
            switch (item->key[i]) {            
            case '"':
                ret[reti++] = '\\';
                ret[reti++] = '"';
                break;
            case '\\':
                ret[reti++] = '\\';
                ret[reti++] = '\\';
                break;
            case '/':
                ret[reti++] = '\\';
                ret[reti++] = '/';
                break;
            case '\b':
                ret[reti++] = '\\';
                ret[reti++] = 'b';
                break;
            case '\f':
                ret[reti++] = '\\';
                ret[reti++] = 'f';
                break;
            case '\n':
                ret[reti++] = '\\';
                ret[reti++] = 'n';
                break;
            case '\r':
                ret[reti++] = '\\';
                ret[reti++] = 'r';
                break;
            case '\t':            
                ret[reti++] = '\\';
                ret[reti++] = 't';
                break;
            default:
                ret[reti++] = item->key[i];
            }            
        }
        ret[reti++] = '"';
        ret[reti++] = ':';

        /* value */
        if (item->value_type == JSON_STRING) {
            ret[reti++] = '"';
            for (int i = 0; item->value[i]; i++) {
                /* if needed, do escaping */
                switch (item->value[i]) {            
                case '"':
                    ret[reti++] = '\\';
                    ret[reti++] = '"';
                    break;
                case '\\':
                    ret[reti++] = '\\';
                    ret[reti++] = '\\';
                    break;
                case '/':
                    ret[reti++] = '\\';
                    ret[reti++] = '/';
                    break;
                case '\b':
                    ret[reti++] = '\\';
                    ret[reti++] = 'b';
                    break;
                case '\f':
                    ret[reti++] = '\\';
                    ret[reti++] = 'f';
                    break;
                case '\n':
                    ret[reti++] = '\\';
                    ret[reti++] = 'n';
                    break;
                case '\r':
                    ret[reti++] = '\\';
                    ret[reti++] = 'r';
                    break;
                case '\t':            
                    ret[reti++] = '\\';
                    ret[reti++] = 't';
                    break;
                default:
                    ret[reti++] = item->value[i];
                }            
            }
            ret[reti++] = '"';
        } else {
            for (int i = 0; item->value[i]; i++) {
                ret[reti++] = item->value[i];
            }
        }

        if (idx < object->size - 1) {
            ret[reti++] = ',';
        }
    }
    ret[reti++] = '}';
    ret[reti] = '\0';

    assert(reti == ret_len);
    return ret;
}