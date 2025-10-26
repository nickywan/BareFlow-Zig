/**
 * C++ Runtime Test Suite
 *
 * Tests all C++ runtime features:
 * - new/delete operators
 * - Constructors/destructors
 * - Virtual functions
 * - Static initialization
 */

#include "cxx_test.h"
#include "cxx_runtime.h"

// C function declarations
extern "C" {
    void terminal_writestring(const char* str);
}

// ============================================================================
// Test Classes
// ============================================================================

// Simple class with constructor/destructor
class TestObject {
public:
    int value;

    TestObject(int v) : value(v) {
        terminal_writestring("  TestObject constructor called (value=");
        print_test_int(value);
        terminal_writestring(")\n");
    }

    ~TestObject() {
        terminal_writestring("  TestObject destructor called (value=");
        print_test_int(value);
        terminal_writestring(")\n");
    }

    int getValue() const { return value; }

private:
    void print_test_int(int n) {
        if (n == 0) {
            terminal_writestring("0");
            return;
        }
        if (n < 0) {
            terminal_writestring("-");
            n = -n;
        }
        char buf[12];
        int i = 0;
        while (n > 0) {
            buf[i++] = '0' + (n % 10);
            n /= 10;
        }
        while (i > 0) {
            char c[2] = {buf[--i], 0};
            terminal_writestring(c);
        }
    }
};

// Base class with virtual function
class Base {
public:
    virtual ~Base() {
        terminal_writestring("  Base destructor\n");
    }

    virtual int compute() {
        return 42;
    }
};

// Derived class
class Derived : public Base {
public:
    ~Derived() override {
        terminal_writestring("  Derived destructor\n");
    }

    int compute() override {
        return 100;
    }
};

// Global object to test static initialization
static int g_static_init_value = 0;

class StaticInitTest {
public:
    StaticInitTest() {
        g_static_init_value = 999;
        terminal_writestring("  Global constructor called\n");
    }
};

static StaticInitTest g_static_test;

// ============================================================================
// Test Functions
// ============================================================================

static bool test_basic_new_delete() {
    terminal_writestring("\n[Test 1] Basic new/delete\n");

    TestObject* obj = new TestObject(42);
    if (!obj) {
        terminal_writestring("  FAIL: new returned nullptr\n");
        return false;
    }

    if (obj->getValue() != 42) {
        terminal_writestring("  FAIL: getValue() returned wrong value\n");
        delete obj;
        return false;
    }

    delete obj;

    terminal_writestring("  PASS\n");
    return true;
}

static bool test_array_new_delete() {
    terminal_writestring("\n[Test 2] Array new[]/delete[]\n");

    TestObject* arr = new TestObject[3]{TestObject(1), TestObject(2), TestObject(3)};
    if (!arr) {
        terminal_writestring("  FAIL: new[] returned nullptr\n");
        return false;
    }

    delete[] arr;

    terminal_writestring("  PASS\n");
    return true;
}

static bool test_virtual_functions() {
    terminal_writestring("\n[Test 3] Virtual functions\n");

    Base* basePtr = new Derived();
    if (!basePtr) {
        terminal_writestring("  FAIL: new returned nullptr\n");
        return false;
    }

    int result = basePtr->compute();
    if (result != 100) {
        terminal_writestring("  FAIL: Virtual function returned wrong value\n");
        delete basePtr;
        return false;
    }

    delete basePtr;

    terminal_writestring("  PASS\n");
    return true;
}

static bool test_static_initialization() {
    terminal_writestring("\n[Test 4] Static initialization\n");

    if (g_static_init_value != 999) {
        terminal_writestring("  FAIL: Global constructor not called\n");
        return false;
    }

    terminal_writestring("  PASS: Global constructor executed before main\n");
    return true;
}

static bool test_placement_new() {
    terminal_writestring("\n[Test 5] Placement new\n");

    // Allocate memory for object manually
    char buffer[sizeof(TestObject)];

    // Use placement new to construct object in buffer
    TestObject* obj = new (buffer) TestObject(77);

    if (obj->getValue() != 77) {
        terminal_writestring("  FAIL: Placement new failed\n");
        return false;
    }

    // Manually call destructor (placement new doesn't call delete)
    obj->~TestObject();

    terminal_writestring("  PASS\n");
    return true;
}

static bool test_allocation_stats() {
    terminal_writestring("\n[Test 6] Allocation statistics\n");

    cxx_alloc_stats_t stats_before;
    cxx_get_alloc_stats(&stats_before);

    TestObject* obj = new TestObject(123);
    delete obj;

    cxx_alloc_stats_t stats_after;
    cxx_get_alloc_stats(&stats_after);

    if (stats_after.num_allocations <= stats_before.num_allocations) {
        terminal_writestring("  FAIL: Allocation count not increased\n");
        return false;
    }

    terminal_writestring("  PASS\n");
    return true;
}

// ============================================================================
// Main Test Entry Point
// ============================================================================

extern "C" int test_cxx_runtime(void) {
    terminal_writestring("\n");
    terminal_writestring("========================================\n");
    terminal_writestring("  C++ Runtime Test Suite\n");
    terminal_writestring("========================================\n");

    int passed = 0;
    int total = 6;

    if (test_static_initialization()) passed++;
    if (test_basic_new_delete()) passed++;
    if (test_array_new_delete()) passed++;
    if (test_virtual_functions()) passed++;
    if (test_placement_new()) passed++;
    if (test_allocation_stats()) passed++;

    terminal_writestring("\n========================================\n");
    terminal_writestring("  Results: ");

    // Print results
    char result[32];
    int idx = 0;
    int p = passed;
    while (p > 0) {
        result[idx++] = '0' + (p % 10);
        p /= 10;
    }
    if (idx == 0) result[idx++] = '0';
    while (idx > 0) {
        char c[2] = {result[--idx], 0};
        terminal_writestring(c);
    }

    terminal_writestring(" / ");

    idx = 0;
    int t = total;
    while (t > 0) {
        result[idx++] = '0' + (t % 10);
        t /= 10;
    }
    while (idx > 0) {
        char c[2] = {result[--idx], 0};
        terminal_writestring(c);
    }

    terminal_writestring(" tests passed\n");
    terminal_writestring("========================================\n\n");

    return (passed == total) ? 0 : 1;
}
