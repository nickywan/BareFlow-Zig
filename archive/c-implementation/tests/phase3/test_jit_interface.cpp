// test_jit_interface.cpp - Test the unified JIT interface with profiling
// This test demonstrates the merged interface+runtime capabilities

#include "kernel/jit_interface.h"
#include <iostream>
#include <cstring>

// Simulate cycle counting (in real kernel, use rdtsc)
static inline uint64_t read_cycles() {
    static uint64_t fake_cycles = 0;
    fake_cycles += 1000; // Simulate 1000 cycles per call
    return fake_cycles;
}

int main() {
    std::cout << "=== BareFlow JIT Interface Test (LLVM 18) ===\n\n";

    // 1. Create JIT context
    std::cout << "[1] Creating JIT context...\n";
    JITContext* ctx = jit_create();
    if (!ctx) {
        std::cerr << "Failed to create JIT context\n";
        return 1;
    }
    std::cout << "    [OK] JIT context created\n\n";

    // 2. Load bitcode module
    std::cout << "[2] Loading bitcode module...\n";
    JITModule* mod = jit_load_bitcode(ctx, "libs/minimal.bc");
    if (!mod) {
        std::cerr << "    [ERROR] " << jit_get_last_error(ctx) << "\n";
        jit_destroy(ctx);
        return 1;
    }
    std::cout << "    [OK] Module loaded\n\n";

    // 3. Find strlen function
    std::cout << "[3] Looking up 'strlen' function...\n";
    typedef size_t (*StrlenFunc)(const char*);
    StrlenFunc strlen_jit = (StrlenFunc)jit_find_function(ctx, "strlen");
    if (!strlen_jit) {
        std::cerr << "    [ERROR] " << jit_get_last_error(ctx) << "\n";
        jit_unload_module(mod);
        jit_destroy(ctx);
        return 1;
    }
    std::cout << "    [OK] Function found at " << (void*)strlen_jit << "\n\n";

    // 4. Test function execution with profiling
    std::cout << "[4] Testing function with profiling...\n";
    const char* test_str = "BareFlow LLVM JIT";

    for (int i = 0; i < 150; i++) {
        uint64_t start = read_cycles();
        size_t len = strlen_jit(test_str);
        uint64_t end = read_cycles();

        jit_record_call(ctx, "strlen", end - start);

        if (i == 0) {
            std::cout << "    strlen(\"" << test_str << "\") = " << len << "\n";
        }

        // Trigger auto-optimization periodically
        if (i > 0 && i % 50 == 0) {
            int result = jit_auto_optimize(ctx, "strlen");
            if (result > 0) {
                std::cout << "    [OPTIMIZE] Function reoptimized after " << i << " calls\n";
            }
        }
    }
    std::cout << "    [OK] Executed function 150 times\n\n";

    // 5. Get function profiling info
    std::cout << "[5] Function profiling info:\n";
    JITFunctionInfo info;
    if (jit_get_function_info(ctx, "strlen", &info) == 0) {
        std::cout << "    Name: " << info.name << "\n";
        std::cout << "    Code ptr: " << info.code_ptr << "\n";
        std::cout << "    Call count: " << info.call_count << "\n";
        std::cout << "    Total cycles: " << info.total_cycles << "\n";
        std::cout << "    Avg cycles/call: "
                  << (info.call_count > 0 ? info.total_cycles / info.call_count : 0) << "\n";
        std::cout << "    Opt level: ";
        switch (info.current_opt_level) {
            case JIT_OPT_NONE: std::cout << "NONE\n"; break;
            case JIT_OPT_BASIC: std::cout << "BASIC (-O1)\n"; break;
            case JIT_OPT_AGGRESSIVE: std::cout << "AGGRESSIVE (-O2/-O3)\n"; break;
        }
    }
    std::cout << "\n";

    // 6. List all functions
    std::cout << "[6] Listing all JIT functions:\n";
    JITFunctionInfo functions[64];
    int count = jit_list_functions(ctx, functions, 64);
    std::cout << "    Found " << count << " function(s)\n";
    for (int i = 0; i < count; i++) {
        std::cout << "    - " << functions[i].name
                  << " (calls: " << functions[i].call_count << ")\n";
    }
    std::cout << "\n";

    // 7. Get global statistics
    std::cout << "[7] Global JIT statistics:\n";
    JITStats stats;
    jit_get_stats(ctx, &stats);
    std::cout << "    Functions compiled: " << stats.functions_compiled << "\n";
    std::cout << "    Total function calls: " << stats.total_function_calls << "\n";
    std::cout << "    Reoptimizations: " << stats.reoptimizations << "\n";
    std::cout << "    Memory used: " << stats.memory_used_bytes << " bytes\n\n";

    // 8. Cleanup
    std::cout << "[8] Cleaning up...\n";
    jit_unload_module(mod);
    jit_destroy(ctx);
    std::cout << "    [OK] Cleanup complete\n\n";

    std::cout << "=== ALL TESTS PASSED ===\n";
    return 0;
}
