#include "ux/menus/menus.h"
#include "gfx/text/text.h"
#include "kbd/kbd.h"
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

#define MENU_MAIN_WHITE_OVERRIDE TEXT_COLOR_WHITE
#define MENU_MAIN_BLACK_OVERRIDE TEXT_COLOR_BLACK

#define MENU_MAIN_INDENT_SIZE 4     // Indent size in spaces
#define MENU_MAIN_MARGIN_TOP 2      // Margin in rows from the top of the screen
#define MENU_MAIN_MARGIN_BOTTOM 2   // Margin in rows from the bottom of the screen
#define MENU_MAIN_MARGIN_LEFT 4     // Margin in columns from the left of the screen
#define MENU_MAIN_MARGIN_RIGHT 4    // Margin in columns from the right of the screen

static menu_item_t* __selected_option = NULL;

static menu_item_t __menu_main_option_1;
static menu_item_t __menu_main_option_2;
static menu_item_t __menu_main_option_3;
static menu_item_t __menu_main_option_4;

static menu_item_t __menu_main_option_1 = {
    .next = &__menu_main_option_2,   .perform = NULL_MENU_TASK_HANDLER,
    .text = "Option 1",
    .col = 0,   .row = 0,   .color = TEXT_COLOR_RED,
    .up = &__menu_main_option_2,    .down = &__menu_main_option_2,   .left = NULL,    .right = &__menu_main_option_3
};

static menu_item_t __menu_main_option_2 = {
    .next = &__menu_main_option_3,   .perform = NULL_MENU_TASK_HANDLER,
    .text = "Option 2",
    .col = 0,   .row = 6,   .color = TEXT_COLOR_CYAN,
    .up = &__menu_main_option_1,    .down = &__menu_main_option_1,   .left = NULL,    .right = NULL
};

static menu_item_t __menu_main_option_3 = {
    .next = &__menu_main_option_4,   .perform = NULL_MENU_TASK_HANDLER,
    .text = "Option 3",
    .col = MENU_MAIN_INDENT_SIZE,   .row = 2,   .color = TEXT_COLOR_GREEN,
    .up = &__menu_main_option_4,    .down = &__menu_main_option_4,   .left = &__menu_main_option_1,    .right = NULL
};

static menu_item_t __menu_main_option_4 = {
    .next = NULL,   .perform = NULL_MENU_TASK_HANDLER,
    .text = "Option 4",
    .col = MENU_MAIN_INDENT_SIZE,   .row = 4,   .color = TEXT_COLOR_MAGENTA,
    .up = &__menu_main_option_3,    .down = &__menu_main_option_3,   .left = &__menu_main_option_1,    .right = NULL
};

#define INVERT_COLOR_BG(color) (color ^ 0x80)
static void _menu_main_highlight_option(const menu_item_t* option) {
    // Highlight the option
    menu_render_string_at(MENU_MAIN_MARGIN_LEFT + option->col, MENU_MAIN_MARGIN_TOP + option->row, option->text, INVERT_COLOR_BG(option->color));
}

static void _menu_main_unhighlight_option(const menu_item_t* option) {
    // Unhighlight the option
    menu_render_string_at(MENU_MAIN_MARGIN_LEFT + option->col, MENU_MAIN_MARGIN_TOP + option->row, option->text, option->color);
}

static void _menu_main_select_option(const menu_item_t* option) {
    switch ((uint64_t)option) {
        case 0:
            return;
        default:
            _menu_main_unhighlight_option(__selected_option);
            __selected_option = (menu_item_t*)option;
            _menu_main_highlight_option(__selected_option);
            return;
    }
}

// NULL_MENU_TASK_HANDLER null op handler for menu items 
void __menu_option_do_nothing(void) {
    return;
}

static bool _menu_drawn = false;
static void _menu_main_draw() {
    // Draw the menu
    _menu_drawn = false;
    text_render_clear();
    
    for (menu_item_t* item = &__menu_main_option_1; item != NULL; item = (menu_item_t *)item->next) {
        menu_render_string_at(MENU_MAIN_MARGIN_LEFT + item->col, MENU_MAIN_MARGIN_TOP + item->row, item->text, item->color);
    }
    _menu_drawn = true;
    _menu_main_select_option(__selected_option);
}

static bool _menu_needs_update = true;
static bool _menu_initialized = false;
void _menu_initialize() {
    if (_menu_initialized) {
        return;
    }
    __selected_option = &__menu_main_option_1;
    _menu_main_select_option(__selected_option);
    _menu_needs_update = true;
    _menu_initialized = true;
}

void menu_main_update() {
    if (!_menu_initialized) {
        _menu_initialize();
    }
    if (!_menu_needs_update) {
        return;
    }
    _menu_main_draw();
    _menu_needs_update = false;
}

void menu_main_handle_key(uint8_t keycode) {
    if (__selected_option == NULL) {
        return;
    }
    switch (keycode) {
        case HID_KEYCODE_UP:
            _menu_main_select_option(__selected_option->up);
            break;
        case HID_KEYCODE_DOWN:
            _menu_main_select_option(__selected_option->down);
            break;
        case HID_KEYCODE_LEFT:
            _menu_main_select_option(__selected_option->left);
            break;
        case HID_KEYCODE_RIGHT:
            _menu_main_select_option(__selected_option->right);
            break;
        case HID_KEYCODE_ENTER:
            __selected_option->perform();
            break;
        default:
            break;
    }
}

tui_menu_handle_t _menu_main = {
    .update = &menu_main_update,
    .handle_key = &menu_main_handle_key,
    .next = NULL
};

tui_menu_handle_t* menus_menu_main = &_menu_main;

void menu_render_string_at(uint64_t col, uint64_t row, const char* str, uint8_t color) {
    text_render_pos_t pos = {
        .p_x = col,
        .p_y = row
    };
    for (uint64_t i = 0; str[i] != '\0'; i++) {
        switch (str[i]) {
            case '\n':
                pos.p_x = col;
                pos.p_y++;
                break;
            case '\t':
                pos.p_x += (MENU_STRING_INDENT_SIZE - (pos.p_x % MENU_STRING_INDENT_SIZE));
                break;
            case '\b':
                pos.p_x--;
                if (pos.p_x < col) {
                    pos.p_x = col;
                }
                break;
            case '\r':
                pos.p_x = col;
                break;
            default:
                text_render_place_at(pos, str[i], color);
                pos.p_x++;
                break;
        }
    }
}
