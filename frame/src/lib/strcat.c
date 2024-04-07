#include <stdint.h>
#include <string.h>

char* strcat(char* dest, const char* src) {
    char* ret = dest;
    while (*dest != '\0') {
        dest++;
    }
    while (*src != '\0') {
        *dest = *src;
        dest++;
        src++;
    }
    *dest = '\0';
    return ret;
}
