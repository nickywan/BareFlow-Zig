// ============================================================================
// BAREFLOW - FAT16 Read-Only Filesystem Implementation
// ============================================================================

#include "fat16.h"
#include <stddef.h>

// External functions
extern void* memset(void* s, int c, size_t n);
extern void* memcpy(void* dest, const void* src, size_t n);
extern int strcmp(const char* s1, const char* s2);
extern size_t strlen(const char* s);
extern void* malloc(size_t size);
extern void free(void* ptr);

// VGA output (for debugging)
extern void terminal_writestring(const char* str);

// ============================================================================
// Disk I/O (Protected Mode - Port I/O)
// ============================================================================

// ATA/IDE port addresses (Primary IDE controller)
#define ATA_DATA        0x1F0
#define ATA_ERROR       0x1F1
#define ATA_SECTOR_COUNT 0x1F2
#define ATA_LBA_LOW     0x1F3
#define ATA_LBA_MID     0x1F4
#define ATA_LBA_HIGH    0x1F5
#define ATA_DRIVE       0x1F6
#define ATA_STATUS      0x1F7
#define ATA_COMMAND     0x1F7

// ATA commands
#define ATA_CMD_READ_SECTORS 0x20

// ATA status bits
#define ATA_SR_BSY  0x80    // Busy
#define ATA_SR_DRDY 0x40    // Drive ready
#define ATA_SR_DRQ  0x08    // Data request ready
#define ATA_SR_ERR  0x01    // Error

// Port I/O functions
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ __volatile__("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ __volatile__("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void inw_rep(uint16_t port, uint16_t* buffer, uint32_t count) {
    __asm__ __volatile__("rep insw" : "+D"(buffer), "+c"(count) : "d"(port) : "memory");
}

// Wait for drive to be ready
static int ata_wait_ready(void) {
    for (int i = 0; i < 100000; i++) {
        uint8_t status = inb(ATA_STATUS);
        if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRDY)) {
            return 0;  // Ready
        }
    }
    return -1;  // Timeout
}

// Read sector using ATA PIO mode
static int ata_read_sector(uint32_t lba, void* buffer, uint8_t drive) {
    // Wait for drive to be ready
    if (ata_wait_ready() != 0) {
        return -1;
    }

    // Select drive and set LBA mode
    // 0xE0 = Master (drive 0), 0xF0 = Slave (drive 1)
    uint8_t drive_select = (drive == 0) ? 0xE0 : 0xF0;
    outb(ATA_DRIVE, drive_select | ((lba >> 24) & 0x0F));

    // Set sector count
    outb(ATA_SECTOR_COUNT, 1);

    // Set LBA
    outb(ATA_LBA_LOW, (uint8_t)(lba));
    outb(ATA_LBA_MID, (uint8_t)(lba >> 8));
    outb(ATA_LBA_HIGH, (uint8_t)(lba >> 16));

    // Send READ command
    outb(ATA_COMMAND, ATA_CMD_READ_SECTORS);

    // Wait for data to be ready
    for (int i = 0; i < 100000; i++) {
        uint8_t status = inb(ATA_STATUS);
        if (status & ATA_SR_ERR) {
            return -1;  // Error
        }
        if (status & ATA_SR_DRQ) {
            // Data ready, read 256 words (512 bytes)
            inw_rep(ATA_DATA, (uint16_t*)buffer, 256);
            return 0;
        }
    }

    return -1;  // Timeout
}

// ============================================================================
// FAT16 Implementation
// ============================================================================

int fat16_read_sector(fat16_fs_t* fs, uint32_t lba, void* buffer) {
    if (!fs || !buffer) return -1;
    return ata_read_sector(lba, buffer, fs->drive_number);
}

