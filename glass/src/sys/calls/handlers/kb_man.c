#include <stddef.h>
#include "proc/task/task.h"
#include "mm/paging/paging.h"
#include "mm/pmm/pmm.h"
#include "dev/hid/kbd.h"
#include "dev/io.h"

static void* current_buffer = (void*)0;
static uint64_t current_pid = -1;
static uint16_t* current_size = (uint16_t*)0;

// Returns a 1 page buffer of keycodes with a 16-bit buffer size field at the end of the page
// If !taking, frees the buffer and returns NULL
void* kb_man(bool taking) {
    task_context_t* ctx = ((gs_kernel_base_t*)(rdmsr(IA32_GS_BASE)))->ctx;
    uint64_t pid = ((gs_kernel_base_t*)(rdmsr(IA32_GS_BASE)))->pid;
    paging_table_t* kpml4 = paging_get_pml4();
    paging_load_pml4((paging_table_t *)ctx->cr3);

    if (!taking) {
        if (current_pid != pid || current_pid == -1) {
            paging_load_pml4(kpml4);
            return NULL;
        }
        current_pid = -1;
        pmm_free_page(current_buffer);
        paging_unmap_page(current_buffer);
        current_buffer = NULL;
        current_size = NULL;
        paging_load_pml4(kpml4);
        return NULL;
    }
    else {
        if (current_pid != -1) {
            paging_load_pml4(kpml4);
            return NULL;
        }
        current_pid = pid;
        current_buffer = pmm_alloc_page();
        current_size = (uint16_t*)((uint64_t)current_buffer + PAGING_PAGE_SIZE - sizeof(uint16_t));
        *current_size = 0;
        paging_map_page(current_buffer, current_buffer, PAGING_FLAGS_USER_PAGE);
        paging_load_pml4(kpml4);
        return current_buffer;
    }

    paging_load_pml4(kpml4);
}

void __kb_man_save_key(uint8_t keycode) {
    if (current_pid == -1 || current_buffer == NULL || current_size == NULL)
        return;
    
    if (*current_size >= (PAGING_PAGE_SIZE - sizeof(uint16_t)))
        return;

    uint8_t* buffer = (uint8_t*)current_buffer;
    buffer[(*current_size)++] = keycode;
}
