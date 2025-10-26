// ============================================================================
// BAREFLOW - FAT16 Read-Only Filesystem Driver
// ============================================================================
// File: kernel/fat16.h
// Purpose: Minimal FAT16 driver for loading modules and model from disk
// ============================================================================

#ifndef FAT16_H
#define FAT16_H

#include <stdint.h>
#include <stdbool.h>

// FAT16 constants
#define FAT16_SECTOR_SIZE       512
#define FAT16_MAX_FILENAME      11      // 8.3 format
#define FAT16_MAX_PATH          256
#define FAT16_CLUSTER_FREE      0x0000
#define FAT16_CLUSTER_EOF       0xFFF8  // End of file marker

// File attributes
#define FAT16_ATTR_READ_ONLY    0x01
#define FAT16_ATTR_HIDDEN       0x02
#define FAT16_ATTR_SYSTEM       0x04
#define FAT16_ATTR_VOLUME_ID    0x08
#define FAT16_ATTR_DIRECTORY    0x10
#define FAT16_ATTR_ARCHIVE      0x20

// FAT16 Boot Sector (BIOS Parameter Block)
typedef struct __attribute__((packed)) {
    uint8_t  jump[3];               // Jump instruction
    char     oem_name[8];           // OEM name
    uint16_t bytes_per_sector;      // Bytes per sector (usually 512)
    uint8_t  sectors_per_cluster;   // Sectors per cluster
    uint16_t reserved_sectors;      // Reserved sectors (usually 1)
    uint8_t  num_fats;              // Number of FATs (usually 2)
    uint16_t root_entries;          // Max root directory entries
    uint16_t total_sectors_16;      // Total sectors (if < 65536)
    uint8_t  media_descriptor;      // Media descriptor
    uint16_t sectors_per_fat;       // Sectors per FAT
    uint16_t sectors_per_track;     // Sectors per track
    uint16_t num_heads;             // Number of heads
    uint32_t hidden_sectors;        // Hidden sectors
    uint32_t total_sectors_32;      // Total sectors (if >= 65536)

    // Extended boot record
    uint8_t  drive_number;          // Drive number
    uint8_t  reserved;              // Reserved
    uint8_t  boot_signature;        // Boot signature (0x29)
    uint32_t volume_id;             // Volume ID
    char     volume_label[11];      // Volume label
    char     fs_type[8];            // File system type ("FAT16   ")
} fat16_boot_sector_t;

// FAT16 Directory Entry
typedef struct __attribute__((packed)) {
    char     name[11];              // Filename (8.3 format, space-padded)
    uint8_t  attributes;            // File attributes
    uint8_t  reserved;              // Reserved
    uint8_t  create_time_tenth;     // Creation time (tenths of second)
    uint16_t create_time;           // Creation time
    uint16_t create_date;           // Creation date
    uint16_t access_date;           // Last access date
    uint16_t first_cluster_high;    // High word of first cluster (FAT32)
    uint16_t modify_time;           // Last modification time
    uint16_t modify_date;           // Last modification date
    uint16_t first_cluster;         // First cluster (low word)
    uint32_t file_size;             // File size in bytes
} fat16_dir_entry_t;

// FAT16 Filesystem State
typedef struct {
    fat16_boot_sector_t boot_sector;

    // Computed values
    uint32_t fat_start_sector;      // First FAT sector
    uint32_t root_dir_start_sector; // Root directory start
    uint32_t data_start_sector;     // Data area start
    uint32_t total_sectors;         // Total sectors
    uint32_t root_dir_sectors;      // Sectors occupied by root dir

    // Buffers
    uint8_t  sector_buffer[FAT16_SECTOR_SIZE];  // General sector buffer
    uint16_t* fat_cache;            // FAT cache (dynamically allocated)
    bool     fat_cached;            // Is FAT cached in memory?

    // Disk access
    uint8_t  drive_number;          // ATA drive number (0 = master, 1 = slave)
} fat16_fs_t;

// File handle for reading
typedef struct {
    fat16_dir_entry_t dir_entry;    // Directory entry
    uint32_t current_position;      // Current read position
    uint16_t current_cluster;       // Current cluster
    uint32_t cluster_offset;        // Offset within current cluster
} fat16_file_t;

// ============================================================================
// API Functions
// ============================================================================

/**
 * Initialize FAT16 filesystem
 * Returns: 0 on success, -1 on error
 */
int fat16_init(fat16_fs_t* fs, uint8_t drive_number, uint32_t partition_lba);

/**
 * Read a sector from disk (uses BIOS int 0x13 or direct I/O)
 * Returns: 0 on success, -1 on error
 */
int fat16_read_sector(fat16_fs_t* fs, uint32_t lba, void* buffer);

/**
 * Find a file in root directory
 * Returns: 0 on success, -1 if not found
 */
int fat16_find_file(fat16_fs_t* fs, const char* filename, fat16_file_t* file);

/**
 * Open a file for reading
 * Returns: 0 on success, -1 on error
 */
int fat16_open(fat16_fs_t* fs, const char* filename, fat16_file_t* file);

/**
 * Read bytes from file
 * Returns: number of bytes read, or -1 on error
 */
int fat16_read(fat16_fs_t* fs, fat16_file_t* file, void* buffer, uint32_t size);

/**
 * Get file size
 * Returns: file size in bytes
 */
uint32_t fat16_get_file_size(fat16_file_t* file);

/**
 * Close file (cleanup)
 */
void fat16_close(fat16_file_t* file);

/**
 * List files in root directory (for debugging)
 */
void fat16_list_files(fat16_fs_t* fs);

// ============================================================================
// Helper Functions
// ============================================================================

/**
 * Convert filename to FAT16 8.3 format
 * Example: "MODULE.MOD" -> "MODULE  MOD"
 */
void fat16_filename_to_83(const char* filename, char* fat_name);

/**
 * Get next cluster from FAT
 * Returns: next cluster number, or FAT16_CLUSTER_EOF if end
 */
uint16_t fat16_get_next_cluster(fat16_fs_t* fs, uint16_t cluster);

/**
 * Convert cluster number to LBA sector
 */
uint32_t fat16_cluster_to_lba(fat16_fs_t* fs, uint16_t cluster);

#endif // FAT16_H