int fat16_init(fat16_fs_t* fs, uint8_t drive_number, uint32_t partition_lba) {
    if (!fs) return -1;

    memset(fs, 0, sizeof(fat16_fs_t));
    fs->drive_number = drive_number;

    // Read boot sector
    if (fat16_read_sector(fs, partition_lba, &fs->boot_sector) != 0) {
        terminal_writestring("[FAT16] Failed to read boot sector\n");
        return -1;
    }

    // Validate FAT16 signature
    if (fs->boot_sector.bytes_per_sector != 512) {
        terminal_writestring("[FAT16] Invalid bytes per sector\n");
        return -1;
    }

    // Calculate filesystem geometry
    fs->fat_start_sector = partition_lba + fs->boot_sector.reserved_sectors;

    fs->root_dir_sectors = ((fs->boot_sector.root_entries * 32) +
                            (fs->boot_sector.bytes_per_sector - 1)) /
                            fs->boot_sector.bytes_per_sector;

    fs->root_dir_start_sector = fs->fat_start_sector +
                                (fs->boot_sector.num_fats * fs->boot_sector.sectors_per_fat);

    fs->data_start_sector = fs->root_dir_start_sector + fs->root_dir_sectors;

    // Get total sectors
    if (fs->boot_sector.total_sectors_16 != 0) {
        fs->total_sectors = fs->boot_sector.total_sectors_16;
    } else {
        fs->total_sectors = fs->boot_sector.total_sectors_32;
    }

    fs->fat_cached = false;

    terminal_writestring("[FAT16] Initialized successfully\n");
    return 0;
}

void fat16_filename_to_83(const char* filename, char* fat_name) {
    // Initialize with spaces
    for (int i = 0; i < 11; i++) {
        fat_name[i] = ' ';
    }

    int name_idx = 0;
    int ext_idx = 8;
    bool in_ext = false;

    for (int i = 0; filename[i] != '\0' && i < FAT16_MAX_PATH; i++) {
        if (filename[i] == '.') {
            in_ext = true;
            continue;
        }

        char c = filename[i];
        // Convert to uppercase
        if (c >= 'a' && c <= 'z') {
            c = c - 'a' + 'A';
        }

        if (!in_ext && name_idx < 8) {
            fat_name[name_idx++] = c;
        } else if (in_ext && ext_idx < 11) {
            fat_name[ext_idx++] = c;
        }
    }
}

int fat16_find_file(fat16_fs_t* fs, const char* filename, fat16_file_t* file) {
    if (!fs || !filename || !file) return -1;

    // Convert filename to FAT 8.3 format
    char fat_name[11];
    fat16_filename_to_83(filename, fat_name);

    // Search root directory
    for (uint32_t sector = 0; sector < fs->root_dir_sectors; sector++) {
        if (fat16_read_sector(fs, fs->root_dir_start_sector + sector, fs->sector_buffer) != 0) {
            return -1;
        }

        fat16_dir_entry_t* entries = (fat16_dir_entry_t*)fs->sector_buffer;
        uint32_t entries_per_sector = FAT16_SECTOR_SIZE / sizeof(fat16_dir_entry_t);

        for (uint32_t i = 0; i < entries_per_sector; i++) {
            // Check if entry is empty (0x00) or deleted (0xE5)
            if ((uint8_t)entries[i].name[0] == 0x00) {
                return -1;  // End of directory
            }
            if ((uint8_t)entries[i].name[0] == 0xE5) {
                continue;  // Deleted entry
            }

            // Skip volume labels and directories
            if (entries[i].attributes & (FAT16_ATTR_VOLUME_ID | FAT16_ATTR_DIRECTORY)) {
                continue;
            }

            // Compare filenames
            bool match = true;
            for (int j = 0; j < 11; j++) {
                if (entries[i].name[j] != fat_name[j]) {
                    match = false;
                    break;
                }
            }

            if (match) {
                // Found the file!
                memcpy(&file->dir_entry, &entries[i], sizeof(fat16_dir_entry_t));
                file->current_position = 0;
                file->current_cluster = entries[i].first_cluster;
                file->cluster_offset = 0;
                return 0;
            }
        }
    }

    return -1;  // Not found
}

int fat16_open(fat16_fs_t* fs, const char* filename, fat16_file_t* file) {
    return fat16_find_file(fs, filename, file);
}

uint32_t fat16_cluster_to_lba(fat16_fs_t* fs, uint16_t cluster) {
    return fs->data_start_sector +
           ((cluster - 2) * fs->boot_sector.sectors_per_cluster);
}

uint16_t fat16_get_next_cluster(fat16_fs_t* fs, uint16_t cluster) {
    // Calculate FAT offset
    uint32_t fat_offset = cluster * 2;  // Each FAT16 entry is 2 bytes
    uint32_t fat_sector = fs->fat_start_sector + (fat_offset / FAT16_SECTOR_SIZE);
    uint32_t entry_offset = fat_offset % FAT16_SECTOR_SIZE;

    // Read FAT sector
    if (fat16_read_sector(fs, fat_sector, fs->sector_buffer) != 0) {
        return FAT16_CLUSTER_EOF;
    }

    // Get next cluster
    uint16_t next_cluster = *(uint16_t*)(&fs->sector_buffer[entry_offset]);

    // Check for EOF
    if (next_cluster >= FAT16_CLUSTER_EOF) {
        return FAT16_CLUSTER_EOF;
    }

    return next_cluster;
}

