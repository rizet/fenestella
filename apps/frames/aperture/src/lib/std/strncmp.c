#include <string.h>
#include <stddef.h>

int strncmp(const char* __s1, const char* __s2, size_t __n) {
    for (size_t i = 0; i < __n; __s1++, i++) {
		if (*__s1 != *__s2)
			return *__s1 - *__s2;
		__s2++;
	}
	return 0;
}
