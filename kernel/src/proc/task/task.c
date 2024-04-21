#include "proc/task/task.h"
#include "mem/paging/paging.h"
#include "cpu/tss/tss.h"
#include "mem/pmm/pmm.h"
#include "iodev/io.h"
#include "iodev/uart/uartsh.h"
#include "extra/conv.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct _linked_task {
    task_t* task;
    struct _linked_task* next;
} linked_task_t;

linked_task_t   *tasks;
uint64_t        task_count;
uint64_t        current_task_id;

// PRECONFIGURES scheduler, scheduler starts when first task is added
void scheduler_preconfigure()
{
    task_count = 0;
}

extern uint8_t __load_base;
extern uint8_t __load_max;

#define MEGABYTE 1024*1024

extern void* __gdt;
extern void* __idt;

// Creates a new page table for a tasks context
paging_table_t* task_new_page_table(elf_load_info_t* load_info, task_t* task) {
    paging_table_t* task_table = (paging_table_t *)pmm_alloc_page();
    memset(task_table, 0, PAGING_PAGE_SIZE); // zero out page table or else weird page faults will occur

    paging_table_t* kpml4 = paging_get_pml4();
    paging_load_pml4(task_table);

    uint64_t kernel_phys = (uint64_t)get_kernel_load_physical();
    for (uint64_t t = 0; t < ((uint64_t)&__load_max - (kernel_phys + get_kernel_virtual_offset())); t+=PAGING_PAGE_SIZE)
        paging_map_page((void *)((kernel_phys + get_kernel_virtual_offset()) + t), (void *)(kernel_phys + t), PAGING_FLAGS_KERNEL_PAGE);

    for (uint64_t i = 0; i < load_info->segment_count; i++) {
        for (uint64_t j = 0; j < load_info->segments[i].length; j+=PAGING_PAGE_SIZE)
            paging_map_page((void *)((load_info->segments[i].loaded_at & ~0xfff) + j), (void *)(load_info->segments[i].located_at + j), PAGING_FLAGS_USER_PAGE);
    }
        
    for (uint64_t head = task->ctx->stack.rsp - MEGABYTE; head < task->ctx->stack.rsp; head+=PAGING_PAGE_SIZE)     // map user stack
        paging_map_page((void *)head, (void *)(head), PAGING_FLAGS_USER_PAGE);

    for (uint64_t ist = 0; ist < 7; ist++) {
        uint64_t ist_addr = (uint64_t)tss_get(0)->ist[ist];
        if (ist_addr == (uint64_t)NULL) break;
        for (uint64_t offset = 0; offset <= (PAGING_PAGE_SIZE * IST_STACK_PAGES); offset+=PAGING_PAGE_SIZE)
            paging_map_page((void *)(ist_addr - offset), (void *)(ist_addr - offset), PAGING_FLAGS_KERNEL_PAGE);
    }

    paging_map_page((void *)((uint64_t)task->ctx & 0xfffffffffffff000), paging_walk_page((void *)((uint64_t)task->ctx & 0xfffffffffffff000)), PAGING_FLAGS_KERNEL_PAGE);
    paging_map_page((void *)task_table, (void *)task_table, PAGING_FLAGS_KERNEL_PAGE);
    paging_map_page((void *)task->gs_base, (void *)task->gs_base, PAGING_FLAGS_KERNEL_PAGE);

    paging_load_pml4(kpml4);

    return task_table;
}

// Creates a new stack of a megabyte for a task
uint64_t task_new_stack() {
    return (uint64_t)pmm_alloc_pool(MEGABYTE / PAGING_PAGE_SIZE) + MEGABYTE;
}

gs_kernel_base_t* task_new_gs_base() {
    gs_kernel_base_t* gs_base = (gs_kernel_base_t *)pmm_alloc_page();
    memset(gs_base, 0, PAGING_PAGE_SIZE);
    return gs_base;
}


