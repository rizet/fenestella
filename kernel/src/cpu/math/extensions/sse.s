global allow_sse
allow_sse:
    mov rax, cr4
    or rax, 0x200
    or rax, 0x400
    mov cr4, rax
    ret
    