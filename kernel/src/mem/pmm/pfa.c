#include "mem/pmm/pmm.h"
#include "mem/paging/paging.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

extern bool     pfa_allowing_allocations;

extern pmm_section_t* pmm_sections;
extern uint64_t free_memory;

extern void pmm_recombine();
extern pmm_section_t* pmm_new_section();
extern void pmm_delete_section(pmm_section_t* section);

// Creates new section with start at split_at and links surrounding nodes
void pmm_section_split(pmm_section_t* target, uint64_t split_at) {
    if (split_at >= (target->start + (target->pages * PAGING_PAGE_SIZE)))
        return;

    if (target->next != NULL) {
        target->next->prev = pmm_new_section();
        target->next->prev->prev = target;
        target->next->prev->next = target->next;
        target->next = target->next->prev;
    }
    else {
        target->next = pmm_new_section();
        target->next->prev = target;
    }

    target->next->free = target->free;
    target->next->pages = target->pages - ((split_at - target->start) / PAGING_PAGE_SIZE);
    target->next->start = split_at;
    target->pages -= ((target->start + (target->pages * PAGING_PAGE_SIZE)) - split_at) / PAGING_PAGE_SIZE;
}

void pmm_section_combine_next(pmm_section_t* target) {
    if (target->next == NULL) return;
    if (target->free != target->next->free) return;

    target->next->start = target->start;
    target->next->pages += target->pages;
    target->next->prev = target->prev;
    target->prev->next = target->next;
    pmm_delete_section(target);
}
// Combines only work when types are same
void pmm_section_combine_prev(pmm_section_t* target) {
    if (target->prev == NULL) return;
    if (target->free != target->prev->free) return;

    target->prev->pages += target->pages;
    target->prev->next = target->next;
    target->next->prev = target->prev;
    pmm_delete_section(target);
}

extern void __print_pmm_section_headers();

void* pmm_alloc_page() {
    if (!pfa_allowing_allocations) return NULL;
    
    uint64_t base = 0;

    pmm_section_t* selected;
    for (selected = pmm_sections; selected != NULL; selected = selected->next) {
        if (selected->pages == 0) {
            selected->next->prev = selected->prev;
            selected->prev->next = selected->next;
            pmm_section_t* old = selected;
            selected = selected->prev;  // manuever cuz iteration
            pmm_delete_section(old);
            continue;
        }
        if (selected->free == PMM_SECTION_FREE) {
            base = selected->start;
            if (selected->prev->free == PMM_SECTION_USED) { // if previous section is a used one 
                selected->prev->pages += 1;
                selected->pages -= 1;
                selected->start += PAGING_PAGE_SIZE;
            }
            else { // if not create a new used section
                pmm_section_split(selected, selected->start + PAGING_PAGE_SIZE);
                selected->free = PMM_SECTION_USED;
            }
            break;
        }
    }

    if (base != 0)
        free_memory -= PAGING_PAGE_SIZE;

    return (void *)base;
}
// Allocators and freers use complex logic to avoid fragmentation and frequent recombination
void pmm_free_page(void* page) {
    if (!pfa_allowing_allocations) return;
    
    pmm_section_t* selected;
    for (selected = pmm_sections; selected != NULL; selected = selected->next) {
        if (selected->start == (uint64_t)page) {
            if (selected->free == PMM_SECTION_FREE) return;
            if (selected->pages == 1) { // One page chunk and we need to free it
                if (selected->prev->free == PMM_SECTION_FREE && selected->next->free == PMM_SECTION_FREE) {
                    pmm_section_t* prev = selected->prev;
                    pmm_section_combine_next(selected);
                    pmm_section_combine_next(prev);
                    return;
                }
                else if (selected->prev->free == PMM_SECTION_FREE) {
                    pmm_section_combine_prev(selected);
                    return;
                }
                else if (selected->next->free == PMM_SECTION_FREE) {
                    pmm_section_combine_next(selected);
                    return;
                }
                else {
                    selected->free = PMM_SECTION_FREE;
                    return;
                }
            }
            else { // Multiple pages in a chunk and we are only freeing one (and the freeing address is the start of this chunk)
                if (selected->prev->free == PMM_SECTION_FREE) {
                    selected->prev->pages += 1;
                    selected->start += PAGING_PAGE_SIZE;
                    selected->pages -= 1;
                    return;
                }
                else { // in between two used sections
                    pmm_section_split(selected, selected->start + PAGING_PAGE_SIZE);
                    selected->free = PMM_SECTION_FREE; // this is the new small section
                    return;
                }
            }
        }
        if (selected->start < (uint64_t)page && selected->next->start > (uint64_t)page) {   // in the middle of a chunk
            if (selected->free == PMM_SECTION_FREE) return; // if already free return
            if ((selected->next->start - PAGING_PAGE_SIZE) == (uint64_t)page) {   // if at end of a page
                if (selected->next->free == PMM_SECTION_FREE) {
                    selected->next->start -= PAGING_PAGE_SIZE;
                    selected->next->pages += 1;
                    selected->pages -= 1;
                }
                else {
                    pmm_section_split(selected, (uint64_t)page);
                    selected = selected->next;
                    pmm_section_split(selected, selected->start + PAGING_PAGE_SIZE); // now should have one page chunk
                    selected->free = PMM_SECTION_FREE; // now a free chunk
                }
            }
        }
    }
    free_memory += PAGING_PAGE_SIZE;
}

