#pragma once

#include <string.h>


/**
 * @brief Check input is white-space.
 * 
 * @param ch chracter to check
 * @retval 1 if input is white-space
 * @retval 0 if input is white-space
 * 
 * @note The '\n', '\t', '\r', ' ', '\f', and '\v' are white-spaces.
 */
inline static int is_non_space(char ch) {
    return (ch == '\n' || ch == '\t' || ch == '\r' || ch == ' ' || ch == '\f' || ch == '\v');
}

/**
 * @brief Find first non-whitespace character and return its pointer.
 * 
 * @param str String to find non-whitespace
 * @return Pointer of first non-white-space character. If it reaches at `\0` before finding any non-white-space, return **NULL**.
 * @retval `NULL` If cannot find any non-white-space character before `\0`
 */
char *find_non_space (char *str);