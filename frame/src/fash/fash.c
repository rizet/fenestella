#include <string.h>
#include "fash/fash.h"
#include "kbd/kbd.h"
#include "lib/glass.h"

static bool _started = false;
static bool _prompted = false;

static uint8_t* __input_buffer = NULL;
static uint16_t __buffer_index = 0;

static void _fash_buffer_backspace() {
    if (__buffer_index == 0) return;
    __input_buffer[--__buffer_index] = 0x00;
    if (_prompted) tty_putc('\b', 0x00);
}

static void _fash_command_finalize() {
    tty_write("\n");
    fash_execute_command((const char *)__input_buffer);
    memset(__input_buffer, 0x00, 0x1000);
    __buffer_index = 0;
    _prompted = false;
}

void __fash_keyboard_handle_event(uint8_t keycode) {
    if (!_started || __input_buffer == NULL) return;
    if (keycode & 0x80) return;
    char decoded = kbd_char_decode(keycode);
    if (__buffer_index >= 0x1000) return;
    if (keycode == HID_KEYCODE_BACKSPACE) {
        _fash_buffer_backspace();
        return;
    }
    if (keycode == HID_KEYCODE_ENTER) {
        _fash_command_finalize();
        return;
    }
    if (decoded == 0x00) return;
    __input_buffer[__buffer_index++] = decoded;
    char buf[2] = {decoded, 0x00};
    if (_prompted) tty_write(buf);
}

void fash_start() {
    if (_started) return;
    __input_buffer = (uint8_t *)pmap(NULL);
    if (__input_buffer == NULL) {
        tty_write("[fash] ERROR: failed to allocate input buffer!\n");
        return;
    }
    __buffer_index = 0;
    memset(__input_buffer, 0x00, 0x1000);
    if (!kbd_add_listener(__fash_keyboard_handle_event)) {
        tty_write("[fash] ERROR: failed to add keyboard listener!\n");
        return;
    }
    _started = true;
    _prompted = false;
    tty_write("[fash] started.\n\n\n");
}

#define FASH_PROMPT_STRING ">fash> # "
static void _fash_verify_prompt() {
    if (!_prompted) {
        tty_write(FASH_PROMPT_STRING);
        _prompted = true;
    }
}

bool fash_started() {
    return _started;
}

void fash_continue() {
    if (!_started) return;
    _fash_verify_prompt();
}
