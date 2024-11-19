#include <string.h>

#include "utility.h"


char *find_non_space (char *str) {
    while (is_non_space(*str) && *str != '\0') 
        str++;
    if (*str == '\0') 
        return NULL;    
    return str;
}