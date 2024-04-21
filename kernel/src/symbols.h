#pragma once
#include <stdint.h>

typedef struct {
    void* addr;
    char* name;
} symbol_t;

extern const 
__attribute__((weak)) 
symbol_t __symbol_tab[];
