global idt_reload
idt_reload:
    push rbp
    mov rbp, rsp
    pushfq
    cli
    lidt [rdi]
    popfq
    pop rbp
    ret
    