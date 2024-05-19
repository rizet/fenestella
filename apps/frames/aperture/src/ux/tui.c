#include "ux/tui.h"
#include "ux/panels/panels.h"
#include "kbd/kbd.h"
#include "gfx/text/text.h"

static bool _tui_on = false;

void _tui_kbd_event(uint8_t keycode) {

}

static tui_panel_handle_t* _selected_panel;

bool tui_graphic_update() {
    if (!_selected_panel)
        return false;
    _selected_panel->invoke();
    return true;
}

void tui_start() {
    if (_tui_on) return;
    
    if (!kbd_init()) return;
    if (!kbd_add_listener(_tui_kbd_event)) return;

    text_render_init(); 
    _selected_panel = main_panel;
    tui_graphic_update();

    _tui_on = true;
}

void tui_cleanup_exit() {
    text_render_stop();
}

bool tui_update() {
    if (!_tui_on) {
        return false;
    }
    return tui_graphic_update();
}
