/**
 * Minimal C++ Runtime for Bare-Metal Environment
 *
 * Provides essential C++ runtime support:
 * - Memory allocation (new/delete)
 * - Global constructor/destructor support
 * - Virtual function support
 * - Exception stubs (if needed)
 */

#include "cxx_runtime.h"

// C function declarations (to avoid C++ name mangling)
extern "C" {
    void* malloc(size_t size);
    void free(void* ptr);
    void terminal_writestring(const char* str);
}

// ============================================================================
// Memory Allocation Configuration
// ============================================================================

static cxx_alloc_fn g_alloc_fn = nullptr;
static cxx_free_fn g_free_fn = nullptr;

static cxx_alloc_stats_t g_alloc_stats = {0};

void cxx_set_allocator(cxx_alloc_fn alloc, cxx_free_fn free) {
    g_alloc_fn = alloc;
    g_free_fn = free;
}

static inline void* cxx_alloc(size_t size) {
    void* ptr;
    if (g_alloc_fn) {
        ptr = g_alloc_fn(size);
    } else {
        ptr = malloc(size);
    }

    if (ptr) {
        g_alloc_stats.total_allocated += size;
        g_alloc_stats.current_used += size;
        g_alloc_stats.num_allocations++;
    }

    return ptr;
}

static inline void cxx_free(void* ptr, size_t size) {
    if (!ptr) return;

    if (g_free_fn) {
        g_free_fn(ptr);
    } else {
        free(ptr);
    }

    g_alloc_stats.total_freed += size;
    g_alloc_stats.current_used -= size;
    g_alloc_stats.num_deallocations++;
}

void cxx_get_alloc_stats(cxx_alloc_stats_t* stats) {
    if (stats) {
        *stats = g_alloc_stats;
    }
}

// ============================================================================
// C++ Operator new/delete
// ============================================================================

void* operator new(size_t size) {
    void* ptr = cxx_alloc(size);
    if (!ptr) {
        terminal_writestring("FATAL: operator new failed\n");
        while(1) { asm volatile("hlt"); }
    }
    return ptr;
}

void* operator new(size_t size, void* ptr) noexcept {
    // Placement new - just return the pointer
    (void)size;
    return ptr;
}

void operator delete(void* ptr) noexcept {
    // We don't know the size, pass 0
    cxx_free(ptr, 0);
}

void operator delete(void* ptr, size_t size) noexcept {
    cxx_free(ptr, size);
}

void* operator new[](size_t size) {
    return operator new(size);
}

void operator delete[](void* ptr) noexcept {
    operator delete(ptr);
}

void operator delete[](void* ptr, size_t size) noexcept {
    operator delete(ptr, size);
}

// ============================================================================
// C++ ABI Functions
// ============================================================================

extern "C" {

/**
 * Called when a pure virtual function is called
 * This should never happen in correct code
 */
void __cxa_pure_virtual() {
    terminal_writestring("FATAL: Pure virtual function called\n");
    while(1) { asm volatile("hlt"); }
}

/**
 * Called to register a destructor for a global object
 * We maintain a simple list of destructors to call
 */
#define MAX_ATEXIT 128

typedef void (*destructor_fn)(void*);

struct atexit_entry {
    destructor_fn func;
    void* arg;
    void* dso_handle;
};

static atexit_entry g_atexit_funcs[MAX_ATEXIT];
static int g_atexit_count = 0;

int __cxa_atexit(destructor_fn func, void* arg, void* dso_handle) {
    if (g_atexit_count >= MAX_ATEXIT) {
        return -1;
    }

    g_atexit_funcs[g_atexit_count].func = func;
    g_atexit_funcs[g_atexit_count].arg = arg;
    g_atexit_funcs[g_atexit_count].dso_handle = dso_handle;
    g_atexit_count++;

    return 0;
}

void __cxa_finalize(void* dso_handle) {
    // Call destructors in reverse order
    for (int i = g_atexit_count - 1; i >= 0; i--) {
        if (dso_handle == nullptr || g_atexit_funcs[i].dso_handle == dso_handle) {
            if (g_atexit_funcs[i].func) {
                g_atexit_funcs[i].func(g_atexit_funcs[i].arg);
            }
        }
    }
}

/**
 * Guard variables for thread-safe initialization of local statics
 * In bare-metal single-threaded environment, these are simple
 */
int __cxa_guard_acquire(uint64_t* guard) {
    return !*(char*)guard;
}

void __cxa_guard_release(uint64_t* guard) {
    *(char*)guard = 1;
}

void __cxa_guard_abort(uint64_t* guard) {
    // Nothing to do in bare-metal
    (void)guard;
}

} // extern "C"

// ============================================================================
// Exception Support (Stubs)
// ============================================================================
// If compiling with -fno-exceptions, these may not be needed
// But we provide stubs just in case

extern "C" {

void __cxa_throw(void* thrown_exception, void* tinfo, void (*dest)(void*)) {
    (void)thrown_exception;
    (void)tinfo;
    (void)dest;
    terminal_writestring("FATAL: Exception thrown (exceptions not supported)\n");
    while(1) { asm volatile("hlt"); }
}

void* __cxa_begin_catch(void* exception) {
    (void)exception;
    terminal_writestring("FATAL: Exception caught (exceptions not supported)\n");
    while(1) { asm volatile("hlt"); }
    return nullptr;
}

void __cxa_end_catch() {
    terminal_writestring("FATAL: End catch (exceptions not supported)\n");
    while(1) { asm volatile("hlt"); }
}

void* __cxa_allocate_exception(size_t size) {
    (void)size;
    terminal_writestring("FATAL: Exception allocation (exceptions not supported)\n");
    while(1) { asm volatile("hlt"); }
    return nullptr;
}

void __cxa_free_exception(void* thrown_exception) {
    (void)thrown_exception;
}

} // extern "C"

// ============================================================================
// Global Constructor/Destructor Support
// ============================================================================

// These symbols are provided by the linker script
typedef void (*constructor_fn)(void);
extern constructor_fn __init_array_start[];
extern constructor_fn __init_array_end[];
extern constructor_fn __fini_array_start[];
extern constructor_fn __fini_array_end[];

extern "C" void cxx_runtime_init(void) {
    // Call all global constructors
    size_t count = __init_array_end - __init_array_start;
    for (size_t i = 0; i < count; i++) {
        if (__init_array_start[i]) {
            __init_array_start[i]();
        }
    }
}

extern "C" void cxx_runtime_fini(void) {
    // Call all global destructors
    size_t count = __fini_array_end - __fini_array_start;
    for (size_t i = 0; i < count; i++) {
        if (__fini_array_start[i]) {
            __fini_array_start[i]();
        }
    }

    // Also call atexit functions
    __cxa_finalize(nullptr);
}

// ============================================================================
// Personality Function for Exception Handling
// ============================================================================
// Required by GCC for exception handling, even with -fno-exceptions

extern "C" {

#if defined(__GNUC__) && !defined(__clang__)
// GCC Itanium ABI personality function
void* _Unwind_Resume = nullptr;

typedef int _Unwind_Reason_Code;

_Unwind_Reason_Code __gxx_personality_v0(
    int version,
    int actions,
    uint64_t exception_class,
    void* exception_object,
    void* context)
{
    (void)version;
    (void)actions;
    (void)exception_class;
    (void)exception_object;
    (void)context;

    terminal_writestring("FATAL: Exception personality function called\n");
    while(1) { asm volatile("hlt"); }
    return 0;
}
#endif

} // extern "C"
