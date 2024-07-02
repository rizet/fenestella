#pragma once
#include <stdint.h>
#include <stdbool.h>

#define PS2_KBD_COMMAND_PORT 0x64
#define PS2_KBD_STATUS_REG 0x64
#define PS2_KBD_DATA_PORT 0x60

void hid_enable_keyboard_interrupts();
void hid_clear_keystroke_buffer();
void hid_register_user_key_buffer(void* user_keystroke_buffer);
bool hid_register_keystroke(uint8_t keycode);

typedef uint8_t hid_keycode_t;
// highest bit is set if key is released
enum HID_KERNEL_KEYCODE {
    HID_KEYCODE_NULL            = 0x00,
    HID_KEYCODE_BACKTICK        = 0x01,
    HID_KEYCODE_NUM1            = 0x02,
    HID_KEYCODE_NUM2            = 0x03,
    HID_KEYCODE_NUM3            = 0x04,
    HID_KEYCODE_NUM4            = 0x05,
    HID_KEYCODE_NUM5            = 0x06,
    HID_KEYCODE_NUM6            = 0x07,
    HID_KEYCODE_NUM7            = 0x08,
    HID_KEYCODE_NUM8            = 0x09,
    HID_KEYCODE_NUM9            = 0x0A,
    HID_KEYCODE_NUM0            = 0x0B,
    HID_KEYCODE_MINUS           = 0x0C,
    HID_KEYCODE_EQUALS          = 0x0D,
    HID_KEYCODE_CHAR_Q          = 0x0E,
    HID_KEYCODE_CHAR_W          = 0x0F,
    HID_KEYCODE_CHAR_E          = 0x10,
    HID_KEYCODE_CHAR_R          = 0x11,
    HID_KEYCODE_CHAR_T          = 0x12,
    HID_KEYCODE_CHAR_Y          = 0x13,
    HID_KEYCODE_CHAR_U          = 0x14,
    HID_KEYCODE_CHAR_I          = 0x15,
    HID_KEYCODE_CHAR_O          = 0x16,
    HID_KEYCODE_CHAR_P          = 0x17,
    HID_KEYCODE_LBRACKET        = 0x18,
    HID_KEYCODE_RBRACKET        = 0x19,
    HID_KEYCODE_BACKSLASH       = 0x1A,
    HID_KEYCODE_CHAR_A          = 0x1B,
    HID_KEYCODE_CHAR_S          = 0x1C,
    HID_KEYCODE_CHAR_D          = 0x1D,
    HID_KEYCODE_CHAR_F          = 0x1E,
    HID_KEYCODE_CHAR_G          = 0x1F,
    HID_KEYCODE_CHAR_H          = 0x20,
    HID_KEYCODE_CHAR_J          = 0x21,
    HID_KEYCODE_CHAR_K          = 0x22,
    HID_KEYCODE_CHAR_L          = 0x23,
    HID_KEYCODE_SEMICOLON       = 0x24,
    HID_KEYCODE_SINGLEQUOTE     = 0x25,
    HID_KEYCODE_CHAR_Z          = 0x26,
    HID_KEYCODE_CHAR_X          = 0x27,
    HID_KEYCODE_CHAR_C          = 0x28,
    HID_KEYCODE_CHAR_V          = 0x29,
    HID_KEYCODE_CHAR_B          = 0x2A,
    HID_KEYCODE_CHAR_N          = 0x2B,
    HID_KEYCODE_CHAR_M          = 0x2C,
    HID_KEYCODE_COMMA           = 0x2D,
    HID_KEYCODE_PERIOD          = 0x2E,
    HID_KEYCODE_SLASH           = 0x2F,
    HID_KEYCODE_SPACE           = 0x30,
    HID_KEYCODE_KP_SLASH        = 0x31,
    HID_KEYCODE_KP_ASTERISK     = 0x32,
    HID_KEYCODE_KP_MINUS        = 0x33,
    HID_KEYCODE_KP_PLUS         = 0x33,
    HID_KEYCODE_KP_7            = 0x34,
    HID_KEYCODE_KP_8            = 0x35,
    HID_KEYCODE_KP_9            = 0x36,
    HID_KEYCODE_KP_4            = 0x37,
    HID_KEYCODE_KP_5            = 0x38,
    HID_KEYCODE_KP_6            = 0x39,
    HID_KEYCODE_KP_1            = 0x3A,
    HID_KEYCODE_KP_2            = 0x3B,
    HID_KEYCODE_KP_3            = 0x3C,
    HID_KEYCODE_KP_0            = 0x3D,
    HID_KEYCODE_KP_PERIOD       = 0x3E,
    HID_KEYCODE_KP_ENTER        = 0x3F,
    HID_KEYCODE_ENTER           = 0x40,
    HID_KEYCODE_TAB             = 0x41,
    HID_KEYCODE_BACKSPACE       = 0x42,
    HID_KEYCODE_ESCAPE          = 0x43,
    HID_KEYCODE_MEDIA_MYPC      = 0x44,
    HID_KEYCODE_LEFT_SHIFT      = 0x45,
    HID_KEYCODE_RIGHT_SHIFT     = 0x46,
    HID_KEYCODE_LEFT_CTRL       = 0x47,
    HID_KEYCODE_RIGHT_CTRL      = 0x48,
    HID_KEYCODE_LEFT_ALT        = 0x49,
    HID_KEYCODE_RIGHT_ALT       = 0x4A,
    HID_KEYCODE_CAPSLOCK        = 0x4B,
    HID_KEYCODE_SCROLLLOCK      = 0x4C,
    HID_KEYCODE_NUMLOCK         = 0x4D,
    HID_KEYCODE_INSERT          = 0x4E,
    HID_KEYCODE_HOME            = 0x4F,
    HID_KEYCODE_PAGEUP          = 0x50,
    HID_KEYCODE_DELETE          = 0x51,
    HID_KEYCODE_END             = 0x52,
    HID_KEYCODE_PAGEDOWN        = 0x53,
    HID_KEYCODE_UP              = 0x54,
    HID_KEYCODE_LEFT            = 0x55,
    HID_KEYCODE_DOWN            = 0x56,
    HID_KEYCODE_RIGHT           = 0x57,
    HID_KEYCODE_F1              = 0x58,
    HID_KEYCODE_F2              = 0x59,
    HID_KEYCODE_F3              = 0x5A,
    HID_KEYCODE_F4              = 0x5B,
    HID_KEYCODE_F5              = 0x5C,
    HID_KEYCODE_F6              = 0x5D,
    HID_KEYCODE_F7              = 0x5E,
    HID_KEYCODE_F8              = 0x5F,
    HID_KEYCODE_F9              = 0x60,
    HID_KEYCODE_F10             = 0x61,
    HID_KEYCODE_F11             = 0x62,
    HID_KEYCODE_F12             = 0x63,
    HID_KEYCODE_PRINTSCREEN     = 0x64,
    HID_KEYCODE_PAUSE           = 0x65,
    HID_KEYCODE_SUPER           = 0x66,
    HID_KEYCODE_MENU            = 0x67,
    HID_KEYCODE_MEDIA_WWW       = 0x68,
    HID_KEYCODE_TRACK_PREV      = 0x69,
    HID_KEYCODE_TRACK_NEXT      = 0x6A,
    HID_KEYCODE_MEDIA_FAVS      = 0x6B,
    HID_KEYCODE_VOLUME_DOWN     = 0x6C,
    HID_KEYCODE_VOLUME_UP       = 0x6D,
    HID_KEYCODE_MEDIA_MUTE      = 0x6E,
    HID_KEYCODE_MEDIA_PLAYPAUSE = 0x6F,
    HID_KEYCODE_MEDIA_STOP      = 0x70,
    HID_KEYCODE_WWW_FORWARD     = 0x71,
    HID_KEYCODE_WWW_BACK        = 0x72,
    HID_KEYCODE_WWW_HOME        = 0x73,
    HID_KEYCODE_WWW_STOP        = 0x74,
    HID_KEYCODE_WWW_REFRESH     = 0x75,
    HID_KEYCODE_MEDIA_CALC      = 0x76,
    HID_KEYCODE_MEDIA_MAIL      = 0x77,
    HID_KEYCODE_MEDIA_SELECT    = 0x78,
    HID_KEYCODE_MEDIA_APPS      = 0x79,
    HID_KEYCODE_MEDIA_SEARCH    = 0x7A,
    HID_KEYCODE_ACPI_POWER      = 0x7B,
    HID_KEYCODE_ACPI_SLEEP      = 0x7C,
    HID_KEYCODE_ACPI_WAKE       = 0x7D,
    HID_KEYCODE_RESERVED_LOW    = 0x7E,
    HID_KEYCODE_RESERVED_HIGH   = 0x7F,
};