// ============================================================================
// FAT16 Filesystem Test Header
// ============================================================================

#ifndef FAT16_TEST_H
#define FAT16_TEST_H

/**
 * Test FAT16 filesystem initialization and file operations
 * This function will:
 * - Initialize FAT16 filesystem on primary IDE drive
 * - Display filesystem information
 * - List files in root directory
 * - Attempt to read a test file (TEST.TXT)
 */
void test_fat16_filesystem(void);

#endif // FAT16_TEST_H
