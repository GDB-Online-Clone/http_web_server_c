#pragma once

#include <stdbool.h>
#include "collections.h"
#include "utility.h"

struct json_element;
struct json_object;

typedef struct json_object json_object_t;
typedef struct array json_array_t;
typedef char* json_number_t;
typedef char* json_string_t;
typedef bool json_boolean_t;
typedef char* json_null_t;

/**
 * @brief A JSON value can be an object, array, number, string, true, false, or null.
 */
enum json_value_type {
    JSON_OBJECT,
    JSON_ARRAY,
    JSON_NUMBER,
    JSON_STRING,
    JSON_BOOLEAN,
    JSON_NULL
};

/**
 * @brief element in json
 * @note key-value pair
 */
struct json_element {
    /**
     * @brief type of value
     */
    enum json_value_type value_type;
    /**
    * @brief key of a json_element to value
    */
    char *key; 
    /**
     * @brief value of a json_element by key
     */
    char *value;
};
/**
 * @brief json_object
 * @note size is the number of json_element
 * 
 */
struct json_object {
    /**
     * @brief size is the number of json_element
     */    
    int size;
    /**
     * @brief capacity of json_elemnts array
     */
    int capacity;
    /**
     * @brief the array of json_elemnts
     */
    struct json_element **items;
};


/**
 * @brief Cleanup `struct json_object` instance.
 * 
 * @param headers target to cleanup
 */
void destruct_json_object(struct json_object* json_object);

/**
 * @brief Parse given string as <string> of json, and then return actual content string.   
 * After closing quotation mark, left chracters are ignored.    
 * ex) `"na"me` is parsed into `na`
 * At now, UTF-16 is parsed as whitespace.   
 * 
 * @param string_token given
 * @return Read length and actual string which was wrapped with quotation marks.
 */
optional_t parse_string_token(const char *string_token);

/**
 * @brief Parse <string>:<value>    
 * At now, parsable value only can be an <number>, <string>, true, false, or null, while cannot be <object> or <array>.   
 * And, it cannot determine if <number>, true, false, or null tokens is malformed.
 * 
 * @param json_element_string `<string>:<value>` string
 * @return Read length and `struct json_element *`.
 * @note <string> is stored as return value from `parse_string_token`.
 */
optional_t parse_json_element(const char *json_element_string);

/**
 * @brief Parse serialized json object
 * 
 * @param json_string serialized json object
 * @return Json obejct as struct alloctaed with malloc. If parsed failed in any situation, return NULL.
 * @retval **NULL** Failed to parse in any situation.
 * @note By default, this function meets the standards in that document.   
 * https://www.ecma-international.org/wp-content/uploads/ECMA-404_2nd_edition_december_2017.pdf
 * @warning Some values are not permitted even though it meets json grammer. Note `parse_json_element()`
 */
struct json_object *parse_json(const char *json_string);

/**
 * @brief Insert new json name/value pair into `json_object`.
 * 
 * @param json_object a `struct json_object` where wants to store given name(key)/value pair.
 * @param key a key of new json element.
 * @param value a value of new json element.
 * @param value_type type of value to insert
 * @return The `struct json_object` given as json_object. In any situation, failing to store new element, return **NULL**.
 * @note Internally, this function uses `malloc` and `strcpy` to store `key` and `value` into new element. (Actually, uses `strdup`)
 */
struct json_object* insert_json_element(struct json_object *json_object, const char *key, const char *value, enum json_value_type value_type);

/**
 * @brief Find a name/value pair having same `key` in `object`.
 * 
 * @param object json object to search for.
 * @param key name to find
 * @return `json_element` matched by `key`. Returns **NULL** if not found.
 * @retval **NULL** Not found
 */
struct json_element* find_json_element(const struct json_object *object, const char *key);

/**
 * @brief Serialize json object and return string.
 * 
 * @param object json object to serialize
 * @return serialized json object
 * @note if there is value as json object, you need to make it serialized string before call this.
 */
char *json_object_stringify(const struct json_object *object);