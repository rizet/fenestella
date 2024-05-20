#include "iodev/hid/kbd.h"
#include "iodev/hid/ps2/ps2_kbd.h"

void hid_enable_keyboard_interrupts() {
    ps2_kbd_init();
}

extern void __kb_man_save_key(uint8_t keycode);
bool hid_register_keystroke(uint8_t keycode) {
    switch (keycode) {
        case HID_KEYCODE_NULL:
            return false;
        default:
            __kb_man_save_key(keycode);
            return true;
    }
}
