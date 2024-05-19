#include "ux/tui.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>

int main(void) {
    tui_start();

    // All user interaction logic is dealt with inside this loop
    while (tui_update());

    // Program will exit with error if the loop breaks
    tui_cleanup_exit();
    return -1;
}
