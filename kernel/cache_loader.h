#ifndef CACHE_LOADER_H
#define CACHE_LOADER_H

#include <stddef.h>
#include <stdint.h>
#include "module_loader.h"

typedef void (*cache_registry_iter_fn)(const char* name, const unsigned char* data, unsigned int size, void* ctx);

void cache_registry_foreach(cache_registry_iter_fn fn, void* ctx);

void cache_load_modules(module_manager_t* mgr);

#endif // CACHE_LOADER_H
