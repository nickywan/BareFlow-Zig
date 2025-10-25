/**
 * Compiler Runtime Intrinsics
 *
 * Provides compiler intrinsics needed for 32-bit bare-metal compilation.
 * Specifically, 64-bit division support (__udivdi3).
 */

#include <stdint.h>

/**
 * 64-bit unsigned division
 * Required by compiler for uint64_t / uint64_t on 32-bit platforms
 */
uint64_t __udivdi3(uint64_t dividend, uint64_t divisor) {
    if (divisor == 0) {
        return 0;  // Division by zero - return 0 instead of crashing
    }

    // Simple long division algorithm for 64-bit
    if (divisor > dividend) {
        return 0;
    }

    if (divisor == dividend) {
        return 1;
    }

    uint64_t quotient = 0;
    uint64_t remainder = 0;

    // Process each bit from MSB to LSB
    for (int i = 63; i >= 0; i--) {
        remainder <<= 1;
        remainder |= (dividend >> i) & 1;

        if (remainder >= divisor) {
            remainder -= divisor;
            quotient |= (1ULL << i);
        }
    }

    return quotient;
}

/**
 * 64-bit signed division
 * Required by compiler for int64_t / int64_t on 32-bit platforms
 */
int64_t __divdi3(int64_t dividend, int64_t divisor) {
    if (divisor == 0) {
        return 0;
    }

    // Handle signs
    int negative = 0;
    uint64_t udividend = (uint64_t)dividend;
    uint64_t udivisor = (uint64_t)divisor;

    if (dividend < 0) {
        negative = !negative;
        udividend = -dividend;
    }

    if (divisor < 0) {
        negative = !negative;
        udivisor = -divisor;
    }

    uint64_t result = __udivdi3(udividend, udivisor);

    return negative ? -(int64_t)result : (int64_t)result;
}
