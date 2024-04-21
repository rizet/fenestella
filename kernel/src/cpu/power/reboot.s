global _reboot
_reboot:
    xor eax, eax
    mov ah, 0x02
    mov dx, 0x0064
    .waiting:
    in al, dx
    and al, ah
    test al, al
    jnz $+(.waiting-$)
    .reset:
    mov ah, 0xFE
    out dx, al
    hlt