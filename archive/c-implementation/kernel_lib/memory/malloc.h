/**
 * Memory Allocator
 *
 * Simple bump allocator for bare-metal environment.
 * 256KB heap, 16-byte alignment, no free() implementation.
 */

#ifndef MALLOC_H
#define MALLOC_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Allocate memory from heap
 * Returns 16-byte aligned pointer or NULL if out of memory
 */
void* malloc(size_t size);

/**
 * Allocate and zero-initialize memory
 */
void* calloc(size_t nmemb, size_t size);

/**
 * Reallocate memory (always allocates new memory, doesn't free old)
 */
void* realloc(void* ptr, size_t size);

/**
 * Free memory (no-op in bump allocator)
 */
void free(void* ptr);

#ifdef __cplusplus
}
#endif

#endif // MALLOC_H
