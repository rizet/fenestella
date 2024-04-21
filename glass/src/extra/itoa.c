#include "extra/conv.h"
#include <stdlib.h>
#include <string.h>

#define UINT_MAX_64     0xFFFFFFFFFFFFFFFF
#define MAX_DIGIT_BASE  16
#define MIN_DIGIT_BASE  2

uint8_t BASE_MAX_DIGIT_MAP[MAX_DIGIT_BASE + 1] = {
    0,
    0,
    64,
    41,
    32,
    28,
    25,
    23,
    22,
    21,
    20,
    19,
    18,
    18,
    17,
    17,
    16
};

// Copies into provided str
char* itoa(int64_t value, char* str, uint8_t base) {
    if (base < MIN_DIGIT_BASE || base > MAX_DIGIT_BASE) {
        return NULL;
    }
    uint8_t base_max_digits = BASE_MAX_DIGIT_MAP[base];
    memset(str, 0, base_max_digits+1);

    if (value == 0) {
        str[0] = '0';
        return str;
    }

    uint8_t i = base_max_digits;

    bool negative = (value < 0);
    if (negative) {
        value = -value;
        base_max_digits++;
    }

    int64_t tmp = value;

    for (; i > 0; i--) {
        tmp /= base;
        if (tmp == 0) {
            break;
        }
    }

    uint8_t offset = i;
    tmp = value;
    i = base_max_digits - offset;
    
    if (negative) {
        for (; i >= 1; i--) {
            char character = "0123456789abcdef"[tmp % base];
            tmp /= base;
            str[i] = character;
            if (tmp == 0)
                break;
        }
        str[0] = '-';
    }
    else {
        for (; i >= 0; i--) {
            char character = "0123456789abcdef"[tmp % base];
            tmp /= base;
            str[i] = character;
            if (tmp == 0)
                break;
        }
    }

    return str;
}

// Copies into provided str
char* utoa(uint64_t value, char* str, uint8_t base) {
    if (base < MIN_DIGIT_BASE || base > MAX_DIGIT_BASE) {
        return NULL;
    }
    uint8_t base_max_digits = BASE_MAX_DIGIT_MAP[base];
    memset(str, 0, base_max_digits+1);

    if (value == 0) {
        str[0] = '0';
        return str;
    }

    uint8_t i = base_max_digits;

    uint64_t tmp = value;

    for (; i > 0; i--) {
        tmp /= base;
        if (tmp == 0) {
            break;
        }
    }

    uint8_t offset = i;
    tmp = value;
    i = base_max_digits - offset;
        
    for (; i >= 0; i--) {
        char character = "0123456789abcdef"[tmp % base];
        tmp /= base;
        str[i] = character;
        if (tmp == 0)
            break;
    }

    return str;
}
