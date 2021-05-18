#include "pmm.h"
#include "../paging/paging.h"
#include "drivers/uart/serial.h"
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

bool initialized = false;
uint8_t* allocation_map = NULL;
uint64_t map_size = 0;
uint64_t fast_index = 0;

void pmm_map_set(uint64_t index, bool value) {
    uint8_t bit = index % 8;
    index -= bit;
    index /= 8;
    allocation_map[index] |= value << bit;
}

uint8_t pmm_map_get(uint64_t index) {
    uint8_t bit = index % 8;
    index -= bit;
    index /= 8;
    uint8_t byte = allocation_map[index];
    return byte & (0b10000000 >> bit);
}

void pmm_reindex() {
    for (uint64_t index = 0; index < map_size; index++) {
        if (!pmm_map_get(index)) {
            fast_index = index;
            return;
        }
    }
    fast_index = 0;
    return;
}

char* itoa(int value, unsigned int base);

void pmm_start(struct stivale2_struct_tag_memmap* memory_map_info) {
    if (initialized)
        return;

    initialized = true;

    for (int i = 0; i < memory_map_info->entries; i++)
        if (memory_map_info->memmap[i].type == STIVALE2_MMAP_USABLE ||
            memory_map_info->memmap[i].type == STIVALE2_MMAP_KERNEL_AND_MODULES ||
            memory_map_info->memmap[i].type == STIVALE2_MMAP_BOOTLOADER_RECLAIMABLE ||
            memory_map_info->memmap[i].type == STIVALE2_MMAP_ACPI_RECLAIMABLE ||
            memory_map_info->memmap[i].type == STIVALE2_MMAP_ACPI_NVS)
                total_memory += memory_map_info->memmap[i].length;

    map_size = total_memory / PAGING_PAGE_SIZE / 8;

    for (int i = 0; i < memory_map_info->entries; i++) {
        if (memory_map_info->memmap[i].type == 1) {
            if (memory_map_info->memmap[i].length >= map_size && memory_map_info->memmap[i].base >= 0x100000) {
                allocation_map = (uint8_t *)memory_map_info->memmap[i].base;
                break;
            }
        }
    }

    memset(allocation_map, 0xFF, map_size);

    free_memory = 0;

    for (int i = 0; i < memory_map_info->entries; i++)
        if (memory_map_info->memmap[i].type == 1)
            pmm_unlock_pages((void *)memory_map_info->memmap[i].base, memory_map_info->memmap[i].length / PAGING_PAGE_SIZE);

    pmm_lock_pages((void *)0x0, 0x100000 / PAGING_PAGE_SIZE);

    pmm_lock_pages(allocation_map, map_size / PAGING_PAGE_SIZE);

    pmm_reindex();

    void* page = pmm_alloc_page();

    serial_terminal()->puts("pmm: test: page 0x")->puts(itoa((int)page, 16));

    pmm_free_page(page);
}