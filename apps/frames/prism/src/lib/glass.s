global rdinfo
rdinfo:
    mov eax, 0x0090
    syscall
    ret

global pmap
pmap:
    mov eax, 0x0091
    syscall
    ret

global punmap
punmap:
    mov eax, 0x0092
    syscall
    ret

global fb_req
fb_req:
    mov eax, 0x0093
    syscall
    ret

global fb_kill
fb_kill:
    mov eax, 0x0094
    syscall
    ret

global pid
pid:
    mov eax, 0x0095
    syscall
    ret

global kb_man
kb_man:
    mov eax, 0x0096
    syscall
    ret

