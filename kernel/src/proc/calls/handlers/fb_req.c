#include "boot/protocol.h"
#include "mem/paging/paging.h"
#include "cpu/tss/tss.h"
#include "proc/task/task.h"
#include "iodev/io.h"
#include <stdint.h>
#include <stddef.h>

static uint64_t current_fb_holder_pid = -1;

// returns null if denied and only gives fb to one process
void* fb_req() {
    if (current_fb_holder_pid != -1)
        return NULL;
    task_context_t* ctx = ((gs_kernel_base_t*)(rdmsr(IA32_GS_BASE)))->ctx;
    current_fb_holder_pid = ((gs_kernel_base_t*)(rdmsr(IA32_GS_BASE)))->pid;
    paging_table_t* kpml4 = paging_get_pml4();
    paging_load_pml4((paging_table_t *)ctx->cr3);
    // Map framebuffer into task address space
    uint64_t framebuffer_size = (framebuffer.frame_pitch * framebuffer.frame_height * (framebuffer.frame_bpp / 8));
    for (uint64_t i = 0; i < framebuffer_size; i += PAGING_PAGE_SIZE) {
        paging_map_page((void *)(framebuffer.frame_addr + i), (void *)((framebuffer.frame_addr - PAGING_VIRTUAL_OFFSET) + i), PAGING_FLAGS_USER_PAGE);
    }
    paging_load_pml4(kpml4);
    return (void *)framebuffer.frame_addr;
}

void fb_kill() {
    if (current_fb_holder_pid == -1)
        return;
    task_context_t* ctx = ((gs_kernel_base_t*)(rdmsr(IA32_GS_BASE)))->ctx;
    paging_table_t* kpml4 = paging_get_pml4();
    paging_load_pml4((paging_table_t *)ctx->cr3);
    // Unmap framebuffer into task address space
    uint64_t framebuffer_size = (framebuffer.frame_pitch * framebuffer.frame_height * (framebuffer.frame_bpp / 8));
    for (uint64_t i = 0; i < framebuffer_size; i += PAGING_PAGE_SIZE) {
        paging_unmap_page((void *)(framebuffer.frame_addr + i));
    }
    paging_load_pml4(kpml4);
    current_fb_holder_pid = -1;
}