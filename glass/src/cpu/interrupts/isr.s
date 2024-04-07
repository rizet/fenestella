global isr_save_task_context
isr_save_task_context:
    lea r15, [rel __safe_r15]
    mov r15, [r15]
    push rbp
    mov rbp, rsp

    push rbx
    push rax
    mov rax, cr3
    mov rbx, [gs:0x20]
    mov [rbx+0x88], rax ; save task cr3

    pop rax
    mov [rbx+0x00], rax
    mov rax, rbx            ; save rax and rbx
    pop rbx
    mov [rax+0x08], rbx

    mov [rax+0x10], rcx    ; save other gp registers
    mov [rax+0x18], rdx
    mov [rax+0x20], rdi
    mov [rax+0x28], rsi
    mov [rax+0x30], r8
    mov [rax+0x38], r9
    mov [rax+0x40], r10
    mov [rax+0x48], r11
    mov [rax+0x50], r12
    mov [rax+0x58], r13
    mov [rax+0x60], r14
    mov [rax+0x68], r15

    extern paging_sync_cr3
    lea rax, [rel paging_sync_cr3]
    call rax
    mov rax, [gs:0x20]
    pop rbp
    ret

global isr_restore_task_context
isr_restore_task_context:
    push rbp
    mov rbp, rsp
    ; TODO proper full context switch
    mov rax, [gs:0x20]
    mov rax, [rax+0x88] ; load task cr3
    mov cr3, rax

    mov rax, [gs:0x20]
    mov rbx, [rax+0x08]
    mov rcx, [rax+0x10]
    mov rdx, [rax+0x18]
    mov rdi, [rax+0x20]
    mov rsi, [rax+0x28]
    mov r8,  [rax+0x30]
    mov r9,  [rax+0x38]
    mov r10, [rax+0x40]
    mov r11, [rax+0x48]
    mov r12, [rax+0x50]
    mov r13, [rax+0x58]
    mov r14, [rax+0x60]
    mov r15, [rax+0x68]
    mov rax, [rax+0x00] ; load general purpose registers

    pop rbp
    ret

%macro isr_err_stub 1
isr_stub_%+%1:
    push %1
    push r15
    lea r15, [rel isr_xframe_assembler]
    jmp r15
%endmacro

%macro isr_no_err_stub 1
isr_stub_%+%1:
    push 0
    push %1
    push r15
    lea r15, [rel isr_xframe_assembler]
    jmp r15
%endmacro

extern __routine_handlers

%macro irq_stub 1
irq_stub_%+%1:
    push rbp
    push r15
    push rax
    xor eax, eax
    mov ax, ds
    cmp ax, 0x10    ;; if ds == 0x10, we are in kernel mode
    jne .yes_task
    lea r15, [rel .no_task]
    jmp r15
    
    .yes_task:
    swapgs
    mov ax, 0x10    ; load kernel data segment
    mov ds, ax
    mov es, ax
    mov ss, ax

    pop rax
    pop r15

    push rax
    mov rax, r15
    lea r15, [rel __safe_r15]   ; elaborate local save r15 while also allowing for relative addressing
    mov [r15], rax
    mov r15, rax
    pop rax

    lea r15, [rel isr_save_task_context]
    call r15
    mov [gs:0x20], rax  ; save task context returned in rax
    lea r15, [rel __routine_handlers]
    mov r15, [r15 + %1 * 8]
    call r15

    mov ax, 0x1B    ; load user data segment
    mov ds, ax
    mov es, ax

    lea r15, [rel isr_restore_task_context]
    call r15
    pop rbp
    swapgs
    iretq
    
    .no_task:   ; interrupting kernel
    pop rax
    pop r15
    pushagrd
    lea r15, [rel __routine_handlers]
    mov r15, [r15 + %1 * 8]
    call r15
    popagrd
    pop rbp
    iretq

%endmacro

%macro pushagrd 0
push rax
push rbx
push rcx
push rdx
push rsi
push rdi
push r8
push r9
push r10
push r11
push r12
push r13
push r14
push r15
%endmacro

%macro popagrd 0
pop r15
pop r14
pop r13
pop r12
pop r11
pop r10
pop r9
pop r8
pop rdi
pop rsi
pop rdx
pop rcx
pop rbx
pop rax
%endmacro

%macro pushacrd 0
mov rax, cr0
push rax
mov rax, cr2
push rax
mov rax, cr3
push rax
mov rax, cr4
push rax
%endmacro

%macro popacrd 0
pop rax
mov cr4, rax
pop rax
mov cr3, rax
pop rax
mov cr2, rax
pop rax
mov cr0, rax
%endmacro

isr_xframe_assembler:
    pop r15
    push rbp
    mov rbp, rsp
    pushagrd
    pushacrd
    mov ax, ds
    push rax
    push qword 0
    push r15
    push rax
    xor eax, eax
    mov ax, ds
    cmp ax, 0x10    ;; if ds == 0x10, we are in kernel mode
    jne .yes_task
    lea r15, [rel .no_task]
    jmp r15
    
    .yes_task:
    swapgs
    mov ax, 0x10 ; load kernel data segment
    mov ds, ax
    mov es, ax
    mov ss, ax

    pop rax
    pop r15

    push rax
    mov rax, r15
    lea r15, [rel __safe_r15]   ; elaborate local save r15 while also allowing for relative addressing
    mov [r15], rax
    mov r15, rax
    pop rax

    lea r15, [rel isr_save_task_context]
    call r15
    mov [gs:0x20], rax  ; save task context returned in rax

    lea rdi, [rsp + 0x10]
    extern isr_exception_handler
    lea r15, [rel isr_exception_handler]
    call r15

    lea r15, [rel isr_restore_task_context]
    call r15

    pop rax
    pop rax
    mov ds, ax
    mov es, ax
    popacrd
    popagrd
    pop rbp
    add rsp, 0x10
    swapgs
    iretq


    .no_task:
    lea rdi, [rsp + 0x10]
    extern isr_exception_handler
    lea r15, [rel isr_exception_handler]
    call r15
    
    pop rax
    pop rax
    mov ds, ax
    mov es, ax
    popacrd
    popagrd
    pop rbp
    add rsp, 0x10
    iretq

isr_no_err_stub 0
isr_no_err_stub 1
isr_no_err_stub 2
isr_no_err_stub 3
isr_no_err_stub 4
isr_no_err_stub 5
isr_no_err_stub 6
isr_no_err_stub 7
isr_err_stub    8
isr_no_err_stub 9
isr_err_stub    10
isr_err_stub    11
isr_err_stub    12
isr_err_stub    13
isr_err_stub    14
isr_no_err_stub 15
isr_no_err_stub 16
isr_err_stub    17
isr_no_err_stub 18
isr_no_err_stub 19
isr_no_err_stub 20
isr_no_err_stub 21
isr_no_err_stub 22
isr_no_err_stub 23
isr_no_err_stub 24
isr_no_err_stub 25
isr_no_err_stub 26
isr_no_err_stub 27
isr_no_err_stub 28
isr_no_err_stub 29
isr_err_stub    30
isr_no_err_stub 31
irq_stub        32
irq_stub        33
irq_stub        34
irq_stub        35
irq_stub        36
irq_stub        37
irq_stub        38

section .data
global isr_stub_table
isr_stub_table:
%assign i 0 
%rep    32 
    dq isr_stub_%+i
%assign i i+1 
%endrep
%assign i 32
%rep    7
    dq irq_stub_%+i
%assign i i+1
%endrep
__safe_r15:
    dq 0
