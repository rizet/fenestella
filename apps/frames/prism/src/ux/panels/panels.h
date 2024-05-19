#pragma once
#include <stdint.h>

typedef struct _tui_panel_handle {
    void (*invoke)();
    struct _tui_panel_handle*   next;
} tui_panel_handle_t;

extern tui_panel_handle_t* main_panel;
