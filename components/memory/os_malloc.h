
#ifndef _OS_MALLOC_H_
#define _OS_MALLOC_H_

/** 
 * The minimum memory size required to construct 
 * the kernel-recognizable memory data structure for the os_malloc_keep() function. 
 */
#define OS_MALLOC_MIN_KEEP_SIZE   (1024)
typedef unsigned int OS_MALLOC_HANDLE;
void os_memory_init(void);
void* os_malloc(unsigned int _size);
void *os_calloc(unsigned int _nmemb, unsigned int _size);
void os_free(void* _ptr);
void* os_malloc_safe(unsigned int _size);
void *os_calloc_safe(unsigned int _nmemb, unsigned int _size);
void os_free_safe(void* _ptr);

void* os_kmalloc(unsigned int _size);
void *os_kcalloc(unsigned int _nmemb, unsigned int _size);
void os_kfree(void* _ptr);

OS_MALLOC_HANDLE os_malloc_keep(void* _m_head, unsigned int _size);
void* os_malloc_usr(OS_MALLOC_HANDLE _handle, unsigned int _size);
void os_free_usr(OS_MALLOC_HANDLE _handle, void* _ptr);

#endif
