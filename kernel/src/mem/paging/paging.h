#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define PAGING_FLAG_PRESENT         0x001
#define PAGING_FLAG_WRITE           0x002
#define PAGING_FLAG_USER            0x004
#define PAGING_FLAG_WRITE_THROUGH   0x008
#define PAGING_FLAG_CACHE_DISABLE   0x010
#define PAGING_FLAG_ACCESSED        0x020
#define PAGING_FLAG_LARGER_PAGES    0x040
#define PAGING_FLAG_OS_AVAILABLE    0xE00
#define PAGING_FLAG_NO_EXECUTE      (1 << 63)

#define PAGING_FLAGS_KERNEL_PAGE    (PAGING_FLAG_PRESENT | PAGING_FLAG_WRITE)
#define PAGING_FLAGS_USER_PAGE      (PAGING_FLAG_PRESENT | PAGING_FLAG_WRITE | PAGING_FLAG_USER)
#define PAGING_FLAGS_MMIO_PAGE      (PAGING_FLAG_PRESENT | PAGING_FLAG_WRITE | PAGING_FLAG_CACHE_DISABLE | PAGING_FLAG_WRITE_THROUGH)

#define PAGING_PAGE_SIZE            0x1000
#define PAGING_PAGE_BOUNDARY        0x1000
#define PAGING_PAGE_ALIGNED         __attribute__((aligned(PAGING_PAGE_SIZE)))

#define PAGING_KERNEL_OFFSET        0xffffffff80000000
#define PAGING_VIRTUAL_OFFSET       0xffff800000000000

#define nullvptr                     - PAGING_VIRTUAL_OFFSET == NULL

typedef uint64_t paging_desc_t;

typedef struct {
    uint16_t    pml4;
    uint16_t    pml3;
    uint16_t    pml2;
    uint16_t    pml1;
} paging_indexer_t;

typedef struct { 
	paging_desc_t   entries[512];
} PAGING_PAGE_ALIGNED paging_table_t;

static
__attribute__((always_inline)) 
inline
void paging_desc_set_address(paging_desc_t* descriptor, uint64_t address) {
    *descriptor |= (uint64_t)address & 0xffffffffff000;
}

static
__attribute__((always_inline)) 
inline
void paging_indexer_assign(paging_indexer_t* indexer, void* address) {
    uint64_t uaddress = (uint64_t)address;
    uaddress >>= 12;
    indexer->pml1 = uaddress & 0x1ff;
    uaddress >>= 9;
    indexer->pml2 = uaddress & 0x1ff;
    uaddress >>= 9;
    indexer->pml3 = uaddress & 0x1ff;
    uaddress >>= 9;
    indexer->pml4 = uaddress & 0x1ff;
}

static
__attribute__((always_inline)) 
inline
void* paging_desc_get_address(paging_desc_t* descriptor) {
    return (void*)(*descriptor & 0xffffffffff000);
}

static
__attribute__((always_inline)) 
inline
bool paging_desc_get_flag(paging_desc_t* descriptor, uint64_t flag) {
    return *descriptor & flag;
}

static
__attribute__((always_inline)) 
inline
void paging_desc_set_flag(paging_desc_t* descriptor, uint64_t flag, bool value) {
    *descriptor &= ~flag;
    if (value)
        *descriptor |= flag;
}

static
__attribute__((always_inline)) 
inline
void paging_desc_set_flags(paging_desc_t* descriptor, uint64_t flags) {
    *descriptor &= ~0x0ffful;
    *descriptor |= flags;
}

static
__attribute__((always_inline)) 
inline
void paging_invlpg(void* page) {
    __asm__ volatile ("invlpg (%0)" : : "r"(page) : "memory");
}

void*   paging_map_page(void* virt, void* phys, uint16_t flags);
void    paging_unmap_page(void* virt);
void*   paging_remap_page(void* old_virt, void* new_virt);
void*   paging_walk_page(void* virt);
void*   paging_edit_page(void* virt, uint16_t flags);
void    paging_load_pml4(paging_table_t* pml4);
void*   paging_get_pml4();
