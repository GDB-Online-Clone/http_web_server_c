struct json_element;
struct json_object;


/**
 * @brief element in json
 * @note key-value pair
 */
struct json_element {
    /**
     * @brief the flag represents if it is <string>
     */
    int is_string;
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

