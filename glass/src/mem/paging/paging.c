#include "mem/pmm/pmm.h"
#include <string.h>
#include "paging.h"

extern paging_table_t* pml4;
paging_table_t* pml4;

void* paging_map_page(void* virt, void* phys, uint16_t flags) {
    flags &= 0x0fff;
    paging_indexer_t indexer;
    paging_indexer_assign(&indexer, virt);
    paging_desc_t pde;

    pde = pml4->entries[indexer.pml4];
    paging_table_t* pdp;
    if (!paging_desc_get_flag(&pde, PAGING_FLAG_PRESENT)) {
        void* new_table = pmm_alloc_page();
        pdp = (paging_table_t *)new_table;
        memset((void *)pdp, 0, PAGING_PAGE_SIZE);
        paging_desc_set_address(&pde, (uint64_t)pdp);
        paging_desc_set_flag(&pde, PAGING_FLAG_PRESENT, true);
        pde |= flags;
        pml4->entries[indexer.pml4] = pde;
        paging_map_page((void *)pdp, (void *)((uintptr_t)pdp), PAGING_FLAGS_KERNEL_PAGE);
    } else {
        pdp = (paging_table_t *)((uintptr_t)paging_desc_get_address(&pde));
        pml4->entries[indexer.pml4] |= flags;
    }
    
    pde = pdp->entries[indexer.pml3];
    paging_table_t* pd;
    if (!paging_desc_get_flag(&pde, PAGING_FLAG_PRESENT)) {
        void* new_table = pmm_alloc_page();
        pd = (paging_table_t *)new_table;
        memset((void *)pd, 0, PAGING_PAGE_SIZE);
        paging_desc_set_address(&pde, (uint64_t)pd);
        paging_desc_set_flag(&pde, PAGING_FLAG_PRESENT, true);
        pde |= flags;
        pdp->entries[indexer.pml3] = pde;
        paging_map_page((void *)pd, (void *)((uintptr_t)pd), PAGING_FLAGS_KERNEL_PAGE);
    } else {
        pd = (paging_table_t *)((uintptr_t)paging_desc_get_address(&pde));
        pdp->entries[indexer.pml3] |= flags;
    }

    pde = pd->entries[indexer.pml2];
    paging_table_t* pt;
    if (!paging_desc_get_flag(&pde, PAGING_FLAG_PRESENT)) {
        void* new_table = pmm_alloc_page();
        pt = (paging_table_t *)new_table;
        memset((void *)pt, 0, PAGING_PAGE_SIZE);
        paging_desc_set_address(&pde, (uint64_t)pt);
        paging_desc_set_flag(&pde, PAGING_FLAG_PRESENT, true);
        pde |= flags;
        pd->entries[indexer.pml2] = pde;
        paging_map_page((void *)pt, (void *)((uintptr_t)pt), PAGING_FLAGS_KERNEL_PAGE);
    } else {
        pt = (paging_table_t *)((uintptr_t)paging_desc_get_address(&pde));
        pd->entries[indexer.pml2] |= flags;
    }

    pde = pt->entries[indexer.pml1];
    paging_desc_set_address(&pde, (uintptr_t)phys);
    paging_desc_set_flag(&pde, PAGING_FLAG_PRESENT, true);
    paging_desc_set_flags(&pde, flags);
    pt->entries[indexer.pml1] = pde;
    return virt;
}

void* paging_walk_page(void* virt) {
    paging_indexer_t indexer;
    paging_indexer_assign(&indexer, virt);
    paging_desc_t pde;

    pde = pml4->entries[indexer.pml4];
    paging_table_t* pdp;
    
    if (!paging_desc_get_flag(&pde, PAGING_FLAG_PRESENT))
        return NULL;
        
    pdp = (paging_table_t *)((uintptr_t)paging_desc_get_address(&pde));
    
    pde = pdp->entries[indexer.pml3];
    paging_table_t* pd;

    if (!paging_desc_get_flag(&pde, PAGING_FLAG_PRESENT))
        return NULL;
        
    pd = (paging_table_t *)((uintptr_t)paging_desc_get_address(&pde));

    pde = pd->entries[indexer.pml2];
    paging_table_t* pt;

    if (!paging_desc_get_flag(&pde, PAGING_FLAG_PRESENT))
        return NULL;

    pt = (paging_table_t *)((uintptr_t)paging_desc_get_address(&pde));

    pde = pt->entries[indexer.pml1];
    return paging_desc_get_address(&pde);
}

