#include "lib/glass.h"
#include "tty/tty.h"
#include "fash/fash.h"
#include "kbd/kbd.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>

int main(void) {
    tty_start();
    tty_write("skylight v0.4: \"sunrise\"\n\n");
    tty_write("\n[frame] reached process entry.\n");
    tty_write("[frame] acquiring framebuffer lock... done.\n");
    
    if (!fash_started()) {
        if (!kbd_init()) {
            tty_write("[frame] ERROR: failed to initialize keyboard!\n");
        }
        tty_write("[frame] starting fash...\n\n");
        fash_start();
        if (!fash_started()) {
            tty_write("[frame] ERROR: failed to start fash!\n");
        }
    }

    while (true) {
        kbd_update();
        fash_continue();
    }

    tty_kill();
    return -1;
}
