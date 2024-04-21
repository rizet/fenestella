#pragma once
#include "boot/protocol.h"
#include <stdint.h>
#include <stddef.h>

static inline char* memtype_string(uint32_t type) {
    switch (type) {
        case MEMORY_MAP_FREE: return "free";
        case MEMORY_MAP_BUSY: return "used";
        case MEMORY_MAP_MMIO: return "mmio";
        case MEMORY_MAP_NOUSE: return "bad";
    }
    return "unidentifiable";
}

#define PMM_SECTION_USED    0
#define PMM_SECTION_FREE    1
#define PMM_SECTION_BAD     2

typedef struct _pmm_section {
    struct _pmm_section*    prev;
    struct _pmm_section*    next;
    uint64_t                start;
    uint64_t                pages;
    uint64_t                free;
} pmm_section_t;

typedef struct _pmm_pool {
    struct _pmm_pool*       next;
    uint64_t                pages;
    uint64_t                base;
} pmm_pool_t;

#define MAX_PMM_HEADER_PROPORTION   64
// uses about 1 MiB of memory for 64 MiB of memory under maximum load

void*   pmm_alloc_page(void);
void    pmm_free_page(void* page);
void*   pmm_alloc_pool(size_t page_count);
void*   pmm_realloc_pool(void* pool, size_t page_count);
void    pmm_free_pool(void* pool);
void    pmm_lock_page(void* page);
void    pmm_unlock_page(void* page);
void    pmm_lock_pages(void* page, size_t count);
void    pmm_unlock_pages(void* page, size_t count);

uint64_t pmm_get_free_memory();
