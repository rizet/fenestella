global syscall_dispatch

%define nsyscalls 152

syscall_switch_kernel:
    push rbp
    mov rbp, rsp

    mov rax, [gs:0x20]
    mov [rax+0x00], rax ; get address of ctx and save rax
    
    xor eax, eax
    mov ax, 0x10     ; kernel data segment
    mov ds, ax
    mov es, ax

    mov rax, [gs:0x20]
    mov [rax+0x08], rbx
    mov [rax+0x10], rcx
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
    call paging_sync_cr3

    pop rbp
    ret

syscall_switch_user:
    push rbp
    mov rbp, rsp

    push rax    ; save rax (return register)
    push rdx    ; save rdx too (other return register)

    mov rax, [gs:0x20]
    mov rax, [rax+0x88] ; get cr3
    mov cr3, rax

    xor eax, eax
    mov ax, 0x1B    ; user data segment
    mov ds, ax
    mov es, ax

    mov rax, [gs:0x20]
    mov rbx, [rax+0x08]
    mov rcx, [rax+0x10]
    mov rdx, [rax+0x18]
    mov rdi, [rax+0x20] 
    mov rsi, [rax+0x28]
    mov r8, [rax+0x30]
    mov r9, [rax+0x38]
    mov r10, [rax+0x40]
    mov r11, [rax+0x48]
    mov r12, [rax+0x50]
    mov r13, [rax+0x58]
    mov r14, [rax+0x60]
    mov r15, [rax+0x68]
    mov rax, [rax+0x00] ; get rax

    pop rdx     ; restore rdx
    pop rax     ; restore rax

    pop rbp
    ret

syscall_dispatch:
    swapgs          ; always coming from user always swapgs
    mov [gs:0x28], rsp ; save rsp
    mov rsp, [gs:0x10] ; load rsp
    push rbp        ; save rbp
    mov rbp, rsp

    cmp rax, nsyscalls  ; check if syscall number is valid
    jge $+(.generate_ud-$)    ; if not, generate #UD

    push rdi
    push rsi
    push rdx
    push rax
    call syscall_switch_kernel
    pop rax
    pop rdx
    pop rsi
    pop rdi

    lea r10, [rel syscall_table]; relative address of syscall table
    lea r10, [r10 + (rax * 8)]  ; relative address of syscall
    mov rax, [r10]              ; load the syscall address

    xor r10d, r10d  ; clear r10

    .handle:
    cld             ; c calling convention cld
    call rax        ; call the syscall

    .sysret:
    call syscall_switch_user
    pop rbp         ; restore rbp
    mov rsp, [gs:0x28]  ; restore rsp
    swapgs
    o64 sysret      ; return to user mode

    .generate_ud:
    xor eax, eax    ; clear rax
    mov [rcx], rax  ; set null invalid opcode at return address to generate #UD (invalid opcode)
    jmp $+(.sysret-$)     ; like destroys the executing program, but it's fine it shouldn't have made a bad syscall

section .data

extern rdinfo
extern pmap
extern punmap
extern fb_req
extern fb_kill
extern pid
extern kb_man

syscall_table:
%rep 144
    dq 0x0000000000000000   ; placeholders for future use
%endrep
    dq rdinfo   ; read system information
    dq pmap     ; map a page
    dq punmap   ; unmap a page
    dq fb_req   ; framebuffer request
    dq fb_kill  ; framebuffer kill
    dq pid      ; get pid
    dq kb_man   ; take keyboard manager
    dq 0x0000000000000000   ; null syscall