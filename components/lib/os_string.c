
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
