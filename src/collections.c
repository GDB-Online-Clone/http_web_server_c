#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "collections.h"

struct array* init_array(struct array *target, int m_size, void (*cleanup_handler)(void *)) {
    const int INITIAL_CAPACITY = 8;
    target->capacity = INITIAL_CAPACITY;
    target->items = malloc(INITIAL_CAPACITY * m_size);
    target->size = 0;
    target->m_size = m_size;
    target->cleanup_handler = cleanup_handler;

    return target;    
}


struct array* insert_back(struct array *target, void *element) {
    if (target->capacity == target->size) {
        int new_capacity = target->capacity * 2;

        void* new_items = realloc(target->items, new_capacity * target->m_size);
        if (!new_items) {
            perror("realloc");            
            exit(EXIT_FAILURE);
        }
        target->items = new_items;
        target->capacity = new_capacity;            
    }

    memcpy(item_ptr_of(target, target->size), element, target->m_size);
    target->size++;
    
    return target;
}

struct array* reserve_array(struct array *target, int new_capacity) {
    void* new_items = realloc(target->items, new_capacity * target->m_size);
    if (!new_items) {
        perror("realloc");            
        exit(EXIT_FAILURE);
    }
    target->items = new_items;
    target->capacity = new_capacity;
    return target;
}

struct array* remove_back(struct array *target) {
    if (target->size == 0)
        return target;

    target->cleanup_handler(item_ptr_of(target, target->size - 1));

    target->size--;
    return target;
}

void destruct_target(struct array* target) {
    while (target->size--) {
        target->cleanup_handler(item_ptr_of(target, target->size));
    }
    free(target->items);
    target->items = NULL;
    target->capacity = 0;
    target->size = 0;    
}