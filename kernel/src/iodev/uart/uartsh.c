#include "cpu/interrupts/idt.h"
#include "iodev/uart/uartsh.h"
#include "iodev/io.h"
#include "iodev/apic/ioapic.h"
#include "iodev/apic/lapic.h"
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

static uint8_t available_com_ports[8] = {
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00
};

static bool _input_masked;
static uint8_t input_com_gsi;
static uint8_t input_idt_vector;
static uint16_t input_port;
static uint16_t output_port;

uint16_t __com_port_addrs[9] = {
    0,
    SERIAL_COM1_PORT,
    SERIAL_COM2_PORT,
    SERIAL_COM3_PORT,
    SERIAL_COM4_PORT,
    SERIAL_COM5_PORT,
    SERIAL_COM6_PORT,
    SERIAL_COM7_PORT,
    SERIAL_COM8_PORT
};


static void serial_write(const char* s);

static bool uart_input_data_available(uint16_t port) {
    return (inb(__com_port_addrs[input_port] + COM_REGISTER_LINE_STAT)) & 0x1; // data ready bit
}

extern void __uart_shell_handle_character(uint8_t c);

void __uart_input_irq_handler() {
    if (_input_masked) return;
    while (uart_input_data_available(input_port)) {
        uint8_t c = inb(__com_port_addrs[input_port] + COM_REGISTER_DATA);
        __uart_shell_handle_character(c);
    }
    apic_local_send_eoi();
}

static bool serial_check_port(uint16_t port) {
    if (port == 0 || port > 8) return false;
    outb(__com_port_addrs[port] + COM_REGISTER_SCRATCH, 0xAF);   // probe by writing to and reading back scratchr register
    uint8_t test = inb(__com_port_addrs[port] + 0x07);
    return (test == 0xAF);
}

static void serial_configure_port(uint16_t port) {
    if (!serial_check_port(port)) return;

    uint8_t line_control = inb(__com_port_addrs[port] + COM_REGISTER_LINE_CTRL);
    line_control |= 0x80;   // enable DLAB, the MSB
    outb(__com_port_addrs[port] + COM_REGISTER_LINE_CTRL, line_control); // set dlab bit to access divisor
    outb(__com_port_addrs[port] + COM_REGISTER_BAUD_LOW, 0x01);   // divisor = 1, lsb
    outb(__com_port_addrs[port] + COM_REGISTER_BAUD_HIGH, 0x00); // divisor = 1, msb

    line_control = inb(__com_port_addrs[port] + COM_REGISTER_LINE_CTRL);
    line_control &= ~0x80;    // disable DLAB
    line_control &= ~0x4;   // 1 stop bit
    line_control &= 0x38; // no parity, clear bits
    line_control |= 0x3; // 8 data bits
    outb(__com_port_addrs[port] + COM_REGISTER_LINE_CTRL, line_control); // disable dlab, divisor already set
}

static void serial_select_input_port(uint16_t port) {
    if (port < 1 || port > 2) return;
    input_port = port;
    input_com_gsi = apic_io_get_gsi((port == 1) ? SERIAL_COM1_IRQ : SERIAL_COM2_IRQ);
    if (input_idt_vector == 0x00)
        input_idt_vector = idt_allocate_vector();
    idt_install_irq_handler(input_idt_vector, &__uart_input_irq_handler);
    apic_io_redirect_irq(input_com_gsi, input_idt_vector, false, false);
    uint8_t interrupt_byte = 0x1; // enabled data received interrupt
    outb(__com_port_addrs[port] + COM_REGISTER_INTERRUPT, interrupt_byte);
    if (!_input_masked)
        apic_io_unmask_irq(input_com_gsi);
}

static void serial_select_output_port(uint16_t port) {
    if (port < 1 || port > 8) return;
    output_port = port;
}

void serial_set_input_masked(bool masked) {
    _input_masked = masked;
    if (input_com_gsi != 0) {
        if (_input_masked)
            apic_io_mask_irq(input_com_gsi);
        else
            apic_io_unmask_irq(input_com_gsi);
    }
}

extern void uart_shell_start(uint16_t out_port);

void serial_console_enable() {
    _input_masked = true;
    input_idt_vector = 0;
    input_com_gsi = 0;
    input_port = 0;
    output_port = 0;
    for (uint64_t i = 1; i < 9; i++) {
        if (serial_check_port(i)) {
            available_com_ports[i] = true;
            serial_configure_port(i);
        }
    }
    for (uint64_t i = 1; i <= 2; i++) {
        if (available_com_ports[i]) {
            serial_select_input_port(i);
            break;
        }
    }
    for (uint64_t i = 1; i <= 8; i++) {
        if (available_com_ports[i]) {
            serial_select_output_port(i);
            break;
        }
    }
    serial_set_input_masked(false);
    serial_write("[debug] serial console enabled!\r\n");
    uart_shell_start(output_port);
    serial_write("[info] no backspace support, re-enter command to resolve typos...\r\n");
}


extern void uart_shell_cancel_prompt();
extern void uart_shell_replace_prompt();

static void serial_write(const char* s) {
    uart_shell_cancel_prompt();
    if (output_port == 0) return;
    char* __s = (char*)s;
    while (*__s != '\0') {
        outb(__com_port_addrs[output_port] + COM_REGISTER_DATA, *__s);
        __s++;
    }
    uart_shell_replace_prompt();
}

void serial_print_error(const char* error) {
    uart_shell_cancel_prompt();
    serial_write("[ERROR] ");
    serial_write(error);
    uart_shell_replace_prompt();
}

void __uartsh_uart_dump() {
    printf("\r\nUART info:\r\n\r\nCOM port statuses:");
    for (uint8_t i = 0; i < 8; i++) {
        if (available_com_ports[i])
            printf(" y");
        else
            printf(" n");
    }
    printf("\r\nInput port: %d\r\n", input_port);
    printf("Output port: %d\r\n", output_port);
    printf("Input GSI/vector: %d/%d %s\r\n\r\n", input_com_gsi, input_idt_vector, (_input_masked ? " (masked)" : " (unmasked)"));
}
