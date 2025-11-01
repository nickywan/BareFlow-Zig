// Simple memset for kernel
void* memset(void* dest, int val, unsigned long len) {
    unsigned char* ptr = (unsigned char*)dest;
    while (len-- > 0) {
        *ptr++ = (unsigned char)val;
    }
    return dest;
}