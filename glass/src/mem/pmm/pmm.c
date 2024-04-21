#include "mem/pmm/pmm.h"
#include "mem/paging/paging.h"
#include "iodev/uart/uartsh.h"
#include "boot/protocol.h"
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

extern bool pfa_allowing_allocations;
bool pfa_allowing_allocations = false;
uint64_t free_memory;

pmm_section_t* pmm_sections = NULL;
pmm_pool_t* pmm_pools = NULL;
uint64_t _pmm_section_head;
uint64_t pmm_data_size;

void pmm_section_manager_reindex();

void pmm_section_manager_create(void* base) {
    if (pfa_allowing_allocations) return;
    pmm_sections = (pmm_section_t*)base;
    memset(pmm_sections, 0, pmm_data_size); // helps validate the area is large enough
    _pmm_section_head = 1;
}

void pmm_section_manager_reindex() {
    _pmm_section_head = 0;
    for (uint64_t i = 0; (i * sizeof(pmm_section_t)) < pmm_data_size; i++) {
        pmm_section_t* current = &pmm_sections[i];
        if (current->prev == NULL && current->next == NULL) {
            _pmm_section_head = i;
            return;
        }
    }
}

pmm_section_t* pmm_new_section() {
    if (_pmm_section_head * sizeof(pmm_section_t) >= pmm_data_size) {
        return NULL;
    }

    pmm_section_t* new_section = &pmm_sections[_pmm_section_head];
    if (new_section->prev != NULL || new_section->next != NULL) {
        pmm_section_manager_reindex();
        new_section = &pmm_sections[_pmm_section_head];
        if (new_section->prev != NULL || new_section->next != NULL) {
            return NULL;
        }
    }
    _pmm_section_head++;
    return new_section;
}

void pmm_delete_section(pmm_section_t* section) {
    section->prev = NULL;
    section->next = NULL;
    section->free = 0;
    section->pages = 0;
    section->start = 0;
}

uint64_t pmm_get_free_memory() {
    return free_memory;
}

uint64_t pmm_interpret_signal(uint64_t memory_map_signal) {
    switch (memory_map_signal) {
        case MEMORY_MAP_BUSY:
            return PMM_SECTION_USED;
        case MEMORY_MAP_FREE:
            return PMM_SECTION_FREE;
        case MEMORY_MAP_MMIO:
            return PMM_SECTION_BAD;
        case MEMORY_MAP_NOUSE:
            return PMM_SECTION_BAD;
        default:
            return PMM_SECTION_BAD;
    }
}

void pmm_recombine() {
    pmm_section_t* current;
    uint64_t detected = 0;
    // reiterate until all combined
    for (uint64_t iteration = 0; !(detected == 0 && iteration > 0); iteration++) {
        detected = 0;
        for (current = pmm_sections; current != NULL; current = current->next) {
            if (current->next == NULL) {
                break;       // if next is null then break
            }
            if (current->free != current->next->free) {
                continue;       // if types don't match then continue to next iteration
            }
            detected = 1;            
            // since current->free == current->next->free
            current->pages += current->next->pages;
            if (current->next->next != NULL) {
                current->next = current->next->next;
                pmm_delete_section(current->next->prev);
                current->next->prev = current;
            }
            else {
                pmm_delete_section(current->next);
                current->next = NULL;
            }
        }
    }
}

void pmm_recalculate_free_memory() {
    pmm_section_t* current;
    free_memory = 0;
    for (current = pmm_sections; current != NULL; current = current->next) {
        if (current->free == PMM_SECTION_FREE) {
            free_memory += current->pages * PAGING_PAGE_SIZE;
        }
    }
}

static uint64_t estimated_total_memory;

#define MEGABYTE (1024*1024)
#define ROUND_OFF 64