void paging_unmap_page(void* virt) {
    paging_indexer_t indexer;
    paging_indexer_assign(&indexer, virt);
    paging_desc_t pde;

    pde = pml4->entries[indexer.pml4];
    paging_table_t* pdp;
    
    if (!paging_desc_get_flag(&pde, PAGING_FLAG_PRESENT))
        return;
        
    pdp = (paging_table_t *)((uintptr_t)paging_desc_get_address(&pde));
    
    pde = pdp->entries[indexer.pml3];
    paging_table_t* pd;

    if (!paging_desc_get_flag(&pde, PAGING_FLAG_PRESENT))
        return;
        
    pd = (paging_table_t *)((uintptr_t)paging_desc_get_address(&pde));

    pde = pd->entries[indexer.pml2];
    paging_table_t* pt;

    if (!paging_desc_get_flag(&pde, PAGING_FLAG_PRESENT))
        return;

    pt = (paging_table_t *)((uintptr_t)paging_desc_get_address(&pde));

    pde = pt->entries[indexer.pml1];
    pde = 0;
    pt->entries[indexer.pml1] = pde;

    paging_invlpg(virt);
}


void* paging_edit_page(void* virt, uint16_t flags) {
    paging_indexer_t indexer;
    paging_indexer_assign(&indexer, virt);

    paging_desc_t pde;
    pde = pml4->entries[indexer.pml4];
    pml4->entries[indexer.pml4] |= flags;
    paging_table_t* pdp = (paging_table_t *)((uintptr_t)paging_desc_get_address(&pde));
    pdp->entries[indexer.pml3] |= flags;
    pde = pdp->entries[indexer.pml3];
    paging_table_t* pd = (paging_table_t *)((uintptr_t)paging_desc_get_address(&pde));
    pd->entries[indexer.pml2] |= flags;
    pde = pd->entries[indexer.pml2];
    paging_table_t* pt = (paging_table_t *)((uintptr_t)paging_desc_get_address(&pde));
    paging_desc_set_flags(&pt->entries[indexer.pml1], flags);

    paging_invlpg(virt);
    return virt;
}

void* paging_remap_page(void* old, void* new) {
    paging_indexer_t indexer;
    paging_indexer_assign(&indexer, old);
    paging_desc_t old_pde;

    old_pde = pml4->entries[indexer.pml4];
    paging_table_t* pdp;
    if (!paging_desc_get_flag(&old_pde, PAGING_FLAG_PRESENT))
        return NULL;
    else
        pdp = (paging_table_t *)((uintptr_t)paging_desc_get_address(&old_pde));
    
    
    old_pde = pdp->entries[indexer.pml3];
    paging_table_t* pd;
    if (!paging_desc_get_flag(&old_pde, PAGING_FLAG_PRESENT)) 
        return NULL;
    else
        pd = (paging_table_t *)((uintptr_t)paging_desc_get_address(&old_pde));

    old_pde = pd->entries[indexer.pml2];
    paging_table_t* pt;
    if (!paging_desc_get_flag(&old_pde, PAGING_FLAG_PRESENT)) 
        return NULL;
    else
        pt = (paging_table_t *)((uintptr_t)paging_desc_get_address(&old_pde));

    old_pde = pt->entries[indexer.pml1];
    
    paging_unmap_page(old);

    paging_map_page(new, paging_desc_get_address(&old_pde), old_pde &= 0x0fff);

    paging_invlpg(old);

    return new;
}

void paging_load_pml4(paging_table_t* _pml4) {
    pml4 = _pml4;
}

void* paging_get_pml4() {
    return pml4;
}
