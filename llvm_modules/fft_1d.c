// ============================================================================
// 1D FFT (Fast Fourier Transform) - Complex Compute-Intensive Module
// ============================================================================
// FFT is excellent for PGO because:
// - Multiple conditional branches based on bit reversal patterns
// - Complex arithmetic with predictable access patterns
// - Hot loops that benefit from aggressive optimization
// - Real-world scientific computing workload
// ============================================================================

#define FFT_SIZE 16  // Power of 2

// Simple fixed-point representation for complex numbers
typedef struct {
    int real;  // Fixed-point: multiply by 1000
    int imag;
} complex_t;

// Sine/cosine lookup table for common angles (fixed-point * 1000)
static const int sine_table[] = {
    0,    // sin(0)
    707,  // sin(45°) ≈ 0.707
    1000, // sin(90°) = 1.000
    707,  // sin(135°) ≈ 0.707
    0,    // sin(180°)
    -707, // sin(225°)
    -1000,// sin(270°)
    -707  // sin(315°)
};

static const int cosine_table[] = {
    1000, // cos(0) = 1.000
    707,  // cos(45°) ≈ 0.707
    0,    // cos(90°)
    -707, // cos(135°)
    -1000,// cos(180°)
    -707, // cos(225°)
    0,    // cos(270°)
    707   // cos(315°)
};

// Bit reversal for FFT input reordering
static unsigned int bit_reverse(unsigned int x, int bits) {
    unsigned int result = 0;
    for (int i = 0; i < bits; i++) {
        if (x & (1 << i)) {
            result |= (1 << (bits - 1 - i));
        }
    }
    return result;
}

// Complex multiplication (fixed-point)
static complex_t complex_mul(complex_t a, complex_t b) {
    complex_t result;
    // (a.real + i*a.imag) * (b.real + i*b.imag)
    // = (a.real*b.real - a.imag*b.imag) + i*(a.real*b.imag + a.imag*b.real)
    result.real = (a.real * b.real - a.imag * b.imag) / 1000;
    result.imag = (a.real * b.imag + a.imag * b.real) / 1000;
    return result;
}

// 1D FFT implementation (Cooley-Tukey algorithm)
static void fft_1d(complex_t* data, int n) {
    // Bit-reversal permutation
    int bits = 0;
    int temp = n;
    while (temp > 1) {
        bits++;
        temp >>= 1;
    }

    for (int i = 0; i < n; i++) {
        int j = bit_reverse(i, bits);
        if (j > i) {
            // Swap data[i] and data[j]
            complex_t temp = data[i];
            data[i] = data[j];
            data[j] = temp;
        }
    }

    // FFT computation
    for (int len = 2; len <= n; len <<= 1) {
        int half = len >> 1;

        for (int i = 0; i < n; i += len) {
            for (int j = 0; j < half; j++) {
                // Twiddle factor index (simplified for small FFT)
                int angle_idx = (8 * j) / half;
                if (angle_idx >= 8) angle_idx = 7;

                complex_t twiddle;
                twiddle.real = cosine_table[angle_idx];
                twiddle.imag = -sine_table[angle_idx];

                complex_t t = complex_mul(twiddle, data[i + j + half]);

                complex_t u = data[i + j];

                // Butterfly operation
                data[i + j].real = u.real + t.real;
                data[i + j].imag = u.imag + t.imag;

                data[i + j + half].real = u.real - t.real;
                data[i + j + half].imag = u.imag - t.imag;
            }
        }
    }
}

// Entry point - will be called 2000+ times for HOT classification
int compute(void) {
    static complex_t data[FFT_SIZE];

    // Initialize with test signal (sum of two frequencies)
    for (int i = 0; i < FFT_SIZE; i++) {
        // Simple test: DC component + fundamental frequency
        data[i].real = 1000 + (i % 2) * 500;  // DC + square wave
        data[i].imag = 0;
    }

    // Perform FFT
    fft_1d(data, FFT_SIZE);

    // Compute magnitude of first few frequency bins (checksum)
    int magnitude_sum = 0;
    for (int i = 0; i < 4; i++) {
        // Magnitude² = real² + imag²
        int mag_sq = (data[i].real * data[i].real + data[i].imag * data[i].imag) / 1000;
        magnitude_sum += mag_sq;
    }

    return magnitude_sum;
}
