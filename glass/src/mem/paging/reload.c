#include <string.h>
#include "paging.h"
#include "mem/pmm/pmm.h"
#include "boot/protocol.h"

extern paging_table_t* pml4;

extern uint8_t __load_base;
extern uint8_t __load_max;

extern memory_map_t* memory_map;
void paging_reload_kernel_map();
void paging_reload_kernel_map() {
    pml4 = (paging_table_t *)pmm_alloc_page();

    memset(pml4, 0x00, PAGING_PAGE_SIZE);

    for (uint64_t i = 0; i < memory_map->entry_count; i++) {
        if (memory_map->entries[i].signal != MEMORY_MAP_NOUSE) {
            for (uint64_t t = memory_map->entries[i].base; t < memory_map->entries[i].base + memory_map->entries[i].length; t+=PAGING_PAGE_SIZE) {
                paging_map_page((void *)t, (void *)t, PAGING_FLAGS_KERNEL_PAGE);
                paging_map_page((void *)t + PAGING_VIRTUAL_OFFSET, (void *)t, PAGING_FLAGS_KERNEL_PAGE);
            }
        }
    }

    uint64_t kernel_phys = (uint64_t)get_kernel_load_physical();
    for (uint64_t t = 0; t < ((uint64_t)&__load_max - (kernel_phys + get_kernel_virtual_offset())); t+=PAGING_PAGE_SIZE)
        paging_map_page((void *)((kernel_phys + get_kernel_virtual_offset()) + t), (void *)(kernel_phys + t), PAGING_FLAGS_KERNEL_PAGE);
        
    __asm__ volatile ("mov %0, %%cr3" : : "a"(paging_walk_page(pml4)));
}

void paging_sync_cr3() {
    __asm__ volatile ("mov %0, %%cr3" : : "a"(pml4));
}
