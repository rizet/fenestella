global _start
extern main

default rel

_start:
    xor rbp, rbp
    push rbp
    mov rbp, rsp
    xor edi, edi
    mov bx, 0x0101
    lea r15, [rel main]
    call r15
    pop rbp
    ret
    