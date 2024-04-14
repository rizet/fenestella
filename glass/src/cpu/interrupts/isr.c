#include "isr.h"
#include "proc/task/task.h"
#include "symbols.h"
#include "dev/uart/uartsh.h"
#include "misc/conv.h"
#include "boot/protocol.h"

static const char* __exception_labels[] = {
    "[0x00] Divide by Zero Exception",
    "[0x01] Debug Exception",
    "[0x02] Unhandled Non-maskable Interrupt",
    "[0x03] Breakpoint Exception",
    "[0x04] Overflow Exception",
    "[0x05] Bound Range Exceeded Exception",
    "[0x06] Invalid Opcode/Operand Exception",
    "[0x07] Device Unavailable Exception",
    "[0x08] Double Fault",
    "[0x09] Coprocessor Segment Overrun",
    "[0x0A] Invalid TSS Exception",
    "[0x0B] Absent Segment Exception",
    "[0x0C] Stack-segment Fault",
    "[0x0D] General Protection Fault",
    "[0x0E] Page Fault",
    "[0x0F] Inexplicable Error",
    "[0x10] x87 Floating Exception",
    "[0x11] Alignment Check",
    "[0x12] Machine Check",
    "[0x13] SIMD Floating Exception",
    "[0x14] Virtualized Exception",
    "[0x15] Control Protection Exception",
    "[0x16] Inexplicable Error",
    "[0x17] Inexplicable Error",
    "[0x18] Inexplicable Error",
    "[0x19] Inexplicable Error",
    "[0x1A] Inexplicable Error",
    "[0x1B] Inexplicable Error",
    "[0x1C] Hypervisor Intrusion Exception",
    "[0x1D] VMM Communications Exception",
    "[0x1E] Security Exception",
    "[0x1F] Inexplicable Error"
};

void __print_interrupt_stacktrace(isr_xframe_t* ctx) {
    char itoa_buffer[67];
    serial_print_quiet("\r\nAttempted stacktrace: interrupt during \r\n");
    uint64_t rbp = ctx->base_frame.rbp;
    void* rip = (void *)ctx->base_frame.rip;
    // Assumes unrelocated
    while (true) {
        symbol_t* symbol = NULL;
        // Symbols are sorted by address lo->hi
        for (int i = 0; __symbol_tab[i].addr != (void *)0xFFFFFFFFFFFFFFFF; i++) {
            if ((uint64_t)rip >= (uint64_t)__symbol_tab[i].addr && (uint64_t)rip < (uint64_t)__symbol_tab[i+1].addr) {
                symbol = (symbol_t *)&__symbol_tab[i];
                serial_print_quiet("\t");
                serial_print_quiet(utoa((uint64_t)rip, itoa_buffer, 16));
                serial_print_quiet(" <");
                serial_print_quiet(symbol->name);
                serial_print_quiet(">\r\n");
                break;
            }
        }
        if (symbol == NULL) {
            serial_print_quiet("\t");
            serial_print_quiet(utoa((uint64_t)rip, itoa_buffer, 16));
            serial_print_quiet(" <unknown>\r\n");
        }
        if ((uint64_t)rip != ctx->base_frame.rip)
            rbp = *(uint64_t*)rbp;
        if (rbp == 0) return;
        rip = (void *)(*(uint64_t*)(rbp + 8));
    }
}

