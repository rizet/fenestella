global tss_reload
tss_reload:
    push rbp
    mov rbp, rsp
    pushfq
    cli
    ltr di
    popfq
    pop rbp
    ret
    