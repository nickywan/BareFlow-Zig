/**
 * String and Memory Functions
 *
 * Standard C library string/memory functions for bare-metal.
 */

#ifndef STRING_H
#define STRING_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// MEMORY FUNCTIONS
// ============================================================================

/**
 * Fill memory with constant byte
 */
void* memset(void* s, int c, size_t n);

/**
 * Copy memory area
 */
void* memcpy(void* dest, const void* src, size_t n);

/**
 * Copy memory area (handles overlapping regions)
 */
void* memmove(void* dest, const void* src, size_t n);

/**
 * Compare memory areas
 */
int memcmp(const void* s1, const void* s2, size_t n);

// ============================================================================
// STRING FUNCTIONS
// ============================================================================

/**
 * Calculate string length
 */
size_t strlen(const char* s);

/**
 * Copy string
 */
char* strcpy(char* dest, const char* src);

/**
 * Copy string with length limit
 */
char* strncpy(char* dest, const char* src, size_t n);

/**
 * Compare strings
 */
int strcmp(const char* s1, const char* s2);

/**
 * Compare strings with length limit
 */
int strncmp(const char* s1, const char* s2, size_t n);

/**
 * Concatenate strings
 */
char* strcat(char* dest, const char* src);

#ifdef __cplusplus
}
#endif

#endif // STRING_H
