#include "mem/pmm/pmm.h"
#include "boot/protocol.h"
#include <stdint.h>
#include <stddef.h>

uint64_t free_ram() {
    return pmm_get_free_memory();
}

uint64_t display_width() {
    return framebuffer.frame_width;
}

uint64_t display_height() {
    return framebuffer.frame_height;
}

uint64_t display_bpp() {
    return framebuffer.frame_bpp;
}

uint64_t (*retriever[])() = {
    free_ram,
    display_width,
    display_height,
    display_bpp
};

uint64_t rdinfo(uint64_t field) {
    return retriever[field]();
}
