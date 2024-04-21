#include "cpu/interrupts/idt.h"
#include "iodev/hid/kbd.h"
#include "iodev/hid/ps2/ps2_kbd.h"
#include "iodev/apic/ioapic.h"
#include "iodev/apic/lapic.h"
#include "iodev/uart/uartsh.h"
#include "iodev/io.h"
#include <stdlib.h>
#include <string.h>

#define SCANCODE_SET_1 0x01 // old and compatible scan code set
#define SCANCODE_SET_2 0x02 // default and universal scan code set
#define SCANCODE_SET_3 0x03 // advanced scan code set
#define SCANCODES_SUPPORTED 0x02 // selected scancode set 2

#define PS2_NO_OPERAND 0xff

static uint8_t ps2_kbd_vector   = 0x00;
static uint8_t ps2_kbd_gsi      = 0x00;
static uint8_t ps2_kbd_scan_set = 0x00;

static uint8_t __kbd_scancode_buffer[8];
static uint8_t __kbd_scancode_buffer_pos = 0;

static void __ps2_kbd_clear_scan_buffer() {
    memset(&__kbd_scancode_buffer[0], 0x00, 8);
    __kbd_scancode_buffer_pos = 0;
}

static bool _capslock;
static bool _numlock;
static bool _scrolllock;
static bool _ctrldown;
static bool _altdown;
static bool _fndown;
static bool _shiftdown;

static bool _ismasked;

static bool ps2_test_responsivity() {
    uint8_t status = 0;
    outb(PS2_KBD_DATA_PORT, 0xEE);
    status = inb(PS2_KBD_DATA_PORT);
    return status == 0xEE;
}

static void ps2_buffer_wait() {
    uint8_t status = inb(PS2_KBD_STATUS_REG);
    while (status & 0x02) {
        status = inb(PS2_KBD_STATUS_REG);
    }
}

static bool __commanding = false;
static bool ps2_kbd_send_command(uint8_t command, uint8_t operand) {
    __commanding = true;
    if (!ps2_test_responsivity()) {
        __commanding = false;
        return false;
    }
    uint8_t status = inb(PS2_KBD_DATA_PORT);
    if (status != PS2_KBD_STATUS_ERROR) {
        outb(PS2_KBD_DATA_PORT, command);
        ps2_buffer_wait();
        if (operand != PS2_NO_OPERAND)
            outb(PS2_KBD_DATA_PORT, operand);
        
    }
    uint64_t timeout = 0;
    while (status != PS2_KBD_STATUS_ACK) {
        if (status == PS2_KBD_STATUS_RESEND && timeout < 1) {
            outb(PS2_KBD_COMMAND_PORT, command);
            timeout++;
        }
        if (status == PS2_KBD_STATUS_ERROR) {
            __commanding = false;
            return false;
        }
        status = inb(PS2_KBD_DATA_PORT);
    }
    __commanding = false;
    return true;
}

// Only works with scan code set two
static bool ps2_kbd_needs_next_byte(uint8_t* scancode, size_t bytes) {
    switch (bytes) {
        case 1:
            if (scancode[0] == 0xE0) return true; // many different keys
            if (scancode[0] == 0xF0) return true; // many different keys
            if (scancode[0] == 0xE1) return true; // pause
            break;
        case 2:
            if (scancode[1] == 0xF0) return true; // many different keys
            if (scancode[0] == 0xE0 && scancode[1] == 0x12) return true; // print screen
            if (scancode[0] == 0xE1 && scancode[1] == 0x14) return true; // pause
            break;
        case 3:
            if (scancode[2] == 0x77) return true; // pause
            if (scancode[2] == 0x7C) return true; // print screen
            if (scancode[2] == 0xE0) return true; // print screen
            break;
        case 4:
            if (scancode[3] == 0xE0) return true; // print screen
            if (scancode[3] == 0xE1) return true; // pause
            break;
        case 5:
            if (scancode[4] == 0xF0) return true; // pause and print screen
            break;
        case 6:
            if (scancode[5] == 0x14) return true; // pause
            break;
        case 7:
            if (scancode[6] == 0xF0) return true; // pause
            break;
    };
    return false;
}

static bool ps2_kbd_validate_state() {
    return ((ps2_kbd_scan_set == SCANCODES_SUPPORTED) && !_ismasked);
}

