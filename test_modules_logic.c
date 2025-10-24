#include <stdio.h>

// Test fibonacci
int test_fibonacci(void) {
    int a = 0, b = 1;
    for (int i = 0; i < 20; i++) {
        int temp = a + b;
        a = b;
        b = temp;
    }
    return a;
}

// Test sum
int test_sum(void) {
    int sum = 0;
    for (int i = 1; i <= 100; i++) {
        sum += i;
    }
    return sum;
}

// Test primes
int is_prime(int n) {
    if (n < 2) return 0;
    if (n == 2) return 1;
    if (n % 2 == 0) return 0;
    for (int i = 3; i * i <= n; i += 2) {
        if (n % i == 0) return 0;
    }
    return 1;
}

int test_primes(void) {
    int count = 0;
    for (int i = 0; i < 1000; i++) {
        if (is_prime(i)) {
            count++;
        }
    }
    return count;
}

int main() {
    printf("Fibonacci(20): %d (expected: 6765)\n", test_fibonacci());
    printf("Sum(1..100): %d (expected: 5050)\n", test_sum());
    printf("Primes(<1000): %d (expected: 168)\n", test_primes());
    return 0;
}
