#pragma once
#include <stdint.h>
#include <stdbool.h>

void tui_start();
void tui_cleanup_exit();
// Returns false when TUI is done
bool tui_update();