void ps2_kbd_sync_leds() {
    uint8_t leds = 0x00;
    if (_scrolllock) leds |= (1 << 0);
    if (_numlock) leds |= (1 << 1);
    if (_capslock) leds |= (1 << 2);
    ps2_kbd_send_command(PS2_FUNC_SET_LEDS, leds);
}

static hid_keycode_t ps2_translate_keycode(uint64_t rendered_scancode);

void ps2_kbd_handle_scancode(uint8_t* scancode, size_t bytes) {
    uint64_t scancode_rendered = 0x00;
    if (bytes == 0) return;
    for (uint64_t i = 0; i < bytes; i++) {
        scancode_rendered <<= 8;
        scancode_rendered |= scancode[i];
    }
    hid_register_keystroke(ps2_translate_keycode(scancode_rendered));
}

void __kbd_ps2_irq_handler() {
    if (!ps2_kbd_validate_state() || __commanding) {
        ps2_kbd_set_mask(PS2_KBD_MASKED);
        if (!__commanding) inb(PS2_KBD_DATA_PORT);
        apic_local_send_eoi();
        return;
    }
    __kbd_scancode_buffer[__kbd_scancode_buffer_pos] = inb(PS2_KBD_DATA_PORT);
    if (!ps2_kbd_needs_next_byte(&__kbd_scancode_buffer[0], ++__kbd_scancode_buffer_pos)) {
        ps2_kbd_handle_scancode(&__kbd_scancode_buffer[0], __kbd_scancode_buffer_pos);
        __ps2_kbd_clear_scan_buffer();
        apic_local_send_eoi();
        return;
    }
    apic_local_send_eoi();
}

uint8_t _scancode_set_id(uint8_t scancode_set) {
    switch (scancode_set) {
        case 1: return 0x43;
        case 2: return 0x41;
        case 3: return 0x3f;
        default: return 0x00;
    }
}

void ps2_kbd_disable_set_translation() {
    uint8_t config_byte = 0;
    ps2_buffer_wait();
    outb(PS2_KBD_COMMAND_PORT, 0x20); // read config byte
    ps2_buffer_wait();
    config_byte = inb(PS2_KBD_DATA_PORT);
    config_byte &= ~(1 << 6);
    outb(PS2_KBD_COMMAND_PORT, 0x60); // write config byte
    outb(PS2_KBD_DATA_PORT, config_byte);
    ps2_buffer_wait();
}

void ps2_kbd_set_scancodes(uint8_t scancode_set) {
    ps2_kbd_disable_set_translation();
    ps2_kbd_scan_set = scancode_set;
    if (scancode_set == 0) return;
    ps2_kbd_send_command(PS2_FUNC_SCANSET, scancode_set);
    ps2_buffer_wait();
    outb(PS2_KBD_DATA_PORT, PS2_FUNC_SCANSET);
    ps2_buffer_wait();
    outb(PS2_KBD_DATA_PORT, 0x00);
    ps2_buffer_wait();
    inb(PS2_KBD_DATA_PORT); // discard ack
    uint8_t set = inb(PS2_KBD_DATA_PORT);
    if (set != scancode_set)
        ps2_kbd_set_scancodes(scancode_set);
}

void ps2_kbd_reset_state() {
    _capslock = false;
    _numlock = false;
    _scrolllock = false;
    _ctrldown = false;
    _altdown = false;
    _fndown = false;
    _shiftdown = false;
    ps2_kbd_sync_leds();
}

void ps2_kbd_init() {
    if (ps2_kbd_vector != 0x00) return;
    
    ps2_kbd_send_command(PS2_FUNC_ENABLE, PS2_NO_OPERAND);
    outb(PS2_KBD_COMMAND_PORT, 0xAE);
    ps2_kbd_set_scancodes(SCANCODE_SET_2);
    ps2_kbd_reset_state();

    ps2_kbd_vector = idt_allocate_vector();
    idt_install_irq_handler(ps2_kbd_vector, &__kbd_ps2_irq_handler);
    ps2_kbd_gsi = apic_io_get_gsi(PS2_KBD_IRQ_LINE);
    apic_io_redirect_irq(ps2_kbd_gsi, ps2_kbd_vector, false, false);
    ps2_kbd_set_mask(PS2_KBD_MASK_OFF);
    __ps2_kbd_clear_scan_buffer();
    __commanding = false;
}

