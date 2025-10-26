// SHA256 benchmark - tests memory bandwidth and bitwise operations
// Reduced to 1KB chunks for bare-metal constraints

#include <stdint.h>

#define MODULE_MAGIC 0x4D4F4442

typedef struct {
    uint32_t magic;
    char name[32];
    void* entry_point;
    uint32_t code_size;
    uint32_t version;
} __attribute__((packed)) module_header_t;

// SHA256 constants
static const uint32_t K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

#define ROTR(x,n) (((x) >> (n)) | ((x) << (32-(n))))
#define CH(x,y,z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x,y,z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTR(x,2) ^ ROTR(x,13) ^ ROTR(x,22))
#define EP1(x) (ROTR(x,6) ^ ROTR(x,11) ^ ROTR(x,25))
#define SIG0(x) (ROTR(x,7) ^ ROTR(x,18) ^ ((x) >> 3))
#define SIG1(x) (ROTR(x,17) ^ ROTR(x,19) ^ ((x) >> 10))

static void sha256_transform(uint32_t state[8], const uint8_t data[64]) {
    uint32_t w[64];
    uint32_t a, b, c, d, e, f, g, h, t1, t2;

    // Prepare message schedule
    for (int i = 0; i < 16; i++)
        w[i] = (data[i*4] << 24) | (data[i*4+1] << 16) | (data[i*4+2] << 8) | data[i*4+3];
    for (int i = 16; i < 64; i++)
        w[i] = SIG1(w[i-2]) + w[i-7] + SIG0(w[i-15]) + w[i-16];

    // Initialize working variables
    a = state[0]; b = state[1]; c = state[2]; d = state[3];
    e = state[4]; f = state[5]; g = state[6]; h = state[7];

    // Compression
    for (int i = 0; i < 64; i++) {
        t1 = h + EP1(e) + CH(e,f,g) + K[i] + w[i];
        t2 = EP0(a) + MAJ(a,b,c);
        h = g; g = f; f = e; e = d + t1;
        d = c; c = b; b = a; a = t1 + t2;
    }

    // Update state
    state[0] += a; state[1] += b; state[2] += c; state[3] += d;
    state[4] += e; state[5] += f; state[6] += g; state[7] += h;
}

// Simple inline memcpy for module
static void* my_memcpy(void* dst, const void* src, int n) {
    char* d = (char*)dst;
    const char* s = (const char*)src;
    while (n--) *d++ = *s++;
    return dst;
}

static int sha256_benchmark(void) {
    static uint8_t data[1024] = {1}; // 1KB test data (.data section)
    uint32_t state[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };

    // Process 16 blocks (1KB)
    for (int i = 0; i < 16; i++)
        sha256_transform(state, data + i * 64);

    // Return checksum
    return (int)(state[0] ^ state[7]);
}

__attribute__((section(".module_header")))
const module_header_t module_info = {
    .magic = MODULE_MAGIC,
    .name = "sha256",
    .entry_point = (void*)sha256_benchmark,
    .code_size = 0,
    .version = 1
};
