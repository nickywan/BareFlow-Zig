/**
 * C++ Runtime - Exception Handling Stubs
 *
 * Bare-metal environment with -fno-exceptions.
 * These are no-op stubs for symbols the compiler may reference.
 */

#include <stddef.h>

// ============================================================================
// Exception Handling Stubs (No-op)
// ============================================================================

extern "C" {

// Called when an exception is thrown (should never happen with -fno-exceptions)
void __cxa_throw(void* thrown_exception, void* tinfo, void (*dest)(void*)) {
    // In bare-metal with -fno-exceptions, this should never be called
    // Infinite loop to catch bugs
    while (1) {}
}

// Called to begin catching an exception
void* __cxa_begin_catch(void* exceptionObject) {
    return exceptionObject;
}

// Called to end catching an exception
void __cxa_end_catch() {
    // No-op
}

// Called for rethrow
void __cxa_rethrow() {
    while (1) {}
}

// Called to allocate exception object
void* __cxa_allocate_exception(size_t thrown_size) {
    (void)thrown_size;
    return nullptr;
}

// Called to free exception object
void __cxa_free_exception(void* thrown_exception) {
    (void)thrown_exception;
}

// Pure virtual function call (should never happen in correct code)
void __cxa_pure_virtual() {
    // Called when a pure virtual function is invoked
    // This is a bug - infinite loop to catch it
    while (1) {}
}

// Deleted virtual function call
void __cxa_deleted_virtual() {
    while (1) {}
}

} // extern "C"
