// ============================================================================
// Compute Dispatch Table - Indirect Call Module for PGO
// ============================================================================
// This module is EXCELLENT for PGO because:
// - Indirect function calls through function pointers
// - Virtual dispatch simulation (like C++ vtables)
// - Call frequency patterns vary dramatically
// - Branch target prediction critical for performance
//
// PGO Benefits:
// - Devirtualization of hot call sites
// - Speculative inlining of frequently called operations
// - Branch target buffer optimization
// - Call graph optimization based on actual usage
// ============================================================================

#define NUM_OPERATIONS 8
#define COMPUTE_ITERATIONS 32

// Operation function pointer type
typedef int (*operation_fn)(int a, int b);

// Basic arithmetic operations
static int op_add(int a, int b) {
    return a + b;
}

static int op_sub(int a, int b) {
    return a - b;
}

static int op_mul(int a, int b) {
    return a * b;
}

static int op_div(int a, int b) {
    // Safe division
    if (b == 0) return 0;
    return a / b;
}

// Bitwise operations
static int op_and(int a, int b) {
    return a & b;
}

static int op_or(int a, int b) {
    return a | b;
}

static int op_xor(int a, int b) {
    return a ^ b;
}

// Complex operation (most expensive)
static int op_mod_pow(int a, int b) {
    // Modular exponentiation (simplified)
    if (b == 0) return 1;
    int result = 1;
    int base = a % 100;  // Keep numbers manageable
    int exp = b % 10;    // Limit exponent

    for (int i = 0; i < exp; i++) {
        result = (result * base) % 1000;
    }
    return result;
}

// Global dispatch table (simulates vtable)
static operation_fn dispatch_table[NUM_OPERATIONS] = {
    op_add,      // Index 0 - Most frequently used
    op_sub,      // Index 1 - Frequently used
    op_mul,      // Index 2 - Moderately used
    op_div,      // Index 3 - Moderately used
    op_and,      // Index 4 - Rarely used
    op_or,       // Index 5 - Rarely used
    op_xor,      // Index 6 - Rarely used
    op_mod_pow   // Index 7 - Very rarely used but expensive
};

// Simulated random selection with bias towards common operations
static unsigned int dispatch_rng = 12345;

static int get_operation_index(void) {
    dispatch_rng = dispatch_rng * 1103515245 + 12345;
    int rand_val = (int)((dispatch_rng / 65536) % 100);

    // CRITICAL: Non-uniform distribution simulates real-world usage
    // PGO will learn these patterns and optimize accordingly
    if (rand_val < 40) {
        return 0;  // 40% - op_add (very hot)
    } else if (rand_val < 70) {
        return 1;  // 30% - op_sub (hot)
    } else if (rand_val < 85) {
        return 2;  // 15% - op_mul (warm)
    } else if (rand_val < 93) {
        return 3;  // 8% - op_div (warm)
    } else if (rand_val < 96) {
        return 4;  // 3% - op_and (cold)
    } else if (rand_val < 98) {
        return 5;  // 2% - op_or (cold)
    } else if (rand_val < 99) {
        return 6;  // 1% - op_xor (very cold)
    } else {
        return 7;  // 1% - op_mod_pow (very cold but expensive)
    }
}

// Dispatch function with indirect call
// This is the HOT PATH that PGO optimizes
static int dispatch_compute(int op_idx, int a, int b) {
    // Bounds check (predictable branch)
    if (op_idx < 0 || op_idx >= NUM_OPERATIONS) {
        return 0;
    }

    // CRITICAL INDIRECT CALL
    // PGO can:
    // 1. Predict most likely target (op_add)
    // 2. Inline hot functions
    // 3. Devirtualize based on call frequency
    return dispatch_table[op_idx](a, b);
}

// Polymorphic compute workload
static int polymorphic_compute(void) {
    int accumulator = 0;

    // Perform multiple dispatched operations
    for (int i = 0; i < COMPUTE_ITERATIONS; i++) {
        int op_idx = get_operation_index();
        int a = (i * 7 + 13) % 100;
        int b = (i * 11 + 29) % 50 + 1;  // Avoid division by zero

        // Indirect call through dispatch table
        int result = dispatch_compute(op_idx, a, b);

        // Accumulate result with wrapping
        accumulator = (accumulator + result) % 10000;
    }

    return accumulator;
}

// Strategy pattern simulation
typedef enum {
    STRATEGY_DIRECT,      // Direct computation
    STRATEGY_DISPATCH,    // Dispatch table
    STRATEGY_HYBRID       // Mix of both
} compute_strategy_t;

static int execute_strategy(compute_strategy_t strategy, int iterations) {
    int result = 0;

    switch (strategy) {
        case STRATEGY_DIRECT:
            // Direct calls (fast but less flexible)
            for (int i = 0; i < iterations; i++) {
                int a = i % 50;
                int b = (i + 1) % 30 + 1;
                result += op_add(a, b);
                result %= 10000;
            }
            break;

        case STRATEGY_DISPATCH:
            // Dispatched calls (flexible but slower without PGO)
            result = polymorphic_compute();
            break;

        case STRATEGY_HYBRID:
            // Mix of both (complex branching)
            for (int i = 0; i < iterations / 2; i++) {
                if (i % 3 == 0) {
                    int a = i % 50;
                    int b = (i + 1) % 30 + 1;
                    result += op_add(a, b);
                } else {
                    int op_idx = get_operation_index();
                    int a = i % 50;
                    int b = (i + 1) % 30 + 1;
                    result += dispatch_compute(op_idx, a, b);
                }
                result %= 10000;
            }
            break;
    }

    return result;
}

// Entry point - will be called 3000+ times for HOT classification
int compute(void) {
    // Cycle through different strategies to stress dispatch patterns
    static int call_count = 0;
    call_count++;

    compute_strategy_t strategy;

    // COMPLEX BRANCHING based on call patterns
    // PGO learns which strategy is most common
    if (call_count % 10 < 5) {
        strategy = STRATEGY_DISPATCH;  // 50% - Most common
    } else if (call_count % 10 < 8) {
        strategy = STRATEGY_HYBRID;    // 30% - Less common
    } else {
        strategy = STRATEGY_DIRECT;    // 20% - Least common
    }

    // Execute chosen strategy
    int result = execute_strategy(strategy, COMPUTE_ITERATIONS);

    // Add call count for variation
    return (result * 1000 + call_count) % 1000000;
}