extern memory_map_t* memory_map;
void pmm_start();
void pmm_start() {
    if (pfa_allowing_allocations) return;

    pmm_data_size = 0;
    _pmm_section_head = 0;
    estimated_total_memory = 0;
    free_memory = 0;

    // First document the size of memory and required pmm memory, "explore" memory map
    for (uint64_t i = 0; i < memory_map->entry_count; i++) {
        if (memory_map->entries[i].signal == MEMORY_MAP_FREE) {
            free_memory += memory_map->entries[i].length;
        }
    }
    // round up to nearest 64mb for estimated total memory to calculate pmm data size
    estimated_total_memory = free_memory / (ROUND_OFF * MEGABYTE);
    if (free_memory % (ROUND_OFF * MEGABYTE) != 0) estimated_total_memory++;
    estimated_total_memory *= (ROUND_OFF * MEGABYTE);
    pmm_data_size = estimated_total_memory / MAX_PMM_HEADER_PROPORTION;
    

    // Now find place to store pmm data
    for (uint64_t i = 0; i < memory_map->entry_count; i++) {
        if (memory_map->entries[i].signal == MEMORY_MAP_FREE) {
            if (memory_map->entries[i].length >= pmm_data_size) {
                pmm_section_manager_create((void *)memory_map->entries[i].base);
                break;
            }
        }
    }

    // Now create the pmm data (cry)
    // Create an initial section
    pmm_sections->prev = NULL;
    pmm_sections->next = NULL;
    pmm_sections->start = 0x0000000000000000;
    pmm_sections->pages = MEGABYTE / PAGING_PAGE_SIZE;
    pmm_sections->free = PMM_SECTION_BAD;

    pmm_section_t* current = pmm_sections;
    for (uint64_t i = 0; i < memory_map->entry_count; i++) {
        // This case mostly applies on the first iteration
        if (memory_map->entries[i].base == current->start) {
            continue;
        }
        else {
            // Create a new pmm section based on the memory map entry information
            current->next = pmm_new_section();
            current->next->prev = current;
            current->next->next = NULL;
            current->next->start = memory_map->entries[i].base;
            current->next->free = pmm_interpret_signal(memory_map->entries[i].signal);
            current->next->pages = memory_map->entries[i].length / PAGING_PAGE_SIZE;
            if (memory_map->entries[i].length % PAGING_PAGE_SIZE != 0) current->next->pages++;
            current = current->next;
            // If there is a gap between the current section and the previous one, create a new section labeled bad
            if (current->prev->start + (current->prev->pages * PAGING_PAGE_SIZE) != current->start) {
                current->prev->next = pmm_new_section();    // make new section between previous and current
                current->prev->next->prev = current->prev;  // make new section previous current's current previous
                current->prev->next->next = current;        // make new section next the current
                current->prev->next->start = current->prev->start + (current->prev->pages * PAGING_PAGE_SIZE); // make new section start at end of current previous
                current->prev->next->pages = (current->start - current->prev->next->start) / PAGING_PAGE_SIZE; // make new section size of the gap
                if ((current->start - current->prev->next->start) % PAGING_PAGE_SIZE != 0) current->prev->next->pages++;
                current->prev->next->free = PMM_SECTION_BAD;      // label new section as bad unusable 
                current->prev = current->prev->next;        // make current previous the new section, current previous's next
            }
        }
    }

    pmm_recombine();

    pmm_recalculate_free_memory();

    pfa_allowing_allocations = true;
    
    // Lock the pmm data pages
    pmm_lock_pages(pmm_sections, (pmm_data_size / PAGING_PAGE_SIZE) + ((pmm_data_size % PAGING_PAGE_SIZE != 0) ? 1 : 0));
}

static char* __memstate_string(uint64_t state) {
    switch (state) {
        case PMM_SECTION_FREE:
            return "FREE";
        case PMM_SECTION_USED:
            return "USED";
        case PMM_SECTION_BAD:
            return "BAD.";
        default:
            return "ERR!";
    }
}

void __uartsh_pmm_dump() {
    pmm_recalculate_free_memory();
    printf("\r\nmemory state:\r\n\r\n");
    printf("free memory: %d megabytes\r\n", free_memory / MEGABYTE);
    printf("total memory: %d megabytes\r\n", estimated_total_memory / MEGABYTE);
    printf("regions:\r\n");
    for (pmm_section_t* current = pmm_sections; current != NULL; current = current->next)
        printf("\t%xh => %dkB/%dmB %s\r\n", current->start, (current->pages * PAGING_PAGE_SIZE) / 1024, (current->pages * PAGING_PAGE_SIZE) / MEGABYTE, __memstate_string(current->free));
}
