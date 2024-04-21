#include "cpu/interrupts/idt.h"
#include "iodev/uart/uartsh.h"
#include "iodev/io.h"
#include "iodev/apic/ioapic.h"
#include "iodev/apic/lapic.h"
#include "mem/pmm/pmm.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#define SHELL_PROMPT "glass #> "
#define PROMPT_LENGTH 9

static void uart_shell_write(const char* s);

#define UNSUPPORTED_LF 0x0A
#define SUPPORTED_CR 0x0D

static bool prompt_placed;

static uint16_t output_port = 0;
extern uint16_t __com_port_addrs[9];

static uint8_t* command_buffer;
static uint64_t command_buffer_index;

static void __uart_internal_write(const char* s, uint16_t port) {
    if (port == 0) return;
    char* __s = (char*)s;
    while (*__s != '\0') {
        outb(__com_port_addrs[port] + COM_REGISTER_DATA, *__s);
        __s++;
    }
}

// Called before writing to serial output
void uart_shell_cancel_prompt() {
    if (!prompt_placed) return;
    char s[PROMPT_LENGTH + 1];
    memset(s, '\b', PROMPT_LENGTH);
    s[PROMPT_LENGTH] = '\0';
    __uart_internal_write(s, output_port);
    prompt_placed = false;
}
// Called after writing to serial output
void uart_shell_replace_prompt() {
    if (prompt_placed) uart_shell_cancel_prompt();
    __uart_internal_write(SHELL_PROMPT, output_port);
    prompt_placed = true;
}

static void uart_shell_write(const char* s) {
    uart_shell_cancel_prompt();
    if (output_port == 0) return;
    __uart_internal_write(s, output_port);
    uart_shell_replace_prompt();
}

static void __shell_command_help();
static void __shell_command_ping();
static void __shell_command_dump_idt();
static void __shell_command_dump_gdt();
static void __shell_command_dump_tss();
static void __shell_command_dump_proc();
static void __shell_command_mem_map();
static void __shell_command_uart_info();
static void __shell_command_pci_info();
static void __shell_command_clear();

#define NUMBER_OF_COMMANDS 10

void(*__shell_command_handlers[NUMBER_OF_COMMANDS])() = {
    __shell_command_help,
    __shell_command_ping,
    __shell_command_dump_idt,
    __shell_command_dump_gdt,
    __shell_command_dump_tss,
    __shell_command_dump_proc,
    __shell_command_mem_map,
    __shell_command_uart_info,
    __shell_command_pci_info,
    __shell_command_clear
};

const char* __uart_shell_commands[NUMBER_OF_COMMANDS] = {
    "help",
    "ping",
    "idt",
    "gdt",
    "tss",
    "lsp",
    "mem",
    "uart",
    "pci",
    "clear"
};

static void uart_shell_command_execute(uint16_t command_no) {
    if (command_no >= NUMBER_OF_COMMANDS) return;
    __shell_command_handlers[command_no]();
}

static void uart_shell_clear_buffer() {
    memset((void *)command_buffer, 0, PAGING_PAGE_SIZE);
    command_buffer_index = 0;
}

static bool __uart_command_match(const char* input, const char* command) {
    unsigned char* __s1 = (unsigned char *)input;
    unsigned char* __s2 = (unsigned char *)command;
    for (/**/; /**/; __s1++, __s2++) {
        if (*__s1 == 0 && *__s2 == 0)
            return true;
		if (*__s1 != *__s2)
			return false;
	}
}

#define UARTSH_BUFFER_MAX 4094
static void uart_command_buffer_append(uint8_t c) {
    if (command_buffer_index <= UARTSH_BUFFER_MAX)
        command_buffer[command_buffer_index++] = c;
}

static void uart_shell_command_finalize();

