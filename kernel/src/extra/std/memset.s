global memset
memset:
    push rbp
    mov rbp, rsp
    pushfq
    mov rdi, rdi
    mov rax, rsi
    push rcx
    push rdi
    mov rcx, rdx
    cld
    rep stosb
    pop rax
    pop rcx
    popfq
    pop rbp
    ret
    