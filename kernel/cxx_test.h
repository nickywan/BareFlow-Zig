#ifndef CXX_TEST_H
#define CXX_TEST_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Test C++ runtime functionality
 * Tests: new/delete, constructors/destructors, virtual functions
 *
 * Returns: 0 on success, non-zero on failure
 */
int test_cxx_runtime(void);

#ifdef __cplusplus
}
#endif

#endif // CXX_TEST_H
