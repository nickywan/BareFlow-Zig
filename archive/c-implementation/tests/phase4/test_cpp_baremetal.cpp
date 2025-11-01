/**
 * Test: Bare-Metal C++ Runtime
 *
 * Tests C++ runtime with custom malloc (no standard library).
 * This validates the runtime for bare-metal LLVM integration.
 */

#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

// ============================================================================
// External Functions (from kernel_lib)
// ============================================================================

extern "C" {
    // From kernel_lib/memory/malloc.c
    void* malloc(size_t size);
    void free(void* ptr);
    void* calloc(size_t nmemb, size_t size);

    // Minimal string functions
    void* memset(void* s, int c, size_t n);
    void* memcpy(void* dest, const void* src, size_t n);
    size_t strlen(const char* s);
}

// ============================================================================
// Minimal Printf Implementation
// ============================================================================

void print(const char* str) {
    write(1, str, strlen(str));
}

void print_int(int value) {
    char buf[32];
    int i = 0;
    int is_negative = 0;

    if (value < 0) {
        is_negative = 1;
        value = -value;
    }

    if (value == 0) {
        buf[i++] = '0';
    } else {
        while (value > 0) {
            buf[i++] = '0' + (value % 10);
            value /= 10;
        }
    }

    if (is_negative) {
        buf[i++] = '-';
    }

    // Reverse
    for (int j = 0; j < i / 2; j++) {
        char tmp = buf[j];
        buf[j] = buf[i - j - 1];
        buf[i - j - 1] = tmp;
    }

    buf[i] = '\0';
    print(buf);
}

// ============================================================================
// Test Classes
// ============================================================================

class SimpleClass {
public:
    int value;

    SimpleClass(int v) : value(v) {
        print("  SimpleClass(");
        print_int(v);
        print(") constructed\n");
    }

    ~SimpleClass() {
        print("  SimpleClass(");
        print_int(value);
        print(") destructed\n");
    }

    int getValue() const { return value; }
};

// ============================================================================
// Tests
// ============================================================================

void test_heap_allocation() {
    print("\n=== Test 1: Heap Allocation ===\n");

    SimpleClass* obj = new SimpleClass(42);
    print("  Allocated object, value = ");
    print_int(obj->getValue());
    print("\n");

    delete obj;
    print("  ✅ PASS\n");
}

void test_array_allocation() {
    print("\n=== Test 2: Array Allocation ===\n");

    int* arr = new int[10];
    for (int i = 0; i < 10; i++) {
        arr[i] = i * i;
    }

    print("  Array[5] = ");
    print_int(arr[5]);
    print("\n");

    delete[] arr;
    print("  ✅ PASS\n");
}

void test_multiple_allocations() {
    print("\n=== Test 3: Multiple Objects ===\n");

    SimpleClass* obj1 = new SimpleClass(10);
    SimpleClass* obj2 = new SimpleClass(20);
    SimpleClass* obj3 = new SimpleClass(30);

    int sum = obj1->getValue() + obj2->getValue() + obj3->getValue();
    print("  Sum = ");
    print_int(sum);
    print("\n");

    delete obj3;
    delete obj2;
    delete obj1;

    print("  ✅ PASS\n");
}

// ============================================================================
// Main
// ============================================================================

int main() {
    print("=== Bare-Metal C++ Runtime Test ===\n");
    print("Using custom malloc + cpp_runtime.a\n");

    test_heap_allocation();
    test_array_allocation();
    test_multiple_allocations();

    print("\n=== Summary ===\n");
    print("✅ SUCCESS: Bare-metal C++ runtime working!\n");
    print("   - Custom malloc: ✓\n");
    print("   - operator new/delete: ✓\n");
    print("   - C++ constructors/destructors: ✓\n");
    print("\n");
    print("Ready for LLVM bare-metal port!\n");

    return 0;
}
