global _start_limine64

section .text
default rel

_start_limine64:
    cli                 ; avoid exceptions or interruptions because idt is undefined
    cld                 ; standard c calling convention requirements
    
    xor ebp, ebp        
    push rbp
    mov rbp, rsp

    extern gdt_assemble
    lea r15, [rel gdt_assemble]
    call r15   ; assemble the bonito gdt

    extern idt_assemble
    lea r15, [rel idt_assemble]
    call r15   ; assemble the bonito idt
    sti                 ; yay, interrupts work** now!

    extern configure_math_extensions
    lea r15, [rel configure_math_extensions]
    call r15  ; floating points, sse, all those goodies

    xor eax, eax                    
    mov fs, ax                  ; zeroing (currently irrelevant) segment registers
    mov gs, ax

    extern limine_reinterpret
    lea r15, [rel limine_reinterpret]
    call r15   ; reinterpret stivale2 information to match internal protocol (boot protocol abstraction)

    extern pmm_start
    lea r15, [rel pmm_start]
    call r15      ; start the beautiful pmm

    extern paging_reload_kernel_map
    lea r15, [rel paging_reload_kernel_map]
    call r15   ; hope no page faults >>>>:((((((

    xor edi, edi        ; we're not passing anything
    extern tss_install
    lea r15, [rel tss_install]
    call r15    ; instalar la tss bonita

    extern apic_initialize
    lea r15, [rel apic_initialize]
    call r15                ; initialize the Local APIC and IOAPIC
    
    extern serial_console_enable
    lea r15, [rel serial_console_enable]
    call r15

    extern local_timer_calibrate
    lea r15, [rel local_timer_calibrate]
    call r15          ; calibrate the local APIC timer (using PIT)

    extern pci_conf_load_cache
    lea r15, [rel pci_conf_load_cache]
    call r15            ; load pci devices

    extern install_syscalls
    lea r15, [rel install_syscalls]
    call r15               ; install (los tontos) system calls

    xor edi, edi
    xor esi, esi                ; cleanup scratch registers
    xor eax, eax

    lea rdi, [rel initx_id]     ; load the init program id          
    extern get_boot_module
    lea r15, [rel get_boot_module]
    call r15        ; get the Frame executable
    mov rdi, [rax]              ; accessing first field of the returned structure (virt)
    extern elf_load_program
    lea r15, [rel elf_load_program]
    call r15       ; exec is module in memory, this function loads the elf segments properly and ready for execution

    extern task_create_new
    mov rdi, rax                ; create the new task with the entry point
    lea r15, [rel task_create_new]
    call r15      ; this function returns the task id

    push rax

    extern hid_enable_keyboard_interrupts
    lea r15, [rel hid_enable_keyboard_interrupts]
    call r15

    mov rdi, 0x0000             ; num of cpu
    mov rsi, 0x0000            ; num of ist0/rsp0
    extern tss_get_stack
    lea r15, [rel tss_get_stack]
    call r15          ; load the rsp0/ist0 stack and swap away the limine stack
    
    pop r15

    pop rbp
    pop rbp
    mov rsp, rax
    xor ebp, ebp            ; swap stacks
    push rbp
    mov rbp, rsp

    extern task_select
    mov rdi, r15
    lea r15, [rel task_select]
    call r15            ; select the task to run

    pop rbp

    extern _reboot
    lea r15, [rel _reboot]
    jmp r15                      ; reboot the system if the task_select function returns (which it shouldn't)
    
    
section .data
initx_id:
    db "prism.se",0     ; null-terminated c-string
