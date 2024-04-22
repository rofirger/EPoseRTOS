
#include "../../os_def.h"
void *os_memset(void *dest, int set, unsigned len)
{
    if (NULL == dest || len < 0)
    {
        return NULL;
    }
    char *pdest = (char *)dest;
    while (len-- > 0)
    {
        *pdest++ = set;
    }
    return dest;
}

unsigned int os_strlen(const char* str)
{
    const char* end = str;
    while (*end++);
    return end - str - 1;
}

void* os_memcpy(void* dst, const void* src, unsigned len)
{
    if (NULL == dst || NULL == src) {
        return NULL;
    }

    void* ret = dst;

    if (dst <= src || (char*)dst >= (char*)src + len) {
        // There is no memory overlap. Copy from a low address
        while (len--) {
            *(char*)dst = *(char*)src;
            dst = (char*)dst + 1;
            src = (char*)src + 1;
        }
    }
    else {
        // There is memory overlap. Copy from a high address
        src = (char*)src + len - 1;
        dst = (char*)dst + len - 1;
        while (len--) {
            *(char*)dst = *(char*)src;
            dst = (char*)dst - 1;
            src = (char*)src - 1;
        }
    }
    return ret;
}
