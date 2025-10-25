// FFT 1D benchmark module (radix-2, in-place)
// Tests: memory stride, trig functions, complex arithmetic
// Size: 32 samples (minimal for bare-metal)

#include <stdint.h>

#define MODULE_MAGIC 0x4D4F4442
#define N 32  // Must be power of 2

typedef struct {
    uint32_t magic;
    char name[32];
    void* entry_point;
    uint32_t code_size;
    uint32_t version;
} __attribute__((packed)) module_header_t;

// Complex number
typedef struct { double re, im; } complex_t;

// Simplified sin/cos (no external dependencies)
static double my_sin(double x) {
    double x2 = x * x;
    return x * (1.0 - x2/6.0 * (1.0 - x2/20.0));
}

static double my_cos(double x) {
    double x2 = x * x;
    return 1.0 - x2/2.0 * (1.0 - x2/12.0);
}

static const double PI = 3.14159265358979;

// Bit reversal for FFT
static uint32_t reverse_bits(uint32_t x, int bits) {
    uint32_t result = 0;
    for (int i = 0; i < bits; i++) {
        result = (result << 1) | (x & 1);
        x >>= 1;
    }
    return result;
}

// In-place radix-2 FFT
static void fft_inplace(complex_t* data, int n) {
    // Bit-reversal permutation
    int bits = 0;
    for (int i = n; i > 1; i >>= 1) bits++;

    for (uint32_t i = 0; i < (uint32_t)n; i++) {
        uint32_t j = reverse_bits(i, bits);
        if (j > i) {
            complex_t temp = data[i];
            data[i] = data[j];
            data[j] = temp;
        }
    }

    // FFT butterfly
    for (int size = 2; size <= n; size *= 2) {
        double angle = -2.0 * PI / size;
        for (int start = 0; start < n; start += size) {
            for (int k = 0; k < size/2; k++) {
                double a = angle * k;
                double wr = my_cos(a);
                double wi = my_sin(a);

                complex_t* even = &data[start + k];
                complex_t* odd = &data[start + k + size/2];

                complex_t t;
                t.re = odd->re * wr - odd->im * wi;
                t.im = odd->re * wi + odd->im * wr;

                odd->re = even->re - t.re;
                odd->im = even->im - t.im;
                even->re = even->re + t.re;
                even->im = even->im + t.im;
            }
        }
    }
}

static int fft_benchmark(void) {
    static complex_t data[N] = {{1}};  // Use .data section

    // Initialize with simple signal
    for (int i = 0; i < N; i++) {
        data[i].re = my_cos(2.0 * PI * i / N);
        data[i].im = 0;
    }

    // Perform FFT
    fft_inplace(data, N);

    // Checksum (sum of magnitudes)
    int checksum = 0;
    for (int i = 0; i < N; i++) {
        int mag = (int)(data[i].re * data[i].re + data[i].im * data[i].im);
        checksum ^= mag;
    }

    return checksum;
}

__attribute__((section(".module_header")))
const module_header_t module_info = {
    .magic = MODULE_MAGIC,
    .name = "fft_1d",
    .entry_point = (void*)fft_benchmark,
    .code_size = 0,
    .version = 1
};
