#include "mem/pmm/pmm.h"
#include "mem/paging/paging.h"
#include <stdint.h>
#include <stddef.h>

void punmap(void* virt) {
    if (!virt)
        return;

    void* phys = paging_walk_page(virt);
    paging_unmap_page(virt);
    paging_map_page(phys, phys, PAGING_FLAGS_KERNEL_PAGE);

    return;
}
