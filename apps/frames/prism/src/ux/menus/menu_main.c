#include "ux/menus/menus.h"
#include "gfx/text/text.h"
#include "kbd/kbd.h"
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

#define MENU_MAIN_WHITE_OVERRIDE TEXT_COLOR_WHITE
#define MENU_MAIN_BLACK_OVERRIDE TEXT_COLOR_BLACK

struct _menu_main_option {
    const char* text;
    const struct _menu_main_option* above;    // Up key
    const struct _menu_main_option* below;    // Down key
    const struct _menu_main_option* left;     // Left key
    const struct _menu_main_option* right;    // Right key
    const struct _menu_main_option* parent;   // Backspace key
    const struct _menu_main_option* child;    // Enter key
    const void (*invoke_entry)();             // Space key
    uint64_t last_draw;             // Last time the option was drawn (iterator)
    uint64_t p_col;
    uint64_t p_row;
    uint8_t color;
};

static struct _menu_main_option _reboot_option;
static struct _menu_main_option _nop_option;

static struct _menu_main_option* __topleft_option = &_nop_option;
static struct _menu_main_option* __current_option = NULL;

static struct _menu_main_option _nop_option = {
    .text = "Do nothing",
    .above = NULL,  // No wraparound
    .below = &_reboot_option,   // Go down
    .left = NULL,
    .right = NULL,
    .parent = NULL,
    .child = NULL,
    .invoke_entry = NULL,
    .last_draw = 0,
    .p_col = 0,     // Do not set them by default, the drawing algorithm will do that
    .p_row = 0,     // Do not set them by default, the drawing algorithm will do that
    .color = TEXT_COLOR_COMBO(MENU_MAIN_WHITE_OVERRIDE, MENU_MAIN_BLACK_OVERRIDE)
};

static struct _menu_main_option _reboot_option = {
    .text = "Reboot violently",
    .above = &_nop_option,   // Go up
    .below = NULL,
    .left = NULL,
    .right = NULL,
    .parent = NULL,
    .child = NULL,
    .invoke_entry = NULL,
    .last_draw = 0,
    .p_col = 0,
    .p_row = 0,
    .color = TEXT_COLOR_COMBO(TEXT_COLOR_RED, MENU_MAIN_BLACK_OVERRIDE)
};


#define INVERT_COLOR_BG(color) (color ^ 0x80)
static void _menu_main_highlight_option(const struct _menu_main_option* option) {
    // Highlight the option
    menu_render_string_at(option->p_col, option->p_row, option->text, INVERT_COLOR_BG(option->color));
}

static void _menu_main_unhighlight_option(const struct _menu_main_option* option) {
    // Unhighlight the option
    menu_render_string_at(option->p_col, option->p_row, option->text, option->color);
}

static void _menu_main_select_option(const struct _menu_main_option* option) {
    if (option == NULL) {
        return;
    }
    // Select the option
    _menu_main_unhighlight_option(__current_option);
    __current_option = (struct _menu_main_option *)option;
    _menu_main_highlight_option(__current_option);
}

#define MENU_MAIN_INDENT_SIZE 4     // Indent size in spaces
#define MENU_MAIN_DEFAULT_INDENT 0  // No indent by default
#define MENU_MAIN_MARGIN_TOP 2      // Margin in rows from the top of the screen
#define MENU_MAIN_MARGIN_BOTTOM 2   // Margin in rows from the bottom of the screen
#define MENU_MAIN_MARGIN_LEFT 4     // Margin in columns from the left of the screen
#define MENU_MAIN_MARGIN_RIGHT 4    // Margin in columns from the right of the screen
#define MENU_MAIN_MARGIN_OPTION 1   // Margin in rows between options
#define MENU_MAIN_ROW_INCREMENT MENU_MAIN_MARGIN_OPTION+1   // Row increment for the drawing algorithm