void __uart_shell_handle_character(uint8_t c) {
    if (c == UNSUPPORTED_LF || c == SUPPORTED_CR) {
        c = SUPPORTED_CR;
        __uart_internal_write("\r\n", output_port);
        uart_shell_command_finalize();
        return;
    }
    if ((c < 0x20 || c > 0x7E) && c != SUPPORTED_CR) return;
    uint8_t cs[2] = {c == SUPPORTED_CR ? UNSUPPORTED_LF : c, '\0'};
    __uart_internal_write((const char *)&cs[0], output_port);
    uart_command_buffer_append(c);
}

static void uart_shell_command_finalize() {
    if (command_buffer_index == 0) {
        prompt_placed = false;
        uart_shell_replace_prompt();
        return;
    }
    command_buffer[command_buffer_index] = '\0';
    char* command = (char*)command_buffer;
    for (uint16_t i = 0; i < NUMBER_OF_COMMANDS; i++) {
        if (__uart_command_match(command, __uart_shell_commands[i])) {
            uart_shell_command_execute(i);
            prompt_placed = false;
            uart_shell_replace_prompt();
            uart_shell_clear_buffer();
            return;
        }
    }
    uart_shell_write("Unknown command...\r\n");
    uart_shell_clear_buffer();
}

void uart_shell_start(uint16_t out_port) {
    output_port = out_port;
    command_buffer = (uint8_t*)pmm_alloc_page();
    command_buffer_index = 0;
    memset(command_buffer, 0, PAGING_PAGE_SIZE);
    prompt_placed = false;
    uart_shell_replace_prompt();
}

void serial_print_quiet(const char* s) {
    __uart_internal_write(s, output_port);    
}

static void __shell_command_help() {
    __uart_internal_write("You are using the \"glass\" kernel debug shell through the serial port. Available commands:\r\n\r\n", output_port);
    __uart_internal_write("help: print this message\r\n", output_port);
    __uart_internal_write("ping: print \"pong\"\r\n", output_port);
    __uart_internal_write("idt: print dump of idt structure\r\n", output_port);
    __uart_internal_write("gdt: print dump of gdt structure\r\n", output_port);
    __uart_internal_write("tss: print dump of tss structure\r\n", output_port);
    __uart_internal_write("lsp: print dump of current processes\r\n", output_port);
    __uart_internal_write("mem: print info on memory state\r\n", output_port);
    __uart_internal_write("uart: print info on uart (serial) state\r\n", output_port);
    __uart_internal_write("pci: print list of pci functions\r\n", output_port);
    __uart_internal_write("clear: attempt to clear the screen (compatibility varies)\r\n", output_port);
    __uart_internal_write("\r\n", output_port);
}

static void __shell_command_ping() {
    __uart_internal_write("pong\r\n", output_port);
}

/// NOTICE: IMPORTANT!
/// ALL UARTSH FUNCTIONS MUST USE CRLF "\r\n" FOR PRINTING NEWLINES
/// THIS ENSURES LINE DISCIPLINE AMONG DIFFERENT SERIAL MONITORS, INCLUDING DEFAULT QEMU
extern void __uartsh_idt_dump();
static void __shell_command_dump_idt() {
    __uartsh_idt_dump();
}

extern void __uartsh_gdt_dump();
static void __shell_command_dump_gdt() {
    __uartsh_gdt_dump();
}

extern void __uartsh_tss_dump();
static void __shell_command_dump_tss() {
    __uartsh_tss_dump();
}

extern void __uartsh_proc_dump();
static void __shell_command_dump_proc() {
    __uartsh_proc_dump();
}

extern void __uartsh_pmm_dump();
static void __shell_command_mem_map() {
    __uartsh_pmm_dump();
}

extern void __uartsh_uart_dump();
static void __shell_command_uart_info() {
    __uartsh_uart_dump();
}

extern void __uartsh_pci_dump();
static void __shell_command_pci_info() {
    __uartsh_pci_dump();
}

static void __shell_command_clear() {
    __uart_internal_write("Viewing this message indicates failure: attempting to clear screen with escape sequences \\033[2J and \\033[H...\r\n", output_port);
    __uart_internal_write("NOTICE: Some monitors may simply scroll the text out of view, this indicates success.\r\n", output_port);
    __uart_internal_write("\033[2J\r\n\033[H", output_port);
}
