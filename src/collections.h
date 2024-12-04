#pragma once

/**
 * @brief Arr_ptr needs to be `struct array *`
 */
#define item_of(Arr_ptr, idx, item_type) (((item_type *)(Arr_ptr))->items[idx])

/**
 * @brief Arr_ptr needs to be `struct array *`   
 * It is equivalent in semantics to `&items[size]`
 */
#define item_ptr_of(Arr_ptr, idx) ((char *)((Arr_ptr)->items) + (idx) * (Arr_ptr)->m_size)

/**
 * @brief Sequential Array Struct
 */
struct array {
    /**
     * @brief the number of items
     */
    int size;
    /**
     * @brief allocated size of items array
     */
    int capacity;
    /**
     * @brief byte size of each item
     */
    int m_size;
    /**
     * @brief items array
     */
    void *items;    
    /**
     * @brief cleanup function for each item. DO NOT free passed argument itself.
     * @note destructor passes `&array::items[idx]` as argument.
     */
    void (*cleanup_handler)(void *);
};

/**
 * @brief Insert new json name/value pair into `target`.
 * 
 * @param target a `struct array *` to init.
 * @param m_size byte size of each item
 * @param cleanup_handler cleanup function for each element. Note `struct array::cleanup_handler`
 * @return always return `target`
 */
struct array* init_array(struct array *target, int m_size, void (*cleanup_handler)(void *));

/**
 * @brief Reserve memory for items.
 * 
 * @param target a `struct array *`
 * @param new_capacity new capacity of items
 * @return always return `target`
 */
struct array* reserve_array(struct array *target, int new_capacity);

/**
 * @brief Insert new element into `target` at last.
 * 
 * @param target a `struct array *` where wants to store given element.
 * @param element new item to insert.
 * @return always return `target`
 * @note Internally, this function uses `malloc` and `memcpy`.
 */
struct array* insert_back(struct array *target, void *element);

/**
 * @brief Remove and cleanup element at last.
 * 
 * @param target a `struct array *`
 * @return always return `target`
 */
struct array* remove_back(struct array *target);

/**
 * @brief Cleanup members of `struct array *` instance.
 * 
 * @param target target to cleanup
 */
void destruct_target(struct array* target);