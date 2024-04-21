_load_task_general_registers:
    push rbp
    mov rbp, rsp
    mov rax, [rdi + 0x00]
    mov rbx, [rdi + 0x08]
    mov rcx, [rdi + 0x10]
    mov rdx, [rdi + 0x18]
    mov rsi, [rdi + 0x28]
    mov r8, [rdi + 0x30]
    mov r9, [rdi + 0x38]
    mov r10, [rdi + 0x40]
    mov r11, [rdi + 0x48]
    mov r12, [rdi + 0x50]
    mov r13, [rdi + 0x58]
    mov r14, [rdi + 0x60]
    mov rdi, [rdi + 0x20]
    pop rbp
    ret

global _load_task_page_tables
_load_task_page_tables:
    push rbp
    mov rbp, rsp
    extern paging_walk_page
    call $+(paging_walk_page-$) ; rdi is virt; rax returns phys
    mov cr3, rax
    pop rbp
    ret

global _finalize_task_switch
_finalize_task_switch:
    cli
    mov rbp, rsp
    push rbp
    mov r15, rdi
    mov rdi, [r15 + 0x88] ; task->ctx->cr3
    pop rbp
    mov rsp, [r15 + 0x70]
    xor ebp, ebp
    push rbp
    mov rbp, rsp
    mov rbp, [r15 + 0x78]
    mov r14, [r15 + 0x80]
    call $+(_load_task_page_tables-$)
    xor eax, eax
    mov ax, 0x1B        ; user data segment
    mov ds, ax
    mov es, ax
    pop rbp
    push rax            ; push user data segment
    push rsp            ; push user stack
    sti
    pushfq              ; push flags
    cli
    push 0x23           ; user code segment
    push r14            ; push user instruction pointer
    mov rdi, r15
    call $+(_load_task_general_registers-$)
    .end:
    iretq
