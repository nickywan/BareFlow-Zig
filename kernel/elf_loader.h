// ============================================================================
// BAREFLOW - Minimal ELF Loader for Bare-Metal
// ============================================================================
// File: kernel/elf_loader.h
// Purpose: Load ELF32 binaries (e.g., LLVM JIT module) without libc
// ============================================================================

#ifndef ELF_LOADER_H
#define ELF_LOADER_H

#include <stdint.h>
#include <stddef.h>

// ============================================================================
// ELF32 FORMAT DEFINITIONS
// ============================================================================

#define EI_NIDENT 16
#define EI_MAG0 0
#define EI_MAG1 1
#define EI_MAG2 2
#define EI_MAG3 3
#define EI_CLASS 4
#define EI_DATA 5
#define EI_VERSION 6

#define ELFMAG0 0x7f
#define ELFMAG1 'E'
#define ELFMAG2 'L'
#define ELFMAG3 'F'

#define ELFCLASS32 1
#define ELFDATA2LSB 1

#define ET_EXEC 2
#define ET_DYN 3

#define PT_LOAD 1

typedef uint32_t Elf32_Addr;
typedef uint32_t Elf32_Off;
typedef uint16_t Elf32_Half;
typedef uint32_t Elf32_Word;

// ELF32 Header
typedef struct {
    unsigned char e_ident[EI_NIDENT];
    Elf32_Half e_type;
    Elf32_Half e_machine;
    Elf32_Word e_version;
    Elf32_Addr e_entry;
    Elf32_Off e_phoff;
    Elf32_Off e_shoff;
    Elf32_Word e_flags;
    Elf32_Half e_ehsize;
    Elf32_Half e_phentsize;
    Elf32_Half e_phnum;
    Elf32_Half e_shentsize;
    Elf32_Half e_shnum;
    Elf32_Half e_shstrndx;
} Elf32_Ehdr;

// Program Header
typedef struct {
    Elf32_Word p_type;
    Elf32_Off p_offset;
    Elf32_Addr p_vaddr;
    Elf32_Addr p_paddr;
    Elf32_Word p_filesz;
    Elf32_Word p_memsz;
    Elf32_Word p_flags;
    Elf32_Word p_align;
} Elf32_Phdr;

// Section Header
typedef struct {
    Elf32_Word sh_name;
    Elf32_Word sh_type;
    Elf32_Word sh_flags;
    Elf32_Addr sh_addr;
    Elf32_Off sh_offset;
    Elf32_Word sh_size;
    Elf32_Word sh_link;
    Elf32_Word sh_info;
    Elf32_Word sh_addralign;
    Elf32_Word sh_entsize;
} Elf32_Shdr;

// Symbol Table Entry
typedef struct {
    Elf32_Word st_name;
    Elf32_Addr st_value;
    Elf32_Word st_size;
    unsigned char st_info;
    unsigned char st_other;
    Elf32_Half st_shndx;
} Elf32_Sym;

// ============================================================================
// ELF LOADER API
// ============================================================================

typedef struct {
    void* base_addr;           // Where ELF is loaded in memory
    Elf32_Addr entry_point;    // Entry point address
    uint32_t total_size;       // Total memory used
    uint32_t num_symbols;      // Number of symbols found
} elf_module_t;

/**
 * Load ELF32 executable from memory buffer
 *
 * @param elf_data  Pointer to ELF file data
 * @param size      Size of ELF data
 * @param load_addr Address where to load the ELF (or NULL for auto)
 * @param out       Output: loaded module information
 * @return 0 on success, -1 on error
 */
int elf_load(const uint8_t* elf_data, size_t size, void* load_addr, elf_module_t** out);

/**
 * Validate ELF header
 * @return 0 if valid, -1 if invalid
 */
int elf_validate(const Elf32_Ehdr* ehdr);

/**
 * Get symbol address by name
 * @return Symbol address or NULL if not found
 */
void* elf_get_symbol(elf_module_t* mod, const char* name);

/**
 * Free ELF module resources
 */
void elf_free(elf_module_t* mod);

#endif // ELF_LOADER_H
