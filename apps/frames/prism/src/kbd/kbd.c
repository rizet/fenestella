#include "kbd/kbd.h"
#include <string.h>

static uint8_t* internal_buffer = NULL;
static uint16_t* internal_buffer_size = NULL;
static bool _started = false;

static kbd_listener_t* listeners = NULL;
static uint64_t listeners_count = 0;

static bool _kb_shift = false;
static bool _kb_caps = false;
static bool _kb_nums = false;

char kbd_char_decode(uint8_t keycode) {
    keycode &= 0x7F;
    if (keycode > 0x42)
        return 0x00;
    if (keycode == 0x00)
        return 0x00;
    switch (_kb_shift) {
        case true:
            switch (_kb_caps) {
                case true:
                    switch (_kb_nums) {
                        case true:
                            return kbd_key_ascii_shift_numlock_capslock[keycode];
                        case false:
                            return kbd_key_ascii_shift_capslock[keycode];
                        default:
                            return 0x00;
                    }
                    break;
                case false:
                    switch (_kb_nums) {
                        case true:
                            return kbd_key_ascii_shift_numlock[keycode];
                        case false:
                            return kbd_key_ascii_shift[keycode];
                        default:
                            return 0x00;
                    }
                    break;
                default:
                    return 0x00;
            }
            break;
        case false:
            switch (_kb_caps) {
                case true:
                    switch (_kb_nums) {
                        case true:
                            return kbd_key_ascii_numlock_capslock[keycode];
                        case false:
                            return kbd_key_ascii_capslock[keycode];
                        default:
                            return 0x00;
                    }
                    break;
                case false:
                    switch (_kb_nums) {
                        case true:
                            return kbd_key_ascii_numlock[keycode];
                        case false:
                            return kbd_key_ascii_default[keycode];
                        default:
                            return 0x00;
                    }
                    break;
                default:
                    return 0x00;
            }
            break;
        default:
            return 0x00;
    }
}

bool kbd_init() {
    if (_started) return true;

    internal_buffer = (uint8_t *)kb_man(true);
    if (internal_buffer == NULL) {
        return false;
    }

    internal_buffer_size = (uint16_t *)((uint64_t)internal_buffer + 0x1000 - sizeof(uint16_t));
    if (*internal_buffer_size != 0) {
        return false;
    }

    if (listeners != NULL) {
        return false;
    }
    listeners = (kbd_listener_t *)pmap(NULL);
    if (listeners == NULL) {
        return false;
    }
    listeners_count = 0;
    memset((void *)listeners, 0x00, 0x1000);

    _started = true;

    return _started;
}

void kbd_kill() {
    _started = false;
    kb_man(false);
    internal_buffer = NULL;
    internal_buffer_size = NULL;
}

bool kbd_add_listener(kbd_listener_t listener) {
    if (!_started) return false;
    if (listeners_count >= (0x1000 / sizeof(kbd_listener_t))) return false;
    listeners[listeners_count++] = listener;
    return true;
}

void kbd_remove_listener(kbd_listener_t listener) {
    if (!_started) return;
    uint64_t target_index = 0;
    for (uint64_t i = 0; i < listeners_count; i++) {
        if (listeners[i] == listener) {
            target_index = i;
            break;
        }
    }
    if (target_index == 0) return;
    listeners[target_index] = NULL;
    for (uint64_t i = target_index; i < listeners_count; i++) {
        listeners[i] = listeners[i + 1];
    }
    listeners[listeners_count--] = NULL;
    return;
}

void kbd_update() {
    if (!_started) return;
    uint16_t buffered_keys = *internal_buffer_size;
    if (buffered_keys == 0) return;
    for (uint16_t i = 0; i < buffered_keys; i++) {
        uint8_t keycode = internal_buffer[i];
        
        switch (keycode) {
            case HID_KEYCODE_CAPSLOCK:
                _kb_caps = !_kb_caps;
                break;
            case HID_KEYCODE_NUMLOCK:
                _kb_nums = !_kb_nums;
                break;
            case HID_KEYCODE_LEFT_SHIFT:
            case HID_KEYCODE_RIGHT_SHIFT:
                _kb_shift = true;
                break;
            case (HID_KEYCODE_LEFT_SHIFT | 0x80):
            case (HID_KEYCODE_RIGHT_SHIFT | 0x80):
                _kb_shift = false;
                break;
            default:
                break;
        }

        for (uint64_t j = 0; j < listeners_count; j++) {
            if (listeners[j] != NULL) listeners[j](keycode);
        }
    }
    memset((void *)internal_buffer, 0x00, 0x1000);
}
