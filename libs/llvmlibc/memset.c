// llvm-libc compatible memset for i686 bare-metal
#include <stddef.h>
#include <stdint.h>

void* memset(void* dst, int c, size_t n) {
    unsigned char* d = (unsigned char*)dst;
    unsigned char val = (unsigned char)c;

    // Word-aligned fast path
    if ((uintptr_t)d % 4 == 0 && n >= 4) {
        uint32_t val32 = val | (val << 8) | (val << 16) | (val << 24);
        uint32_t* d32 = (uint32_t*)d;
        while (n >= 4) {
            *d32++ = val32;
            n -= 4;
        }
        d = (unsigned char*)d32;
    }

    // Byte set remainder
    while (n--) *d++ = val;
    return dst;
}
