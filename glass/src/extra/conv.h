#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Uses provided buffer, returns buffer, no base prefix, signed
char* itoa(int64_t value, char* str, uint8_t base);
// Uses provided buffer, returns buffer, no base prefix, unsigned
char* utoa(uint64_t value, char* str, uint8_t base);