int fat16_read(fat16_fs_t* fs, fat16_file_t* file, void* buffer, uint32_t size) {
    if (!fs || !file || !buffer) return -1;

    uint32_t bytes_read = 0;
    uint8_t* buf_ptr = (uint8_t*)buffer;

    uint32_t bytes_per_cluster = fs->boot_sector.sectors_per_cluster * FAT16_SECTOR_SIZE;

    while (bytes_read < size && file->current_position < file->dir_entry.file_size) {
        // Check if we need to move to next cluster
        if (file->cluster_offset >= bytes_per_cluster) {
            file->current_cluster = fat16_get_next_cluster(fs, file->current_cluster);
            if (file->current_cluster >= FAT16_CLUSTER_EOF) {
                break;  // End of file
            }
            file->cluster_offset = 0;
        }

        // Read current cluster sector
        uint32_t sector_in_cluster = file->cluster_offset / FAT16_SECTOR_SIZE;
        uint32_t offset_in_sector = file->cluster_offset % FAT16_SECTOR_SIZE;

        uint32_t lba = fat16_cluster_to_lba(fs, file->current_cluster) + sector_in_cluster;

        if (fat16_read_sector(fs, lba, fs->sector_buffer) != 0) {
            return -1;
        }

        // Copy data from sector buffer
        uint32_t bytes_to_copy = FAT16_SECTOR_SIZE - offset_in_sector;
        if (bytes_to_copy > size - bytes_read) {
            bytes_to_copy = size - bytes_read;
        }
        if (file->current_position + bytes_to_copy > file->dir_entry.file_size) {
            bytes_to_copy = file->dir_entry.file_size - file->current_position;
        }

        memcpy(buf_ptr, &fs->sector_buffer[offset_in_sector], bytes_to_copy);

        buf_ptr += bytes_to_copy;
        bytes_read += bytes_to_copy;
        file->current_position += bytes_to_copy;
        file->cluster_offset += bytes_to_copy;
    }

    return bytes_read;
}

uint32_t fat16_get_file_size(fat16_file_t* file) {
    return file ? file->dir_entry.file_size : 0;
}

void fat16_close(fat16_file_t* file) {
    if (file) {
        memset(file, 0, sizeof(fat16_file_t));
    }
}

void fat16_list_files(fat16_fs_t* fs) {
    if (!fs) return;

    terminal_writestring("\n=== FAT16 Root Directory ===\n");

    for (uint32_t sector = 0; sector < fs->root_dir_sectors; sector++) {
        if (fat16_read_sector(fs, fs->root_dir_start_sector + sector, fs->sector_buffer) != 0) {
            return;
        }

        fat16_dir_entry_t* entries = (fat16_dir_entry_t*)fs->sector_buffer;
        uint32_t entries_per_sector = FAT16_SECTOR_SIZE / sizeof(fat16_dir_entry_t);

        for (uint32_t i = 0; i < entries_per_sector; i++) {
            if ((uint8_t)entries[i].name[0] == 0x00) return;  // End of directory
            if ((uint8_t)entries[i].name[0] == 0xE5) continue;  // Deleted

            // Skip volume labels
            if (entries[i].attributes & FAT16_ATTR_VOLUME_ID) continue;

            // Print filename
            terminal_writestring("  ");
            for (int j = 0; j < 8; j++) {
                if (entries[i].name[j] != ' ') {
                    char c[2] = {entries[i].name[j], '\0'};
                    terminal_writestring(c);
                }
            }
            if (entries[i].name[8] != ' ') {
                terminal_writestring(".");
                for (int j = 8; j < 11; j++) {
                    if (entries[i].name[j] != ' ') {
                        char c[2] = {entries[i].name[j], '\0'};
                        terminal_writestring(c);
                    }
                }
            }

            // Print size
            terminal_writestring(" (");
            // Simple size printing - would need proper number formatting
            terminal_writestring(" bytes)\n");
        }
    }
}
