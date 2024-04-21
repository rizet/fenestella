global enable_fpu
enable_fpu:
    clts
    mov rax, cr0
    and rax, ~0x04
    or rax, 0x20
    or rax, 0x02
    mov cr0, rax
    fninit
    lea rax, [rel .fpu_control_word]
    fldcw [rax]
    ret

.fpu_control_word:
    dw 0x037A
    