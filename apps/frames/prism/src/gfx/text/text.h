#pragma once
#include <stdint.h>

#define TEXT_COLOR_BLACK 0
#define TEXT_COLOR_BLUE 1
#define TEXT_COLOR_GREEN 2
#define TEXT_COLOR_CYAN 3
#define TEXT_COLOR_RED 4
#define TEXT_COLOR_MAGENTA 5
#define TEXT_COLOR_BROWN 6
#define TEXT_COLOR_LIGHT_GREY 7
#define TEXT_COLOR_DARK_GREY 8
#define TEXT_COLOR_LIGHT_BLUE 9
#define TEXT_COLOR_LIGHT_GREEN 10
#define TEXT_COLOR_LIGHT_CYAN 11
#define TEXT_COLOR_LIGHT_RED 12
#define TEXT_COLOR_LIGHT_MAGENTA 13
#define TEXT_COLOR_LIGHT_BROWN 14
#define TEXT_COLOR_WHITE 15

typedef struct {
    uint64_t p_x;
    uint64_t p_y;
} text_render_pos_t;

typedef struct {
    uint64_t col;
    uint64_t row;
} text_render_pos_align_t;

void text_render_init();
void text_render_stop();

void text_render_clear();

uint64_t text_render_get_width();
uint64_t text_render_get_height();
uint64_t text_render_get_cols();
uint64_t text_render_get_rows();

void text_render_place_at(text_render_pos_t pos, char c, uint8_t color);
void text_render_clean_at(text_render_pos_t pos);

static inline text_render_pos_t text_render_pos_convert(text_render_pos_align_t aligned) {
    text_render_pos_t ret = {
        aligned.col * (text_render_get_width() / text_render_get_cols()),
        aligned.row * (text_render_get_height() / text_render_get_rows())
    };
    return ret;
}
