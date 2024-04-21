#include "mem/pmm/pmm.h"
#include "mem/paging/paging.h"
#include "cpu/tss/tss.h"
#include "proc/task/task.h"
#include "iodev/io.h"
#include <stdint.h>
#include <stddef.h>

#define PMAP_VIRT_DEFAULT   0x0000

uint64_t pmap(void* virt) {
    void* phys = pmm_alloc_page();

    if (virt == PMAP_VIRT_DEFAULT)
        virt = phys;

    task_context_t* ctx = (task_context_t *)((gs_kernel_base_t *)rdmsr(IA32_GS_BASE))->ctx;
    paging_table_t* kpml4 = paging_get_pml4();
    paging_load_pml4((paging_table_t *)ctx->cr3);
    uint64_t address = (uint64_t)paging_map_page(virt, phys, PAGING_FLAGS_USER_PAGE);
    paging_load_pml4(kpml4);
    return address;
}