void __dump_registers(isr_xframe_t* frame) {
    char itoa_buffer[67];
    serial_print_quiet("\r\n\r\nprocess register dump:\r\n\trax: ");
    serial_print_quiet(utoa((uint64_t)frame->general_registers.rax, itoa_buffer, 16));
    serial_print_quiet(", rbx: ");
    serial_print_quiet(utoa((uint64_t)frame->general_registers.rbx, itoa_buffer, 16));
    serial_print_quiet(", rcx: ");
    serial_print_quiet(utoa((uint64_t)frame->general_registers.rcx, itoa_buffer, 16));
    serial_print_quiet(", rdx: ");
    serial_print_quiet(utoa((uint64_t)frame->general_registers.rdx, itoa_buffer, 16));
    serial_print_quiet(", rdi: ");
    serial_print_quiet(utoa((uint64_t)frame->general_registers.rdi, itoa_buffer, 16));
    serial_print_quiet(", rsi: ");
    serial_print_quiet(utoa((uint64_t)frame->general_registers.rsi, itoa_buffer, 16));
    serial_print_quiet("\r\n\tr8: ");
    serial_print_quiet(utoa((uint64_t)frame->general_registers.r8, itoa_buffer, 16));
    serial_print_quiet(", r9: ");
    serial_print_quiet(utoa((uint64_t)frame->general_registers.r9, itoa_buffer, 16));
    serial_print_quiet(", r10: ");
    serial_print_quiet(utoa((uint64_t)frame->general_registers.r10, itoa_buffer, 16));
    serial_print_quiet(", r11: ");
    serial_print_quiet(utoa((uint64_t)frame->general_registers.r11, itoa_buffer, 16));
    serial_print_quiet("\r\n\tr12: ");
    serial_print_quiet(utoa((uint64_t)frame->general_registers.r12, itoa_buffer, 16));
    serial_print_quiet(", r13: ");
    serial_print_quiet(utoa((uint64_t)frame->general_registers.r13, itoa_buffer, 16));
    serial_print_quiet(", r14: ");
    serial_print_quiet(utoa((uint64_t)frame->general_registers.r14, itoa_buffer, 16));
    serial_print_quiet(", r15: ");
    serial_print_quiet(utoa((uint64_t)frame->general_registers.r15, itoa_buffer, 16));
    serial_print_quiet("\r\n\tcr0: ");
    serial_print_quiet(utoa((uint64_t)frame->control_registers.cr0, itoa_buffer, 16));
    serial_print_quiet(", cr2: ");
    serial_print_quiet(utoa((uint64_t)frame->control_registers.cr2, itoa_buffer, 16));
    serial_print_quiet(", cr3: ");
    serial_print_quiet(utoa((uint64_t)frame->control_registers.cr3, itoa_buffer, 16));
    serial_print_quiet(", cr4: ");
    serial_print_quiet(utoa((uint64_t)frame->control_registers.cr4, itoa_buffer, 16));
    serial_print_quiet("\r\n\trsp: ");
    serial_print_quiet(utoa((uint64_t)frame->base_frame.rsp, itoa_buffer, 16));
    serial_print_quiet(", rbp: ");
    serial_print_quiet(utoa((uint64_t)frame->base_frame.rbp, itoa_buffer, 16));
    serial_print_quiet(", rflags: ");
    serial_print_quiet(utoa((uint64_t)frame->base_frame.rflags, itoa_buffer, 16));
    serial_print_quiet("\r\n\terror code: b*");
    serial_print_quiet(utoa((uint64_t)frame->base_frame.error_code, itoa_buffer, 2));
    serial_print_quiet("\r\n");
}

void __analyze_page_fault(uint64_t code) {
    serial_print_quiet("\r\n\r\npage fault details:\r\n\tviolation: ");
    serial_print_quiet(((code >> 0) & 1) ? "privileged" : "absent");
    serial_print_quiet("\r\n\taccess: ");
    serial_print_quiet(((code >> 1) & 1) ? "write" : "read");
    serial_print_quiet("\r\n\torigin: ");
    serial_print_quiet(((code >> 2) & 1) ? "user" : "kernel");
    serial_print_quiet("\r\n\tbad entry: ");
    serial_print_quiet(((code >> 3) & 1) ? "true" : "false");
    serial_print_quiet("\r\n\texecution: ");
    serial_print_quiet(((code >> 4) & 1) ? "true" : "false");
    serial_print_quiet("\r\n\tpkru: ");
    serial_print_quiet(((code >> 5) & 1) ? "true" : "false");
    serial_print_quiet("\r\n\tshadow: ");
    serial_print_quiet(((code >> 6) & 1) ? "true" : "false");
}

#define PAGE_FAULT_CODE 0x0E

void isr_exception_handler(isr_xframe_t* frame);
void isr_exception_handler(isr_xframe_t* frame) {
    serial_set_input_masked(true);
    serial_print_quiet("FATAL ");
    serial_print_quiet(__exception_labels[frame->base_frame.vector]);
    serial_print_quiet(":\r\n");
    __print_interrupt_stacktrace(frame);
    __dump_registers(frame);
    if (frame->base_frame.vector == PAGE_FAULT_CODE)
        __analyze_page_fault(frame->base_frame.error_code);
    while (true) {
        __asm__ volatile ("cli; hlt");
    }
}
