// ============================================================================
// Prime Sieve (Sieve of Eratosthenes) - Compute-Intensive Test Module
// ============================================================================
// Prime sieve is perfect for PGO because:
// - Nested loops with predictable patterns
// - Memory access patterns that benefit from optimization
// - Loop unrolling opportunities

#define SIEVE_SIZE 200

// Sieve of Eratosthenes to find all primes up to SIEVE_SIZE
static int sieve_of_eratosthenes(void) {
    static unsigned char is_prime[SIEVE_SIZE];

    // Initialize: assume all numbers are prime
    for (int i = 0; i < SIEVE_SIZE; i++) {
        is_prime[i] = 1;
    }

    is_prime[0] = 0;  // 0 is not prime
    is_prime[1] = 0;  // 1 is not prime

    // Sieve
    for (int i = 2; i * i < SIEVE_SIZE; i++) {
        if (is_prime[i]) {
            // Mark all multiples of i as not prime
            for (int j = i * i; j < SIEVE_SIZE; j += i) {
                is_prime[j] = 0;
            }
        }
    }

    // Count primes
    int count = 0;
    for (int i = 2; i < SIEVE_SIZE; i++) {
        if (is_prime[i]) {
            count++;
        }
    }

    return count;
}

// Test if a number is prime (trial division)
static int is_prime_trial(int n) {
    if (n < 2) return 0;
    if (n == 2) return 1;
    if (n % 2 == 0) return 0;

    for (int i = 3; i * i <= n; i += 2) {
        if (n % i == 0) return 0;
    }

    return 1;
}

// Entry point - will be called 10000+ times for VERY_HOT classification
int compute(void) {
    // Run sieve
    int sieve_count = sieve_of_eratosthenes();

    // Verify with trial division for first few primes
    int trial_count = 0;
    for (int i = 2; i < 50; i++) {
        if (is_prime_trial(i)) {
            trial_count++;
        }
    }

    // Return combined result
    return (sieve_count * 1000) + trial_count;
}
