#include <stdint.h>
#include <stddef.h>

#include "mm/pmm/pmm.h"
#include "mm/paging/paging.h"

#define PMAP_VIRT_DEFAULT   0x0000

uint64_t pmap(void* virt) {
    void* phys = pmm_alloc_page();

    if (virt == PMAP_VIRT_DEFAULT)
        virt = phys;

    return (uint64_t)paging_map_page(virt, phys, PAGING_FLAGS_USER_PAGE);
}
