global memset
memset:
    push rbp
    mov rbp, rsp
    mov rdi, rdi
    mov rax, rsi
    push rcx
    push rdi
    mov rcx, rdx
    cld
    rep stosb
    pop rax
    pop rcx
    pop rbp
    ret
    