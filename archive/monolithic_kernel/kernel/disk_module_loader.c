// ============================================================================
// BAREFLOW - Disk Module Loader
// ============================================================================

#include "disk_module_loader.h"
#include "stdlib.h"
#include "vga.h"

// Load a single module from disk
int disk_load_module(module_manager_t* mgr, fat16_fs_t* fs, const char* filename) {
    // Open file
    fat16_file_t file;
    if (fat16_open(fs, filename, &file) != 0) {
        return -1;
    }

    // Get file size
    uint32_t size = fat16_get_file_size(&file);
    void* buffer = malloc(size);
    if (!buffer) {
        fat16_close(&file);
        return -1;
    }

    // Read module data
    int bytes_read = fat16_read(fs, &file, buffer, size);
    if (bytes_read != (int)size) {
        free(buffer);
        fat16_close(&file);
        return -1;
    }

    fat16_close(&file);

    // Load module using existing module_loader
    int result = module_load(mgr, buffer, size);

    // Note: We keep buffer allocated - module_loader uses it
    // TODO: Implement proper module unloading if needed

    return result;
}

// Load all .MOD files from root directory
int disk_load_all_modules(module_manager_t* mgr, fat16_fs_t* fs) {
    int loaded = 0;

    // List files and load .MOD files
    uint32_t num_entries = fs->boot_sector.root_entries;
    uint8_t sector_buffer[FAT16_SECTOR_SIZE];

    for (uint32_t sector = 0; sector < fs->root_dir_sectors; sector++) {
        if (fat16_read_sector(fs, fs->root_dir_start_sector + sector,
                              sector_buffer) != 0) {
            continue;
        }

        fat16_dir_entry_t* entries = (fat16_dir_entry_t*)sector_buffer;
        uint32_t entries_per_sector = FAT16_SECTOR_SIZE / sizeof(fat16_dir_entry_t);

        for (uint32_t i = 0; i < entries_per_sector; i++) {
            fat16_dir_entry_t* entry = &entries[i];

            // End of directory?
            if (entry->name[0] == 0x00) {
                return loaded;
            }

            // Deleted or special entry?
            if ((unsigned char)entry->name[0] == 0xE5 ||
                (entry->attributes & FAT16_ATTR_VOLUME_ID) ||
                (entry->attributes & FAT16_ATTR_DIRECTORY)) {
                continue;
            }

            // Check for .MOD extension
            if (entry->name[8] == 'M' && entry->name[9] == 'O' && entry->name[10] == 'D') {
                // Convert to 8.3 filename
                char filename[13];
                int pos = 0;

                // Copy name (8 chars)
                for (int j = 0; j < 8 && entry->name[j] != ' '; j++) {
                    filename[pos++] = entry->name[j];
                }

                // Add dot
                filename[pos++] = '.';

                // Copy extension (3 chars)
                for (int j = 8; j < 11 && entry->name[j] != ' '; j++) {
                    filename[pos++] = entry->name[j];
                }

                filename[pos] = '\0';

                // Load module
                if (disk_load_module(mgr, fs, filename) == 0) {
                    loaded++;
                }
            }
        }
    }

    return loaded;
}
