#include "ux/tui.h"
#include "ux/menus/menus.h"
#include "kbd/kbd.h"
#include "gfx/text/text.h"

static bool _tui_on = false;

static tui_menu_handle_t* _selected_menu;

void _tui_kbd_event(uint8_t keycode) {
    switch ((uint64_t)_selected_menu) {
        case 0:
            return;
        default:
            _selected_menu->handle_key(keycode);
            return;
    }
}

bool tui_graphic_update() {
    switch ((uint64_t)_selected_menu) {
        case 0:
            return false;
        default:
            _selected_menu->update();
            return true;
    }
}

void tui_start() {
    if (_tui_on) return;
    
    if (!kbd_init()) return;
    if (!kbd_add_listener(_tui_kbd_event)) return;

    text_render_init(); 
    _selected_menu = menus_menu_main;
    tui_graphic_update();

    _tui_on = true;
}

void tui_cleanup_exit() {
    text_render_stop();
}

bool tui_update() {
    switch (_tui_on) {
        case false:
            return false;
        default:
            break;
    }
    kbd_update();
    return tui_graphic_update();
}
