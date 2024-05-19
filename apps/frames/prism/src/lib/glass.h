#pragma once
#include <stdint.h>
#include <stdbool.h>

#define FIELD_FREE_RAM          000
#define FIELD_DISPLAY_WIDTH     001
#define FIELD_DISPLAY_HEIGHT    002
#define FIELD_DISPLAY_BPP       003

uint64_t rdinfo(uint64_t field);

#define PMAP_VIRT_DEFAULT       000

void* pmap(void* virt);
void punmap(void* virt);
void* fb_req();
void fb_kill();
uint64_t pid();
void* kb_man(bool taking);