uint64_t task_create_new(elf_load_info_t* load_info) {
    task_t* new_task = (task_t *)malloc(sizeof(task_t));

    new_task->id = task_count;
    new_task->priority = 0;
    new_task->state = TASK_STATE_SLEEPING;  // create it sleeping, start it later

    new_task->ctx = (task_context_t *)malloc(sizeof(task_context_t));
    new_task->ctx->registers.rdi = 0;
    new_task->ctx->registers.rsi = 0;
    new_task->ctx->registers.rdx = 0;
    new_task->ctx->registers.rcx = 0;
    new_task->ctx->registers.rbx = 0;
    new_task->ctx->registers.rax = 0;
    new_task->ctx->registers.r8 = 0;
    new_task->ctx->registers.r9 = 0;
    new_task->ctx->registers.r10 = 0;
    new_task->ctx->registers.r11 = 0;
    new_task->ctx->registers.r12 = 0;
    new_task->ctx->registers.r13 = 0;
    new_task->ctx->registers.r14 = 0;
    new_task->ctx->registers.r15 = 0;

    new_task->gs_base = task_new_gs_base();
    new_task->gs_base->tss = 0;
    new_task->gs_base->pid = new_task->id;
    new_task->gs_base->rsp = tss_get(0)->rsp[0];
    new_task->gs_base->pc = (uint64_t)load_info->entry;
    new_task->gs_base->ctx = (void*)new_task->ctx;

    new_task->ctx->stack.rsp = task_new_stack();
    new_task->ctx->stack.rbp = new_task->ctx->stack.rsp;
    
    new_task->ctx->rip = (uint64_t)load_info->entry;
    new_task->ctx->cr3 = (uint64_t)task_new_page_table(load_info, new_task);
    
    // add task to linked list

    linked_task_t* new_link = (linked_task_t *)tasks;

    if (tasks == NULL) {
        tasks = (linked_task_t *)malloc(sizeof(linked_task_t));
        tasks->task = new_task;
        tasks->next = NULL;
    } else {
        while (new_link->next != NULL) {
            new_link = new_link->next;
        }
        new_link->next = (linked_task_t *)malloc(sizeof(linked_task_t));
        new_link->next->task = new_task;
        new_link->next->next = NULL;
    }
    
    task_count++;

    return new_task->id;
}

void task_schedule(uint64_t task_id) {
    linked_task_t* thread = tasks;
    while (thread != NULL) {
        if (thread->task->id == task_id) {
            thread->task->state = TASK_STATE_WAITING;
            break;
        }
        thread = thread->next;
    }
}

void task_sleep(uint64_t task_id) {
    linked_task_t* thread = tasks;
    while (thread != NULL) {
        if (thread->task->id == task_id) {
            thread->task->state = TASK_STATE_SLEEPING;
            break;
        }
        thread = thread->next;
    }
}

void task_kill(uint64_t task_id) {
    linked_task_t* thread = tasks;
    while (thread != NULL) {
        if (thread->task->id == task_id) {
            thread->task->state = TASK_STATE_DEAD;
            break;
        }
        thread = thread->next;
    }
}

extern void _load_task_page_tables(void* cr3);

extern void _finalize_task_switch(task_context_t* ctx);


void task_select(uint64_t task_id) {
    current_task_id = task_id;

    linked_task_t* thread = tasks;

    while (thread != NULL) {
        if (thread->task->id == task_id)
            break;
        thread = thread->next; 
    }

    __asm__ volatile ("cli");

    wrmsr(IA32_KERNEL_GS_BASE, (uint64_t)thread->task->gs_base);
    wrmsr(IA32_GS_BASE, 0);

    _finalize_task_switch(thread->task->ctx);
}

uint64_t task_select_next() {
    linked_task_t* thread = tasks;

    while (thread != NULL) {
        if (thread->task->id == current_task_id)
            break;
        thread = thread->next;
    }

    if (thread == NULL || tasks == NULL) {
        // panic
        return -1;
    }

    uint64_t gs_base = rdmsr(IA32_GS_BASE);
    uint64_t kgs_base = rdmsr(IA32_KERNEL_GS_BASE);
    thread->task->state = TASK_STATE_WAITING;
    thread->task->gs_base = (gs_kernel_base_t *)gs_base;

    if (thread->next == NULL)
        thread = tasks;    

    thread->task->state = TASK_STATE_EXECUTION;
    current_task_id = thread->task->id;
    gs_base = (uint64_t)thread->task->gs_base;
    kgs_base = 0;
    wrmsr(IA32_GS_BASE, gs_base);
    wrmsr(IA32_KERNEL_GS_BASE, kgs_base);

    gs_kernel_base_t* _gs_base = (gs_kernel_base_t *)gs_base;
    _gs_base->tss = 0;
    _gs_base->ctx = thread->task->ctx;
    _gs_base->pid = thread->task->id;
    _gs_base->rsp = tss_get(_gs_base->tss)->rsp[0];
    _gs_base->pc = thread->task->ctx->rip;

    return current_task_id;
}

static char* __task_state_string(uint64_t state) {
    switch (state) {
        case TASK_STATE_EXECUTION:
            return "EXEC";
        case TASK_STATE_WAITING:
            return "WAIT";
        case TASK_STATE_SLEEPING:
            return "SLEP";
        case TASK_STATE_DEAD:
            return "DEAD";
        default:
            return "ERR!";
    }
}

void __uartsh_proc_dump() {
    printf("\r\nProcess dump:\r\n");
    for (linked_task_t* task = tasks; task != NULL; task = task->next) {
        printf("\tpid %d, cr3=%x, rip=%x, ctx@%x ... %s\r\n", task->task->id, task->task->ctx->cr3, task->task->ctx->rip, task->task->ctx, __task_state_string(task->task->state));
    }
    printf("\r\n");
}
