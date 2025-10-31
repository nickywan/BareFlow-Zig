#include "cache_loader.h"
#include "module_loader.h"
#include "vga.h"

static void cache_log(const char* msg) {
    terminal_setcolor(VGA_LIGHT_BLUE, VGA_BLACK);
    terminal_writestring("[CACHE] ");
    terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);
    terminal_writestring(msg);
}

struct cache_iter_ctx {
    module_manager_t* mgr;
    unsigned int loaded;
};

static void cache_install_callback(const char* name, const unsigned char* data, unsigned int size, void* ctx_ptr) {
    struct cache_iter_ctx* ctx = (struct cache_iter_ctx*)ctx_ptr;
    if (!data || size < sizeof(module_header_t)) {
        cache_log("Skipping malformed cache entry: ");
        cache_log(name);
        cache_log("\n");
        return;
    }

    const module_header_t* header = (const module_header_t*)data;

    if (header->magic != MODULE_MAGIC) {
        cache_log("Invalid module magic for: ");
        cache_log(name);
        cache_log("\n");
        return;
    }

    // For .mod files, entry_point may be NULL (code starts right after header)
    // module_install_override will handle this correctly by treating NULL as offset 0

    int rc = module_install_override(ctx->mgr, header, size);
    if (rc == 1) {
        cache_log("Replaced embedded module '");
        cache_log(name);
        cache_log("' with cached version\n");
        ctx->loaded++;
    } else if (rc == 0) {
        cache_log("Loaded new cached module '");
        cache_log(name);
        cache_log("'\n");
        ctx->loaded++;
    } else {
        cache_log("Failed to install cached module '");
        cache_log(name);
        cache_log("'\n");
    }
}

__attribute__((weak)) void cache_registry_foreach(cache_registry_iter_fn fn, void* ctx) {
    (void)fn;
    (void)ctx;
}

void cache_load_modules(module_manager_t* mgr) {
    struct cache_iter_ctx ctx = {mgr, 0};
    cache_registry_foreach(cache_install_callback, &ctx);

    if (ctx.loaded == 0) {
        cache_log("No optimized modules detected\n");
    }
}
