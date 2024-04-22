
#ifndef _OS_STRING_H_
#define _OS_STRING_H_

void *os_memset(void *dest, int set, unsigned len);
unsigned int os_strlen(const char* str);
void* os_memcpy(void* dst, const void* src, unsigned len);

#endif /* _OS_STRING_H_ */
