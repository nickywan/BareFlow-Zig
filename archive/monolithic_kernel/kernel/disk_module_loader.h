// ============================================================================
// BAREFLOW - Disk Module Loader
// ============================================================================
// File: kernel/disk_module_loader.h
// Purpose: Load modules from FAT16 filesystem
// ============================================================================

#ifndef DISK_MODULE_LOADER_H
#define DISK_MODULE_LOADER_H

#include "fat16.h"
#include "module_loader.h"

// Load a module from FAT16 disk into memory and register it
// Returns 0 on success, -1 on error
int disk_load_module(module_manager_t* mgr, fat16_fs_t* fs, const char* filename);

// Load all .MOD files from disk
// Returns number of modules loaded, -1 on error
int disk_load_all_modules(module_manager_t* mgr, fat16_fs_t* fs);

#endif // DISK_MODULE_LOADER_H