void pmm_lock_page(void* page) {
    if (!pfa_allowing_allocations) return;

    pmm_section_t* selected;
    for (selected = pmm_sections; selected != NULL; selected = selected->next) {
        if (selected->pages == 1) {
            if (selected->start == (uint64_t)page) {
                selected->free = PMM_SECTION_USED;
                if (selected->prev->free == PMM_SECTION_USED && selected->next->free == PMM_SECTION_USED) {
                    pmm_section_t* prev = selected->prev;
                    pmm_section_combine_next(selected);
                    pmm_section_combine_next(prev);
                    return;
                }
                else if (selected->prev->free == PMM_SECTION_USED) {
                    pmm_section_combine_prev(selected);
                    return;
                }
                else if (selected->next->free == PMM_SECTION_USED) {
                    pmm_section_combine_next(selected);
                    return;
                }
                return;
            }
        }
        if (selected->start == (uint64_t)page) { // if at beginning of chunk
            if (selected->free == PMM_SECTION_USED) return; // if used return
            if (selected->prev->free == PMM_SECTION_USED) {
                selected->prev->pages += 1;
                selected->pages -= 1;
                selected->start += PAGING_PAGE_SIZE;
                return;
            }
            else { // Create a new chunk
                pmm_section_split(selected, selected->start + PAGING_PAGE_SIZE);
                selected->free = PMM_SECTION_USED;
                return;
            }
        }
        if (selected->start < (uint64_t)page && selected->next->start > (uint64_t)page) { // in the middle of chunk
            if (selected->free == PMM_SECTION_USED) return; // if used return
            if ((selected->next->start - PAGING_PAGE_SIZE) == (uint64_t)page) { // if at end of chunk
                if (selected->next->free == PMM_SECTION_USED) {
                    selected->next->start -= PAGING_PAGE_SIZE;
                    selected->next->pages += 1;
                    selected->pages -= 1;
                }
                else {
                    pmm_section_split(selected, (uint64_t)page);
                    selected = selected->next;
                    selected->free = PMM_SECTION_USED;
                }
            }
            else { // really in middle and need to create new chunk
                pmm_section_split(selected, (uint64_t)page);
                selected = selected->next;
                pmm_section_split(selected, selected->start + PAGING_PAGE_SIZE);
                selected->free = PMM_SECTION_USED;
            }
        }
    }
}

void pmm_unlock_page(void* page) {
    if (!pfa_allowing_allocations) return;
    pmm_free_page(page);
}

void pmm_lock_pages(void* page, size_t count) {
    if (!pfa_allowing_allocations) return;
    for (size_t i = 0; i < count; i++) {
        pmm_lock_page((void *)((uint64_t)page + (i * PAGING_PAGE_SIZE)));
    }
}

void pmm_unlock_pages(void* page, size_t count) {
    if (!pfa_allowing_allocations) return;
    for (size_t i = 0; i < count; i++) {
        pmm_free_page((void *)((uint64_t)page + (i * PAGING_PAGE_SIZE)));
    }
}

extern pmm_pool_t* pmm_pools; // NULL

void* pmm_alloc_pool(size_t page_count) {
    if (!pfa_allowing_allocations) return NULL;
    
    pmm_pool_t* selected;

    if (pmm_pools == NULL) {
        pmm_pools = (pmm_pool_t*)malloc(sizeof(pmm_pool_t));
        memset(pmm_pools, 0, PAGING_PAGE_SIZE);
        pmm_pools = &pmm_pools[0];
        pmm_pools->next = NULL;
        selected = pmm_pools;
    }
    else {
        for (selected = pmm_pools; selected->next != NULL; selected = selected->next);
        selected->next = (pmm_pool_t*)malloc(sizeof(pmm_pool_t)); // find last pool
        selected = selected->next;
        selected->next = NULL;
    }

    selected->pages = page_count;
    
    pmm_section_t* section;
    for (section = pmm_sections; section != NULL; section = section->next) {
        if (section->pages >= page_count && section->free == PMM_SECTION_FREE) {
            selected->base = section->start;
            pmm_lock_pages((void *)selected->base, selected->pages);
            return (void *)selected->base;
        }
    }

    selected = NULL;

    pmm_recombine();
    return NULL;
}

void pmm_free_pool(void* pool) {
    if (!pfa_allowing_allocations) return;

    if (pmm_pools == NULL) return;
    if (pool == NULL) return;

    for (pmm_pool_t* pooli = pmm_pools; pooli != NULL; pooli = pooli->next) {
        if (pooli->base <= (uint64_t)pool && (pooli->base + (pooli->pages * PAGING_PAGE_SIZE)) >= (uint64_t)pool) {
            pmm_unlock_pages((void *)pooli->base, pooli->pages);
            return;
        }
    }

    pmm_recombine();
}

void* pmm_realloc_pool(void* pool, size_t page_count) {
    if (!pfa_allowing_allocations) return NULL;

    if (pmm_pools == NULL) return NULL;
    if (pool == NULL) return NULL;

    for (pmm_pool_t* pooli = pmm_pools; pooli != NULL; pooli = pooli->next) {
        if (pooli->base == (uint64_t)pool) {
                pmm_section_t* section;
                for (section = pmm_sections; section != NULL; section = section->next) {
                    if (section->pages >= page_count && section->free == PMM_SECTION_FREE) {
                        uint64_t sec_start, sec_pages;
                        sec_start = section->start;
                        sec_pages = section->pages;
                        pmm_lock_pages((void *)sec_start, page_count);
                        memcpy((void *)sec_start, (void *)pooli->base, pooli->pages * PAGING_PAGE_SIZE);
                        memset((void *)pooli->base, 0, pooli->pages * PAGING_PAGE_SIZE);
                        pmm_unlock_pages((void *)pooli->base, pooli->pages);
                        pooli->base = sec_start;
                        pooli->pages = sec_pages;
                        return (void *)sec_start;
                    }
                }
        }
    }

    pmm_recombine();
    return pool;
}
