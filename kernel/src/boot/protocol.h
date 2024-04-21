#pragma once
#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint64_t    base;
    uint64_t    length;
    uint64_t    signal;
} __attribute__((__packed__)) memory_map_entry_t;

typedef struct {
    uint64_t            entry_count;
    memory_map_entry_t* entries;
} memory_map_t;

typedef struct {
    uint64_t    virt;
    uint64_t    phys;
    uint64_t    size;
    char*       name;
} boot_module_t;

typedef struct {
    uint32_t    frame_width;
    uint32_t    frame_height;
    uint64_t    frame_addr;
    uint32_t    frame_pitch;
    uint32_t    frame_bpp;
} framebuffer_info_t;

extern memory_map_t* memory_map;
extern boot_module_t* boot_modules;
extern uint64_t boot_module_count;
extern framebuffer_info_t framebuffer;

boot_module_t* get_boot_module(char* name);

void* get_kernel_load_physical();
uint64_t get_kernel_virtual_offset();

#define MEMORY_MAP_FREE     0x00
#define MEMORY_MAP_BUSY     0x01
#define MEMORY_MAP_MMIO     0x02
#define MEMORY_MAP_NOUSE    0x03

#define MEMORY_AMD_IOMMU_BLOCK_START    0xFD00000000
#define MEMORY_AMD_IOMMU_BLOCK_END      0xFFFFFFFFFF
