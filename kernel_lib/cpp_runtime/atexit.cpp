/**
 * C++ Runtime - atexit and Static Initialization
 *
 * Bare-metal stubs for program termination and static object destruction.
 * Since bare-metal programs never "exit", these are mostly no-ops.
 */

#include <stddef.h>

extern "C" {

// ============================================================================
// atexit() and __cxa_atexit()
// ============================================================================

// Called to register destructors for static objects
// In bare-metal, we don't support program exit, so this is a no-op
int __cxa_atexit(void (*destructor)(void*), void* arg, void* dso_handle) {
    (void)destructor;
    (void)arg;
    (void)dso_handle;
    return 0;  // Success
}

// Called during program exit to run destructors
// In bare-metal, this is never called
void __cxa_finalize(void* dso_handle) {
    (void)dso_handle;
}

// Thread-local version (we're single-threaded, so same as above)
int __cxa_thread_atexit_impl(void (*destructor)(void*), void* arg, void* dso_handle) {
    (void)destructor;
    (void)arg;
    (void)dso_handle;
    return 0;
}

// Traditional atexit() function
int atexit(void (*func)(void)) {
    (void)func;
    return 0;
}

// ============================================================================
// Static Initialization Guards
// ============================================================================

// These are used for thread-safe static local variable initialization
// In single-threaded bare-metal, we can simplify this

// Guard variable is 64-bit: 0 = uninitialized, 1 = initialized
int __cxa_guard_acquire(unsigned long long* guard) {
    if (*guard == 0) {
        // Not initialized yet, we'll initialize it
        return 1;
    }
    // Already initialized
    return 0;
}

void __cxa_guard_release(unsigned long long* guard) {
    *guard = 1;  // Mark as initialized
}

void __cxa_guard_abort(unsigned long long* guard) {
    (void)guard;
    // In case of exception during initialization
    // With -fno-exceptions, this shouldn't happen
}

} // extern "C"
