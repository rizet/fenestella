#pragma once
#include "cpu/tss/tss.h"
#include "proc/loader/elf.h"
#include <stdint.h>
#include <stdbool.h>

#define TASK_STATE_EXECUTION    0
#define TASK_STATE_WAITING      1
#define TASK_STATE_SLEEPING     2
#define TASK_STATE_DEAD         3

#define IA32_KERNEL_GS_BASE 0xC0000102
#define IA32_GS_BASE        0xC0000101

typedef struct {
    struct {
        uint64_t    rax; // +0x00
        uint64_t    rbx; // +0x08
        uint64_t    rcx; // +0x10
        uint64_t    rdx; // +0x18
        uint64_t    rdi; // +0x20
        uint64_t    rsi; // +0x28
        uint64_t    r8; // +0x30
        uint64_t    r9; // +0x38
        uint64_t    r10; // +0x40
        uint64_t    r11; // +0x48
        uint64_t    r12; // +0x50
        uint64_t    r13; // +0x58
        uint64_t    r14; // +0x60
        uint64_t    r15; // +0x68
    } registers;
    
    struct {
        uint64_t rsp; // +0x70
        uint64_t rbp; // +0x78
    } stack;

    uint64_t rip;   // instruction pointer +0x80
    uint64_t cr3;   // page table +0x88
} __attribute__((__packed__)) task_context_t;

typedef struct {
    uint64_t id;
    uint64_t state;
    uint64_t priority; // currently irrelevant
    task_context_t* ctx;
    gs_kernel_base_t* gs_base;
} task_t;

uint64_t task_create_new(elf_load_info_t* load_info);

void task_sleep(uint64_t task_id);
void task_schedule(uint64_t task_id);
void task_kill(uint64_t task_id);
void task_select(uint64_t task_id);
// Sets current Id to next task, returns id, and modifies gs_base. must be run during interrupt
uint64_t task_select_next();
void task_swap_suspended(task_t* new_task);
