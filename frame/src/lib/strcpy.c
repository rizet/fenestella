#include <string.h>
#include <stdint.h>

char* strcpy(char *dest, const char *src)
{
    char* ret = dest;
    while (*src != '\0')
    {
        *dest = *src;
        dest++;
        src++;
    }
    *dest = '\0';
    return ret;
}
