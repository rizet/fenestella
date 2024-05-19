#include "ux/panels/panels.h"
#include <stddef.h>

void panel_main_invoke() {
    
}

tui_panel_handle_t panel_main = {
    &panel_main_invoke,
    NULL
};

tui_panel_handle_t* main_panel = &panel_main;
