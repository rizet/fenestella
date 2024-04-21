global gdt_reload
gdt_reload:
    push rbp
    mov rbp, rsp
    mov ax, dx
    pushfq
    cli
    lgdt [rdi]
    popfq
    mov ds, ax
    mov es, ax
    mov ss, ax
    pop rbp
    pop rax
    push rsi
    push rax
    xor rax, rax
    o64 retf
    