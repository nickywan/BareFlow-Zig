/**
 * System Call Stubs for LLVM
 *
 * Bare-metal stubs for system functions that LLVM may reference:
 * - I/O functions (fprintf, fwrite, etc.)
 * - Program termination (abort, exit)
 * - File operations (minimal stubs)
 * - Threading (single-threaded stubs)
 */

#include <stddef.h>
#include <stdarg.h>

// ============================================================================
// I/O Stubs
// ============================================================================

// For bare-metal, we'll redirect fprintf to serial port
// For now, just stub them out

// FILE structure (minimal)
struct __sFILE {
    int _file;
    // We don't actually use FILE* in bare-metal
};

typedef struct __sFILE FILE;

// Standard streams (dummy pointers)
static FILE __stdio_streams[3];
FILE* stdin  = &__stdio_streams[0];
FILE* stdout = &__stdio_streams[1];
FILE* stderr = &__stdio_streams[2];

extern "C" {

// fprintf - print formatted to stream
int fprintf(FILE* stream, const char* format, ...) {
    (void)stream;
    (void)format;
    // In bare-metal, redirect to serial_printf or ignore
    // For now, no-op
    return 0;
}

// vfprintf - variadic version
int vfprintf(FILE* stream, const char* format, va_list ap) {
    (void)stream;
    (void)format;
    (void)ap;
    return 0;
}

// fwrite - write to stream
size_t fwrite(const void* ptr, size_t size, size_t nmemb, FILE* stream) {
    (void)ptr;
    (void)size;
    (void)stream;
    return nmemb; // Pretend success
}

// fflush - flush stream
int fflush(FILE* stream) {
    (void)stream;
    return 0; // Success
}

// fputc - write character
int fputc(int c, FILE* stream) {
    (void)c;
    (void)stream;
    return c;
}

// fputs - write string
int fputs(const char* s, FILE* stream) {
    (void)s;
    (void)stream;
    return 0;
}

// ============================================================================
// Program Termination
// ============================================================================

// abort - abnormal termination
void abort() {
    // In bare-metal, infinite loop or halt
    while (1) {
        // Could trigger breakpoint or halt CPU
        __asm__ volatile("hlt");
    }
}

// exit - normal termination
void exit(int status) {
    (void)status;
    // In bare-metal, this should never be called
    while (1) {
        __asm__ volatile("hlt");
    }
}

// _Exit - immediate termination
void _Exit(int status) {
    exit(status);
}

// ============================================================================
// Threading Stubs (Single-threaded)
// ============================================================================

// pthread types (minimal)
typedef struct { int dummy; } pthread_mutex_t;
typedef struct { int dummy; } pthread_mutexattr_t;
typedef unsigned long pthread_t;

// pthread_self - get thread ID
pthread_t pthread_self() {
    return 1; // Always thread 1 (single-threaded)
}

// pthread_mutex_init - initialize mutex
int pthread_mutex_init(pthread_mutex_t* mutex, const pthread_mutexattr_t* attr) {
    (void)mutex;
    (void)attr;
    return 0; // Success (no-op in single-threaded)
}

// pthread_mutex_lock - lock mutex
int pthread_mutex_lock(pthread_mutex_t* mutex) {
    (void)mutex;
    return 0; // Success (no-op)
}

// pthread_mutex_unlock - unlock mutex
int pthread_mutex_unlock(pthread_mutex_t* mutex) {
    (void)mutex;
    return 0; // Success (no-op)
}

// pthread_mutex_destroy - destroy mutex
int pthread_mutex_destroy(pthread_mutex_t* mutex) {
    (void)mutex;
    return 0; // Success (no-op)
}

// ============================================================================
// Environment / System Info
// ============================================================================

// getenv - get environment variable
char* getenv(const char* name) {
    (void)name;
    return nullptr; // No environment in bare-metal
}

// sysconf - get system configuration
long sysconf(int name) {
    (void)name;
    // Return sensible defaults
    switch (name) {
        case 30:  // _SC_PAGE_SIZE
            return 4096;
        case 84:  // _SC_NPROCESSORS_ONLN
            return 1; // Single CPU
        default:
            return -1;
    }
}

// ============================================================================
// Time Functions (Minimal)
// ============================================================================

typedef long time_t;
typedef long clock_t;

struct tm {
    int tm_sec;    // seconds
    int tm_min;    // minutes
    int tm_hour;   // hours
    int tm_mday;   // day of month
    int tm_mon;    // month
    int tm_year;   // year since 1900
    int tm_wday;   // day of week
    int tm_yday;   // day of year
    int tm_isdst;  // DST flag
};

// time - get current time
time_t time(time_t* tloc) {
    time_t t = 0; // Epoch (no RTC in bare-metal)
    if (tloc) {
        *tloc = t;
    }
    return t;
}

// localtime - convert time to local time
struct tm* localtime(const time_t* timep) {
    static struct tm tm_buf;
    (void)timep;
    // Return epoch time
    tm_buf.tm_sec = 0;
    tm_buf.tm_min = 0;
    tm_buf.tm_hour = 0;
    tm_buf.tm_mday = 1;
    tm_buf.tm_mon = 0;
    tm_buf.tm_year = 70; // 1970
    tm_buf.tm_wday = 4;  // Thursday
    tm_buf.tm_yday = 0;
    tm_buf.tm_isdst = 0;
    return &tm_buf;
}

// clock - get CPU time
clock_t clock() {
    // Could use rdtsc here
    return 0;
}

// ============================================================================
// Memory Functions (if not provided elsewhere)
// ============================================================================

// mmap - map memory (not supported in bare-metal)
void* mmap(void* addr, size_t length, int prot, int flags, int fd, long offset) {
    (void)addr;
    (void)length;
    (void)prot;
    (void)flags;
    (void)fd;
    (void)offset;
    return (void*)-1; // MAP_FAILED
}

// munmap - unmap memory
int munmap(void* addr, size_t length) {
    (void)addr;
    (void)length;
    return 0; // Success (no-op)
}

// ============================================================================
// Signal Handling (Stubs)
// ============================================================================

typedef void (*sighandler_t)(int);

// signal - set signal handler
sighandler_t signal(int signum, sighandler_t handler) {
    (void)signum;
    (void)handler;
    return nullptr; // No signal support
}

// ============================================================================
// Error Handling
// ============================================================================

// errno - error number
int errno = 0;

// strerror - get error string
char* strerror(int errnum) {
    (void)errnum;
    static char buf[] = "Unknown error";
    return buf;
}

// perror - print error
void perror(const char* s) {
    (void)s;
    // Could print to serial, but stub for now
}

} // extern "C"
