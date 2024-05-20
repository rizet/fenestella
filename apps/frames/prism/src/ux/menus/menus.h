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
void __menu_option_do_nothing(void);
#define NULL_MENU_TASK_HANDLER (const void(*)(void))&__menu_option_do_nothing

typedef struct s_menu_item {
    const struct s_menu_item* next;
    const void (*perform)(void);
    const char* text;
    const uint64_t col;
    const uint64_t row;
    const uint8_t color;
    const struct s_menu_item* up;       // This is used to navigate the menu, null if not present
    const struct s_menu_item* down;     // This is used to navigate the menu, null if not present
    const struct s_menu_item* left;     // This is used to navigate the menu, null if not present
    const struct s_menu_item* right;    // This is used to navigate the menu, null if not present
} menu_item_t;