void ps2_kbd_set_mask(bool mask) {
    if (mask)
        apic_io_mask_irq(ps2_kbd_gsi);
    else
        apic_io_unmask_irq(ps2_kbd_gsi);
    _ismasked = mask;
}

void ps2_kbd_kill() {
    if (ps2_kbd_vector == 0x00) return;
    ps2_kbd_set_mask(PS2_KBD_MASKED);
    apic_io_redirect_irq(ps2_kbd_gsi, 0, false, false);
    idt_set_descriptor(ps2_kbd_vector, 0x00, 0x00, 0x00);
    idt_free_vector(ps2_kbd_vector);
    ps2_kbd_vector = 0x00;
    ps2_kbd_gsi = 0x00;
    outb(PS2_KBD_COMMAND_PORT, 0xAD);
}


// thankfully ai generation and hard work exists
// translates PS/2 scancodes (scan set 2) to kernel-format HID keycodes
static hid_keycode_t ps2_translate_keycode(uint64_t rendered_scancode) {
    switch (rendered_scancode) {
        default:                    return HID_KEYCODE_NULL;
        case 0x01:                  return HID_KEYCODE_F9;
        case 0x03:                  return HID_KEYCODE_F5;
        case 0x04:                  return HID_KEYCODE_F3;
        case 0x05:                  return HID_KEYCODE_F1;
        case 0x06:                  return HID_KEYCODE_F2;
        case 0x07:                  return HID_KEYCODE_F12;
        case 0x09:                  return HID_KEYCODE_F10;
        case 0x0A:                  return HID_KEYCODE_F8;
        case 0x0B:                  return HID_KEYCODE_F6;
        case 0x0C:                  return HID_KEYCODE_F4;
        case 0x0D:                  return HID_KEYCODE_TAB;
        case 0x0E:                  return HID_KEYCODE_BACKTICK;
        case 0x11:                  return HID_KEYCODE_LEFT_ALT;
        case 0x12:                  return HID_KEYCODE_LEFT_SHIFT;
        case 0x14:                  return HID_KEYCODE_LEFT_CTRL;
        case 0x15:                  return HID_KEYCODE_CHAR_Q;
        case 0x16:                  return HID_KEYCODE_NUM1;
        case 0x1A:                  return HID_KEYCODE_CHAR_Z;
        case 0x1B:                  return HID_KEYCODE_CHAR_S;
        case 0x1C:                  return HID_KEYCODE_CHAR_A;
        case 0x1D:                  return HID_KEYCODE_CHAR_W;
        case 0x1E:                  return HID_KEYCODE_NUM2;
        case 0x21:                  return HID_KEYCODE_CHAR_C;
        case 0x22:                  return HID_KEYCODE_CHAR_X;
        case 0x23:                  return HID_KEYCODE_CHAR_D;
        case 0x24:                  return HID_KEYCODE_CHAR_E;
        case 0x25:                  return HID_KEYCODE_NUM4;
        case 0x26:                  return HID_KEYCODE_NUM3;
        case 0x29:                  return HID_KEYCODE_SPACE;
        case 0x2A:                  return HID_KEYCODE_CHAR_V;
        case 0x2B:                  return HID_KEYCODE_CHAR_F;
        case 0x2C:                  return HID_KEYCODE_CHAR_T;
        case 0x2D:                  return HID_KEYCODE_CHAR_R;
        case 0x2E:                  return HID_KEYCODE_NUM5;
        case 0x31:                  return HID_KEYCODE_CHAR_N;
        case 0x32:                  return HID_KEYCODE_CHAR_B;
        case 0x33:                  return HID_KEYCODE_CHAR_H;
        case 0x34:                  return HID_KEYCODE_CHAR_G;
        case 0x35:                  return HID_KEYCODE_CHAR_Y;
        case 0x36:                  return HID_KEYCODE_NUM6;
        case 0x3A:                  return HID_KEYCODE_CHAR_M;
        case 0x3B:                  return HID_KEYCODE_CHAR_J;
        case 0x3C:                  return HID_KEYCODE_CHAR_U;
        case 0x3D:                  return HID_KEYCODE_NUM7;
        case 0x3E:                  return HID_KEYCODE_NUM8;
        case 0x41:                  return HID_KEYCODE_COMMA;
        case 0x42:                  return HID_KEYCODE_CHAR_K;
        case 0x43:                  return HID_KEYCODE_CHAR_I;
        case 0x44:                  return HID_KEYCODE_CHAR_O;
        case 0x45:                  return HID_KEYCODE_NUM0;
        case 0x46:                  return HID_KEYCODE_NUM9;
        case 0x49:                  return HID_KEYCODE_PERIOD;
        case 0x4A:                  return HID_KEYCODE_SLASH;
        case 0x4B:                  return HID_KEYCODE_CHAR_L;
        case 0x4C:                  return HID_KEYCODE_SEMICOLON;
        case 0x4D:                  return HID_KEYCODE_CHAR_P;
        case 0x4E:                  return HID_KEYCODE_MINUS;
        case 0x52:                  return HID_KEYCODE_SINGLEQUOTE;
        case 0x54:                  return HID_KEYCODE_LBRACKET;
        case 0x55:                  return HID_KEYCODE_EQUALS;
        case 0x58:                  return HID_KEYCODE_CAPSLOCK;
        case 0x59:                  return HID_KEYCODE_RIGHT_SHIFT;
        case 0x5A:                  return HID_KEYCODE_ENTER;
        case 0x5B:                  return HID_KEYCODE_RBRACKET;
        case 0x5D:                  return HID_KEYCODE_BACKSLASH;
        case 0x66:                  return HID_KEYCODE_BACKSPACE;
        case 0x69:                  return HID_KEYCODE_KP_1;
        case 0x6B:                  return HID_KEYCODE_KP_4;
        case 0x6C:                  return HID_KEYCODE_KP_7;
        case 0x70:                  return HID_KEYCODE_KP_0;
        case 0x71:                  return HID_KEYCODE_KP_PERIOD;
        case 0x72:                  return HID_KEYCODE_KP_2;
        case 0x73:                  return HID_KEYCODE_KP_5;
        case 0x74:                  return HID_KEYCODE_KP_6;
        case 0x75:                  return HID_KEYCODE_KP_8;
        case 0x76:                  return HID_KEYCODE_ESCAPE;
        case 0x77:                  return HID_KEYCODE_NUMLOCK;
        case 0x78:                  return HID_KEYCODE_F11;
        case 0x79:                  return HID_KEYCODE_KP_PLUS;
        case 0x7A:                  return HID_KEYCODE_KP_3;
        case 0x7B:                  return HID_KEYCODE_KP_MINUS;
        case 0x7C:                  return HID_KEYCODE_KP_ASTERISK;
        case 0x7D:                  return HID_KEYCODE_KP_9;
        case 0x7E:                  return HID_KEYCODE_SCROLLLOCK;
        case 0x83:                  return HID_KEYCODE_F7;
        case 0xE010:                return HID_KEYCODE_MEDIA_SEARCH;
        case 0xE011:                return HID_KEYCODE_RIGHT_ALT;
        case 0xE014:                return HID_KEYCODE_RIGHT_CTRL;
        case 0xE015:                return HID_KEYCODE_TRACK_PREV;
        case 0xE018:                return HID_KEYCODE_MEDIA_FAVS;
        case 0xE01F:                return HID_KEYCODE_SUPER;
        case 0xE020:                return HID_KEYCODE_WWW_REFRESH;
        case 0xE021:                return HID_KEYCODE_VOLUME_DOWN;
        case 0xE023:                return HID_KEYCODE_MEDIA_MUTE;
        case 0xE027:                return HID_KEYCODE_SUPER;
        case 0xE028:                return HID_KEYCODE_WWW_STOP;
        case 0xE02B:                return HID_KEYCODE_MEDIA_CALC;
        case 0xE02F:                return HID_KEYCODE_MEDIA_APPS;
        case 0xE030:                return HID_KEYCODE_WWW_FORWARD;
        case 0xE032:                return HID_KEYCODE_VOLUME_UP;
        case 0xE034:                return HID_KEYCODE_MEDIA_PLAYPAUSE;
        case 0xE037:                return HID_KEYCODE_ACPI_POWER;
        case 0xE038:                return HID_KEYCODE_WWW_BACK;
        case 0xE03A:                return HID_KEYCODE_WWW_HOME;
        case 0xE03B:                return HID_KEYCODE_MEDIA_STOP;
        case 0xE03F:                return HID_KEYCODE_ACPI_SLEEP;
        case 0xE040:                return HID_KEYCODE_MEDIA_MYPC;
        case 0xE048:                return HID_KEYCODE_MEDIA_MAIL;
        case 0xE04A:                return HID_KEYCODE_KP_SLASH;
        case 0xE04D:                return HID_KEYCODE_TRACK_NEXT;
        case 0xE050:                return HID_KEYCODE_MEDIA_SELECT;
        case 0xE05A:                return HID_KEYCODE_KP_ENTER;
        case 0xE05E:                return HID_KEYCODE_ACPI_WAKE;
        case 0xE069:                return HID_KEYCODE_END;
        case 0xE06B:                return HID_KEYCODE_LEFT;
        case 0xE06C:                return HID_KEYCODE_HOME;
        case 0xE070:                return HID_KEYCODE_INSERT;
        case 0xE071:                return HID_KEYCODE_DELETE;
        case 0xE072:                return HID_KEYCODE_DOWN;
        case 0xE074:                return HID_KEYCODE_RIGHT;
        case 0xE075:                return HID_KEYCODE_UP;
        case 0xE07A:                return HID_KEYCODE_PAGEDOWN;
        case 0xE07D:                return HID_KEYCODE_PAGEUP;
        case 0xF001:                return 0x80 | HID_KEYCODE_F9;
        case 0xF003:                return 0x80 | HID_KEYCODE_F5;
        case 0xF004:                return 0x80 | HID_KEYCODE_F3;
        case 0xF005:                return 0x80 | HID_KEYCODE_F1;
        case 0xF006:                return 0x80 | HID_KEYCODE_F2;
        case 0xF007:                return 0x80 | HID_KEYCODE_F12;
        case 0xF009:                return 0x80 | HID_KEYCODE_F10;
        case 0xF00A:                return 0x80 | HID_KEYCODE_F8;
        case 0xF00B:                return 0x80 | HID_KEYCODE_F6;
        case 0xF00C:                return 0x80 | HID_KEYCODE_F4;
        case 0xF00D:                return 0x80 | HID_KEYCODE_TAB;
        case 0xF00E:                return 0x80 | HID_KEYCODE_BACKTICK;
        case 0xF011:                return 0x80 | HID_KEYCODE_LEFT_ALT;
        case 0xF012:                return 0x80 | HID_KEYCODE_LEFT_SHIFT;
        case 0xF014:                return 0x80 | HID_KEYCODE_LEFT_CTRL;
        case 0xF015:                return 0x80 | HID_KEYCODE_CHAR_Q;
        case 0xF016:                return 0x80 | HID_KEYCODE_NUM1;
        case 0xF01A:                return 0x80 | HID_KEYCODE_CHAR_Z;
        case 0xF01B:                return 0x80 | HID_KEYCODE_CHAR_S;
        case 0xF01C:                return 0x80 | HID_KEYCODE_CHAR_A;
        case 0xF01D:                return 0x80 | HID_KEYCODE_CHAR_W;
        case 0xF01E:                return 0x80 | HID_KEYCODE_NUM2;
        case 0xF021:                return 0x80 | HID_KEYCODE_CHAR_C;
        case 0xF022:                return 0x80 | HID_KEYCODE_CHAR_X;
        case 0xF023:                return 0x80 | HID_KEYCODE_CHAR_D;
        case 0xF024:                return 0x80 | HID_KEYCODE_CHAR_E;
        case 0xF025:                return 0x80 | HID_KEYCODE_NUM4;
        case 0xF026:                return 0x80 | HID_KEYCODE_NUM3;
        case 0xF029:                return 0x80 | HID_KEYCODE_SPACE;
        case 0xF02A:                return 0x80 | HID_KEYCODE_CHAR_V;
        case 0xF02B:                return 0x80 | HID_KEYCODE_CHAR_F;
        case 0xF02C:                return 0x80 | HID_KEYCODE_CHAR_T;
        case 0xF02D:                return 0x80 | HID_KEYCODE_CHAR_R;
        case 0xF02E:                return 0x80 | HID_KEYCODE_NUM5;
        case 0xF031:                return 0x80 | HID_KEYCODE_CHAR_N;
        case 0xF032:                return 0x80 | HID_KEYCODE_CHAR_B;
        case 0xF033:                return 0x80 | HID_KEYCODE_CHAR_H;
        case 0xF034:                return 0x80 | HID_KEYCODE_CHAR_G;
        case 0xF035:                return 0x80 | HID_KEYCODE_CHAR_Y;
        case 0xF036:                return 0x80 | HID_KEYCODE_NUM6;
        case 0xF03A:                return 0x80 | HID_KEYCODE_CHAR_M;
        case 0xF03B:                return 0x80 | HID_KEYCODE_CHAR_J;
        case 0xF03C:                return 0x80 | HID_KEYCODE_CHAR_U;
        case 0xF03D:                return 0x80 | HID_KEYCODE_NUM7;
        case 0xF03E:                return 0x80 | HID_KEYCODE_NUM8;
        case 0xF041:                return 0x80 | HID_KEYCODE_COMMA;
        case 0xF042:                return 0x80 | HID_KEYCODE_CHAR_K;
        case 0xF043:                return 0x80 | HID_KEYCODE_CHAR_I;
        case 0xF044:                return 0x80 | HID_KEYCODE_CHAR_O;
        case 0xF045:                return 0x80 | HID_KEYCODE_NUM0;
        case 0xF046:                return 0x80 | HID_KEYCODE_NUM9;
        case 0xF049:                return 0x80 | HID_KEYCODE_PERIOD;
        case 0xF04A:                return 0x80 | HID_KEYCODE_SLASH;
        case 0xF04B:                return 0x80 | HID_KEYCODE_CHAR_L;
        case 0xF04C:                return 0x80 | HID_KEYCODE_SEMICOLON;
        case 0xF04D:                return 0x80 | HID_KEYCODE_CHAR_P;
        case 0xF04E:                return 0x80 | HID_KEYCODE_MINUS;
        case 0xF052:                return 0x80 | HID_KEYCODE_SINGLEQUOTE;
        case 0xF054:                return 0x80 | HID_KEYCODE_LBRACKET;
        case 0xF055:                return 0x80 | HID_KEYCODE_EQUALS;
        case 0xF058:                return 0x80 | HID_KEYCODE_CAPSLOCK;
        case 0xF059:                return 0x80 | HID_KEYCODE_RIGHT_SHIFT;
        case 0xF05A:                return 0x80 | HID_KEYCODE_ENTER;
        case 0xF05B:                return 0x80 | HID_KEYCODE_RBRACKET;
        case 0xF05D:                return 0x80 | HID_KEYCODE_BACKSLASH;
        case 0xF066:                return 0x80 | HID_KEYCODE_BACKSPACE;
        case 0xF069:                return 0x80 | HID_KEYCODE_KP_1;
        case 0xF06B:                return 0x80 | HID_KEYCODE_KP_4;
        case 0xF06C:                return 0x80 | HID_KEYCODE_KP_7;
        case 0xF070:                return 0x80 | HID_KEYCODE_KP_0;
        case 0xF071:                return 0x80 | HID_KEYCODE_KP_PERIOD;
        case 0xF072:                return 0x80 | HID_KEYCODE_KP_2;
        case 0xF073:                return 0x80 | HID_KEYCODE_KP_5;
        case 0xF074:                return 0x80 | HID_KEYCODE_KP_6;
        case 0xF075:                return 0x80 | HID_KEYCODE_KP_8;
        case 0xF076:                return 0x80 | HID_KEYCODE_ESCAPE;
        case 0xF077:                return 0x80 | HID_KEYCODE_NUMLOCK;
        case 0xF078:                return 0x80 | HID_KEYCODE_F11;
        case 0xF079:                return 0x80 | HID_KEYCODE_KP_PLUS;
        case 0xF07A:                return 0x80 | HID_KEYCODE_KP_3;
        case 0xF07B:                return 0x80 | HID_KEYCODE_KP_MINUS;
        case 0xF07C:                return 0x80 | HID_KEYCODE_KP_ASTERISK;
        case 0xF07D:                return 0x80 | HID_KEYCODE_KP_9;
        case 0xF07E:                return 0x80 | HID_KEYCODE_SCROLLLOCK;
        case 0xF083:                return 0x80 | HID_KEYCODE_F7;
        case 0xE0F010:              return 0x80 | HID_KEYCODE_MEDIA_SEARCH;
        case 0xE0F011:              return 0x80 | HID_KEYCODE_RIGHT_ALT;
        case 0xE0F014:              return 0x80 | HID_KEYCODE_RIGHT_CTRL;
        case 0xE0F015:              return 0x80 | HID_KEYCODE_TRACK_PREV;
        case 0xE0F018:              return 0x80 | HID_KEYCODE_MEDIA_FAVS;
        case 0xE0F01F:              return 0x80 | HID_KEYCODE_SUPER;
        case 0xE0F020:              return 0x80 | HID_KEYCODE_WWW_REFRESH;
        case 0xE0F021:              return 0x80 | HID_KEYCODE_VOLUME_DOWN;
        case 0xE0F023:              return 0x80 | HID_KEYCODE_MEDIA_MUTE;
        case 0xE0F027:              return 0x80 | HID_KEYCODE_SUPER;
        case 0xE0F028:              return 0x80 | HID_KEYCODE_WWW_STOP;
        case 0xE0F02B:              return 0x80 | HID_KEYCODE_MEDIA_CALC;
        case 0xE0F02F:              return 0x80 | HID_KEYCODE_MEDIA_APPS;
        case 0xE0F030:              return 0x80 | HID_KEYCODE_WWW_FORWARD;
        case 0xE0F032:              return 0x80 | HID_KEYCODE_VOLUME_UP;
        case 0xE0F034:              return 0x80 | HID_KEYCODE_MEDIA_PLAYPAUSE;
        case 0xE0F037:              return 0x80 | HID_KEYCODE_ACPI_POWER;
        case 0xE0F038:              return 0x80 | HID_KEYCODE_WWW_BACK;
        case 0xE0F03A:              return 0x80 | HID_KEYCODE_WWW_HOME;
        case 0xE0F03B:              return 0x80 | HID_KEYCODE_MEDIA_STOP;
        case 0xE0F03F:              return 0x80 | HID_KEYCODE_ACPI_SLEEP;
        case 0xE0F040:              return 0x80 | HID_KEYCODE_MEDIA_MYPC;
        case 0xE0F048:              return 0x80 | HID_KEYCODE_MEDIA_MAIL;
        case 0xE0F04A:              return 0x80 | HID_KEYCODE_KP_SLASH;
        case 0xE0F04D:              return 0x80 | HID_KEYCODE_TRACK_NEXT;
        case 0xE0F050:              return 0x80 | HID_KEYCODE_MEDIA_SELECT;
        case 0xE0F05A:              return 0x80 | HID_KEYCODE_KP_ENTER;
        case 0xE0F05E:              return 0x80 | HID_KEYCODE_ACPI_WAKE;
        case 0xE0F069:              return 0x80 | HID_KEYCODE_END;
        case 0xE0F06B:              return 0x80 | HID_KEYCODE_LEFT;
        case 0xE0F06C:              return 0x80 | HID_KEYCODE_HOME;
        case 0xE0F070:              return 0x80 | HID_KEYCODE_INSERT;
        case 0xE0F071:              return 0x80 | HID_KEYCODE_DELETE;
        case 0xE0F072:              return 0x80 | HID_KEYCODE_DOWN;
        case 0xE0F074:              return 0x80 | HID_KEYCODE_RIGHT;
        case 0xE0F075:              return 0x80 | HID_KEYCODE_UP;
        case 0xE0F07A:              return 0x80 | HID_KEYCODE_PAGEDOWN;
        case 0xE0F07D:              return 0x80 | HID_KEYCODE_PAGEUP;
        case 0xE012E07C:            return HID_KEYCODE_PRINTSCREEN;
        case 0xE0F07CE0F012:        return 0x80 | HID_KEYCODE_PRINTSCREEN;
        case 0xE11477E1F014F077:    return HID_KEYCODE_PAUSE;
        // There is no pause release scan code
    }
}