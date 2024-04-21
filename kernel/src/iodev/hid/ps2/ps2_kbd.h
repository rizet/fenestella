#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

void ps2_kbd_init();
void ps2_kbd_set_mask(bool mask);
void ps2_kbd_kill();

#define PS2_KBD_IRQ_LINE 1

#define PS2_KBD_MASKED      true
#define PS2_KBD_MASK_OFF    false

#define PS2_KBD_STATUS_ACK      0xFA
#define PS2_KBD_STATUS_RESEND   0xFE
#define PS2_KBD_STATUS_ERROR    0xFC

#define PS2_FUNC_SET_LEDS       0xED
#define PS2_FUNC_SCANSET        0xF0
#define PS2_FUNC_ENABLE         0xF4
