#include <string.h>

int memcmp(const void* __s1, const void* __s2, size_t __n) {
    const char* _s1 = __s1;
    const char* _s2 = __s2;
    for (size_t i = 0; i < __n; _s1++, i++) {
		if (*_s1 != *_s2)
			return *_s1 - *_s2;
		_s2++;
	}
	return 0;
}
