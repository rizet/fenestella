#pragma once
#include <stdint.h>

typedef struct _tui_menu_handle {
    void (*update)();
    void (*handle_key)(uint8_t keycode);
//  void (*handle_click)(uint32_t col, uint32_t row);
    struct _tui_menu_handle*   next;
} tui_menu_handle_t;

extern tui_menu_handle_t* menus_menu_main;

void menu_render_string_at(uint64_t col, uint64_t row, const char* str, uint8_t color);
#define MENU_STRING_INDENT_SIZE 4
