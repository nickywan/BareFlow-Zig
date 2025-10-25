// ============================================================================
// BAREFLOW - Minimal ELF Loader Implementation
// ============================================================================

#include "elf_loader.h"
#include "stdlib.h"

extern void serial_puts(const char* str);

// Section type constants
#define SHT_NULL 0
#define SHT_SYMTAB 2
#define SHT_STRTAB 3

// Symbol binding and type macros
#define ELF32_ST_BIND(i) ((i) >> 4)
#define ELF32_ST_TYPE(i) ((i) & 0xf)
#define STB_GLOBAL 1
#define STT_FUNC 2
#define STT_OBJECT 1

// ============================================================================
// VALIDATION
// ============================================================================

int elf_validate(const Elf32_Ehdr* ehdr) {
    // Check magic bytes
    if (ehdr->e_ident[EI_MAG0] != ELFMAG0 ||
        ehdr->e_ident[EI_MAG1] != ELFMAG1 ||
        ehdr->e_ident[EI_MAG2] != ELFMAG2 ||
        ehdr->e_ident[EI_MAG3] != ELFMAG3) {
        serial_puts("[ELF] Invalid magic bytes\n");
        return -1;
    }

    // Check 32-bit
    if (ehdr->e_ident[EI_CLASS] != ELFCLASS32) {
        serial_puts("[ELF] Not 32-bit ELF\n");
        return -1;
    }

    // Check little-endian
    if (ehdr->e_ident[EI_DATA] != ELFDATA2LSB) {
        serial_puts("[ELF] Not little-endian\n");
        return -1;
    }

    // Check executable or shared object
    if (ehdr->e_type != ET_EXEC && ehdr->e_type != ET_DYN) {
        serial_puts("[ELF] Not executable or shared object\n");
        return -1;
    }

    return 0;
}

// ============================================================================
// ELF LOADING
// ============================================================================

int elf_load(const uint8_t* elf_data, size_t size, void* load_addr, elf_module_t** out) {
    if (!elf_data || size < sizeof(Elf32_Ehdr)) {
        serial_puts("[ELF] Invalid input\n");
        return -1;
    }

    // Parse ELF header
    const Elf32_Ehdr* ehdr = (const Elf32_Ehdr*)elf_data;

    if (elf_validate(ehdr) != 0) {
        return -1;
    }

    serial_puts("[ELF] Valid ELF32 header\n");

    // Allocate module structure
    elf_module_t* mod = (elf_module_t*)malloc(sizeof(elf_module_t));
    if (!mod) {
        serial_puts("[ELF] Failed to allocate module structure\n");
        return -1;
    }

    memset(mod, 0, sizeof(elf_module_t));

    // Determine load address and size
    uint32_t min_vaddr = 0xFFFFFFFF;
    uint32_t max_vaddr = 0;

    // Parse program headers to find memory range
    const Elf32_Phdr* phdr = (const Elf32_Phdr*)(elf_data + ehdr->e_phoff);
    for (int i = 0; i < ehdr->e_phnum; i++) {
        if (phdr[i].p_type == PT_LOAD) {
            if (phdr[i].p_vaddr < min_vaddr) {
                min_vaddr = phdr[i].p_vaddr;
            }
            uint32_t seg_end = phdr[i].p_vaddr + phdr[i].p_memsz;
            if (seg_end > max_vaddr) {
                max_vaddr = seg_end;
            }
        }
    }

    mod->total_size = max_vaddr - min_vaddr;

    // Allocate memory for loaded code
    if (load_addr == NULL) {
        load_addr = malloc(mod->total_size);
        if (!load_addr) {
            serial_puts("[ELF] Failed to allocate load memory\n");
            free(mod);
            return -1;
        }
    }

    mod->base_addr = load_addr;
    serial_puts("[ELF] Allocated ");
    // (Simplified - no int printing here)
    serial_puts(" bytes at load address\n");

    // Zero out the memory
    memset(load_addr, 0, mod->total_size);

    // Load program segments
    for (int i = 0; i < ehdr->e_phnum; i++) {
        if (phdr[i].p_type == PT_LOAD) {
            uint32_t offset = phdr[i].p_vaddr - min_vaddr;
            void* dest = (uint8_t*)load_addr + offset;
            const void* src = elf_data + phdr[i].p_offset;

            // Copy file contents
            memcpy(dest, src, phdr[i].p_filesz);

            // Zero remaining (BSS)
            if (phdr[i].p_memsz > phdr[i].p_filesz) {
                memset((uint8_t*)dest + phdr[i].p_filesz, 0,
                       phdr[i].p_memsz - phdr[i].p_filesz);
            }
        }
    }

    serial_puts("[ELF] Loaded program segments\n");

    // Store entry point (adjusted for base address)
    mod->entry_point = (Elf32_Addr)((uint8_t*)load_addr + (ehdr->e_entry - min_vaddr));

    // Parse symbol table (optional, for debugging)
    mod->num_symbols = 0;
    const Elf32_Shdr* shdr = (const Elf32_Shdr*)(elf_data + ehdr->e_shoff);
    for (int i = 0; i < ehdr->e_shnum; i++) {
        if (shdr[i].sh_type == SHT_SYMTAB) {
            mod->num_symbols = shdr[i].sh_size / sizeof(Elf32_Sym);
            break;
        }
    }

    *out = mod;
    serial_puts("[ELF] Load complete\n");
    return 0;
}

// ============================================================================
// SYMBOL LOOKUP
// ============================================================================

void* elf_get_symbol(elf_module_t* mod, const char* name) {
    // Simplified symbol lookup - for now just return entry point
    // Full implementation would parse .symtab and .strtab sections
    (void)name; // Unused for now

    if (!mod) {
        return NULL;
    }

    // Return entry point as default symbol
    return (void*)mod->entry_point;
}

// ============================================================================
// CLEANUP
// ============================================================================

void elf_free(elf_module_t* mod) {
    if (!mod) {
        return;
    }

    if (mod->base_addr) {
        free(mod->base_addr);
    }

    free(mod);
    serial_puts("[ELF] Module freed\n");
}