static bool _menu_drawn = false;
static uint64_t _menu_last_draw = 0;
static void _menu_main_draw() {
    // Draw the menu
    text_render_clear();
    struct _menu_main_option* option_iterator = __topleft_option;
    uint64_t row = MENU_MAIN_MARGIN_TOP, col = MENU_MAIN_MARGIN_LEFT + (MENU_MAIN_INDENT_SIZE * MENU_MAIN_DEFAULT_INDENT);
    bool drawing = true;
    _menu_last_draw++;
    // Extremely directional algorithm to draw the menu
    // Goes into, right, and down, in that priority
    // Goes left, and out, in that priority
    while (drawing) {
        if (option_iterator->last_draw >= _menu_last_draw) {
            drawing = false;
            break;
        }

        size_t text_length = strlen(option_iterator->text);
        menu_render_string_at(col, row, option_iterator->text, option_iterator->color);
        
        option_iterator->p_col = col;
        option_iterator->p_row = row;
        option_iterator->last_draw = _menu_last_draw;

        if (option_iterator->child != NULL) {
            col += MENU_MAIN_INDENT_SIZE;
            option_iterator = (struct _menu_main_option *)option_iterator->child;
        } else if (option_iterator->right != NULL) {
            col += text_length;
            col += (MENU_MAIN_INDENT_SIZE - (col % MENU_MAIN_INDENT_SIZE));
            option_iterator = (struct _menu_main_option *)option_iterator->right;
        } else if (option_iterator->left != NULL) {
            col -= MENU_MAIN_INDENT_SIZE;
            option_iterator = (struct _menu_main_option *)option_iterator->left;
            while (option_iterator->last_draw >= _menu_last_draw) {
                if (option_iterator->left != NULL) {
                    col -= MENU_MAIN_INDENT_SIZE;
                    option_iterator = (struct _menu_main_option *)option_iterator->left;
                } else if (option_iterator->below != NULL) {
                    row += MENU_MAIN_ROW_INCREMENT;
                    option_iterator = (struct _menu_main_option *)option_iterator->below;
                } else {
                    drawing = false;
                    break;
                }
            }
        } else if (option_iterator->below != NULL) {
            row += MENU_MAIN_ROW_INCREMENT;
            option_iterator = (struct _menu_main_option *)option_iterator->below;
        } else if (option_iterator->parent != NULL) {
            col -= MENU_MAIN_INDENT_SIZE;
            row += MENU_MAIN_ROW_INCREMENT;
            option_iterator = (struct _menu_main_option *)option_iterator->parent;
            while (option_iterator->last_draw >= _menu_last_draw) {
                if (option_iterator->parent != NULL) {
                    col -= MENU_MAIN_INDENT_SIZE;
                    option_iterator = (struct _menu_main_option *)option_iterator->parent;
                } else {
                    drawing = false;
                    break;
                }
            }
        } else {
            drawing = false;
        }
    }
    _menu_drawn = true;
    _menu_main_select_option(__current_option);
}

static bool _menu_needs_update = true;
static bool _menu_initialized = false;
void _menu_initialize() {
    if (_menu_initialized) {
        return;
    }
    __topleft_option = &_nop_option;
    __current_option = __topleft_option;
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
    if (__current_option == NULL) {
        return;
    }
    switch (keycode) {
        case HID_KEYCODE_UP:
            _menu_main_select_option(__current_option->above);
            break;
        case HID_KEYCODE_DOWN:
            _menu_main_select_option(__current_option->below);
            break;
        case HID_KEYCODE_LEFT:
            _menu_main_select_option(__current_option->left);
            break;
        case HID_KEYCODE_RIGHT:
            _menu_main_select_option(__current_option->right);
            break;
        case HID_KEYCODE_BACKSPACE:
            _menu_main_select_option(__current_option->parent);
            break;
        case HID_KEYCODE_ENTER:
            _menu_main_select_option(__current_option->child);
            break;
        case HID_KEYCODE_SPACE:
            if (__current_option->invoke_entry != NULL) {
                __current_option->invoke_entry();
            }
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
