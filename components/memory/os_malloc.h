
#ifndef _OS_MALLOC_H_
#define _OS_MALLOC_H_

/* 交给系统分配的最小内存大小 */
#define OS_MALLOC_MIN_KEEP_SIZE   (1024)
typedef unsigned int OS_MALLOC_HANDLE;
void os_memory_init(void);
void* os_malloc(unsigned int _size);
void os_free(void* _ptr);

OS_MALLOC_HANDLE os_malloc_keep(void* _m_head, unsigned int _size);
void* os_malloc_usr(OS_MALLOC_HANDLE _handle, unsigned int _size);
void os_free_usr(OS_MALLOC_HANDLE _handle, void* _ptr);

#endif
