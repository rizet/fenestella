global memcpy
memcpy:
    push rbp
    mov rbp, rsp
    pushfq
    mov rdi, rdi
    mov rsi, rsi
    mov rax, rdi
    push rcx
    mov rcx, rdx
    cld
    rep movsb
    pop rcx
    popfq
    pop rbp
    ret
    