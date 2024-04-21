#pragma once
#include "extra/conv.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define SERIAL_COM1_PORT 0x3F8
#define SERIAL_COM2_PORT 0x2F8
#define SERIAL_COM3_PORT 0x3E8
#define SERIAL_COM4_PORT 0x2E8
#define SERIAL_COM5_PORT 0x5F8
#define SERIAL_COM6_PORT 0x4F8
#define SERIAL_COM7_PORT 0x5E8
#define SERIAL_COM8_PORT 0x4E8

#define COM_REGISTER_DATA       0x00
#define COM_REGISTER_INTERRUPT  0x01
#define COM_REGISTER_BAUD_LOW   0x00
#define COM_REGISTER_BAUD_HIGH  0x01
#define COM_REGISTER_FIFO       0x02
#define COM_REGISTER_LINE_CTRL  0x03
#define COM_REGISTER_MODEM_CTRL 0x04
#define COM_REGISTER_LINE_STAT  0x05
#define COM_REGISTER_MODEM_STAT 0x06
#define COM_REGISTER_SCRATCH    0x07

#define SERIAL_COM1_IRQ 4
#define SERIAL_COM2_IRQ 3

void serial_console_enable();
void serial_print_error(const char* error);
void serial_print_quiet(const char* s);
void serial_set_input_masked(bool masked);
