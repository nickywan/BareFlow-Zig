// ============================================================================
// BAREFLOW - JIT Demo Header
// ============================================================================

#ifndef JIT_DEMO_H
#define JIT_DEMO_H

/**
 * End-to-end JIT demonstration
 * Shows: Pattern detection → Micro-JIT compilation → Adaptive optimization
 */
void jit_demo_disk_to_jit(void);

/**
 * Helper: Integer to string conversion
 */
void itoa(int value, char* str, int base);

#endif // JIT_DEMO_H
