#pragma once
#include <stdint.h>

#define ELF_SEGMENT_NULL        0x00
#define ELF_SEGMENT_LOAD        0x01
#define ELF_SEGMENT_DYNAMIC     0x02
#define ELF_SEGMENT_INTERP      0x03
#define ELF_SEGMENT_NOTE        0x04

#define ELF_HEADER_MAGIC        "\177ELF"

#define ELF_ARCH_IRRELEVANT     0x00
#define ELF_ARCH_SPARC	        0x02
#define ELF_ARCH_x86	        0x03
#define ELF_ARCH_MIPS	        0x08
#define ELF_ARCH_POWERPC	    0x14
#define ELF_ARCH_ARM	        0x28
#define ELF_ARCH_SUPERH	        0x2A
#define ELF_ARCH_IA_64	        0x32
#define ELF_ARCH_x86_64	        0x3E
#define ELF_ARCH_AARCH64	    0xB7
#define ELF_ARCH_RISCV	        0xF3

#define ELF_ABI_SYSTEMV         0x00

#define ELF_ENDIANNESS_LITTLE   0x01
#define ELF_ENDIANNESS_BIG      0x02

#define ELF_BITNESS_32          0x01
#define ELF_BITNESS_64          0x02


typedef struct {
    uint8_t     magic[4];
    uint8_t     bitness;
    uint8_t     endianness;
    uint8_t     header_version;
    uint8_t     abi;
    uint8_t     pad[8];
    uint16_t    resc;
    uint16_t    architecture;
    uint32_t    elf_version;
    uint64_t    program_entry;  
    uint64_t    header_table;
    uint64_t    section_table;
    uint32_t    flags;
    uint16_t    header_size;
    uint16_t    program_table_entry_size;
    uint16_t    program_table_entries;
    uint16_t    section_table_entry_size;
    uint16_t    section_table_entries;
    uint16_t    section_names_index;
} __attribute__((packed)) elf_header_t;

typedef struct {
    uint32_t    type;
    uint32_t    flags;
    uint64_t    p_offset;
    uint64_t    p_vaddr;
    uint64_t    undefined;
    uint64_t    p_filesz;
    uint64_t    p_memsz;
    uint64_t    alignment;
} __attribute__((packed)) elf_program_header_t;

typedef struct {
    uint64_t    loaded_at; // where segment should be according to exec
    uint64_t    located_at; // where segment physically is in memory (phys)
    uint64_t    length;
} elf_loaded_page_t;

typedef struct {
    uint64_t    entry;
    uint64_t    segment_count;
    elf_loaded_page_t* segments;
} elf_load_info_t;

elf_load_info_t* elf_load_program(void* file);
