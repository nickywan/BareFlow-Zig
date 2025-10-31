// llvm-libc compatible memcpy for i686 bare-metal
#include <stddef.h>
#include <stdint.h>

void* memcpy(void* restrict dst, const void* restrict src, size_t n) {
    unsigned char* d = (unsigned char*)dst;
    const unsigned char* s = (const unsigned char*)src;

    // Word-aligned fast path
    if (((uintptr_t)d | (uintptr_t)s) % 4 == 0) {
        uint32_t* d32 = (uint32_t*)d;
        const uint32_t* s32 = (const uint32_t*)s;
        while (n >= 4) {
            *d32++ = *s32++;
            n -= 4;
        }
        d = (unsigned char*)d32;
        s = (const unsigned char*)s32;
    }

    // Byte copy remainder
    while (n--) *d++ = *s++;
    return dst;
}
