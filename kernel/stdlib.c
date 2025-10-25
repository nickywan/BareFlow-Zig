// kernel/stdlib.c - Minimal libc for Fluid Phase 1

#include <stddef.h>
#include <stdint.h>

// ============================================================================
// STRING FUNCTIONS - DÉFINIES D'ABORD
// ============================================================================

void* memset(void* s, int c, size_t n) {
    uint8_t* p = (uint8_t*)s;
    while (n--)
        *p++ = (uint8_t)c;
    return s;
}

void* memcpy(void* dest, const void* src, size_t n) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    while (n--)
        *d++ = *s++;
    return dest;
}

void* memmove(void* dest, const void* src, size_t n) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    
    if (d < s) {
        while (n--)
            *d++ = *s++;
    } else {
        d += n;
        s += n;
        while (n--)
            *--d = *--s;
    }
    return dest;
}

int memcmp(const void* s1, const void* s2, size_t n) {
    const uint8_t* p1 = (const uint8_t*)s1;
    const uint8_t* p2 = (const uint8_t*)s2;
    
    while (n--) {
        if (*p1 != *p2)
            return *p1 - *p2;
        p1++;
        p2++;
    }
    return 0;
}

size_t strlen(const char* s) {
    size_t len = 0;
    while (s[len])
        len++;
    return len;
}

char* strcpy(char* dest, const char* src) {
    char* d = dest;
    while ((*d++ = *src++));
    return dest;
}

char* strncpy(char* dest, const char* src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i]; i++)
        dest[i] = src[i];
    for (; i < n; i++)
        dest[i] = '\0';
    return dest;
}

int strcmp(const char* s1, const char* s2) {
    while (*s1 && *s1 == *s2) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

int strncmp(const char* s1, const char* s2, size_t n) {
    while (n && *s1 && *s1 == *s2) {
        s1++;
        s2++;
        n--;
    }
    return n ? *(unsigned char*)s1 - *(unsigned char*)s2 : 0;
}

char* strcat(char* dest, const char* src) {
    char* d = dest;
    while (*d)
        d++;
    while ((*d++ = *src++));
    return dest;
}

// ============================================================================
// MEMORY ALLOCATOR - APRÈS les string functions
// ============================================================================

#define HEAP_SIZE (256 * 1024)  // 256 KB heap
static uint8_t heap[HEAP_SIZE] __attribute__((aligned(16)));
static size_t heap_offset = 0;

void* malloc(size_t size) {
    if (size == 0)
        return NULL;
    
    // Align to 16 bytes
    size = (size + 15) & ~15;
    
    if (heap_offset + size > HEAP_SIZE)
        return NULL;
    
    void* ptr = &heap[heap_offset];
    heap_offset += size;
    return ptr;
}

void* calloc(size_t nmemb, size_t size) {
    size_t total = nmemb * size;
    void* ptr = malloc(total);
    if (ptr)
        memset(ptr, 0, total);  // ✅ memset déjà définie
    return ptr;
}

void* realloc(void* ptr, size_t size) {
    if (!ptr)
        return malloc(size);
    
    void* new_ptr = malloc(size);
    if (new_ptr && ptr) {
        memcpy(new_ptr, ptr, size);  // ✅ memcpy déjà définie
    }
    return new_ptr;
}

void free(void* ptr) {
    (void)ptr;
}

// ============================================================================
// MATH FUNCTIONS
// ============================================================================

int abs(int n) {
    return n < 0 ? -n : n;
}

long labs(long n) {
    return n < 0 ? -n : n;
}