/***********************
 * @file: os_malloc.c
 * @author: Feijie Luo
 * @Change Logs:
 * Date           Author       Notes
 * 2023-10-15     Feijie Luo   First version
 * 2023-10-22     Feijie Luo   Fix memory align bug. Fix unsigned int overflow bug.
 * 2023-11-01     Feijie Luo   Add free cmd.
 * 2023-11-02     Feijie Luo   Add NULL judgment in os_free to make os_free safer.
 * @note:
 ***********************/

#include "../../board/libcpu_headfile.h"
#include "../../board/os_board.h"
#include "../../os_config.h"
#include "../../os_def.h"
#include "../../os_list.h"
#include "../../os_service.h"
#include "os_malloc.h"

#define HEAP_MEM_MAGIC (0x6870) // magic number

static LIST_HEAD(heap_list_head);

struct heap_structure {
    void *heap_head;
    unsigned int heap_size;
};

static volatile struct heap_structure heap = {.heap_head = NULL, .heap_size = 0};

struct small_memory_header {
    unsigned short _magic;
    bool _used;
    unsigned int _size; // bytes
    struct list_head _nd;
};

/**
 * Called by os_board_init(), the size of heap must be CONFIG_HEAP_SIZE
 *
 * @param[in/out] ptr      The head of the heap.
 *
 * @return        void
 */
void os_set_sys_heap_head(void* ptr)
{
    heap.heap_head = ptr;
    heap.heap_size = CONFIG_HEAP_SIZE;
}

/**
 * Initialize the memory manage subsystem.
 *
 * In EPoseRTOS, it is called in os_sys_init() function
 */
void os_memory_init(void)
{
    OS_ASSERT(heap.heap_head != NULL);
    struct small_memory_header *_m = (struct small_memory_header *)heap.heap_head;
    _m->_magic = HEAP_MEM_MAGIC;
    _m->_size = heap.heap_size;
    _m->_used = false;
    list_add_tail(&heap_list_head, &(_m->_nd));
}

/**
 * Allocate memory from the list_head(mounted with an unallocated memory).
 *
 * @param[in/out] _heap_list_head The head of the heap list.
 * @param[in]     _size(byte)     The size of the memory block to be allocated.
 *
 * @return                A pointer to the allocated memory block.
 */
__os_static void *__os_malloc_base(struct list_head *_heap_list_head,
                                   unsigned int _size)
{
    // size4
    _size = _size + (4 - (_size % 4));

    struct list_head *_current_node = NULL;
    struct list_head *_next_node = NULL;
    list_for_each_safe(_current_node, _next_node, _heap_list_head)
    {
        struct small_memory_header *smh = os_list_entry(_current_node, struct small_memory_header, _nd);
        if (smh->_magic == HEAP_MEM_MAGIC && smh->_used == false) {
            if (smh->_size >= _size + sizeof(struct small_memory_header)) {
                //
                if (smh->_size <= _size + 2 * sizeof(struct small_memory_header)) {
                    smh->_used = true;
                    return ((void *)((unsigned char *)smh + sizeof(struct small_memory_header)));
                } else {
                    //
                    struct small_memory_header *next_smh = (struct small_memory_header *)((unsigned char *)smh +
                                                                                          sizeof(struct small_memory_header) + _size);
                    next_smh->_magic = HEAP_MEM_MAGIC;
                    next_smh->_used = false;
                    next_smh->_size = smh->_size - (sizeof(struct small_memory_header) + _size);

                    smh->_used = true;
                    smh->_size = sizeof(struct small_memory_header) + _size;

                    list_add(&(smh->_nd), &(next_smh->_nd));

                    return ((void *)((unsigned char *)smh + sizeof(struct small_memory_header)));
                }
            }
        }
    }
    return NULL;
}

/**
 * Allocate memory from the kernel heap. **Non-thread-safe**
 *
 * @param[in] _size The size of the memory block to be allocated.
 *
 * @return A pointer to the allocated memory block.
 */
void *os_malloc(unsigned int _size)
{
    return __os_malloc_base(&heap_list_head, _size);
}

void *os_calloc(unsigned int _nmemb, unsigned int _size)
{
    unsigned int _s = _size * _nmemb;
    void *_mem = __os_malloc_base(&heap_list_head, _s);
    memset(_mem, 0, _s);
    return _mem;
}

/**
 * Construct kernel-recognizable memory data structure from user-provided memory.
 *
 * @param _m_head[in/out] The starting address of the user-provided memory block.
 * @param _size[in]       The size of the memory block to be constructed.
 *
 * @return An OS_MALLOC_HANDLE representing the user-provided memory block.
 */
OS_MALLOC_HANDLE os_malloc_keep(void *_m_head, unsigned int _size)
{
    if (_m_head == NULL ||
        (unsigned int)_m_head % 4 != 0)
        return 0;

    if (_size < OS_MALLOC_MIN_KEEP_SIZE)
        return 0;

    struct list_head *keep_head = (struct list_head *)_m_head;
    list_head_init(keep_head);

    struct small_memory_header *_m =
        (struct small_memory_header *)((unsigned int)_m_head + sizeof(struct list_head));
    _m->_magic = HEAP_MEM_MAGIC;
    _m->_size = _size - sizeof(struct list_head);
    _m->_used = false;
    list_add_tail(keep_head, &(_m->_nd));

    return (OS_MALLOC_HANDLE)keep_head;
}

/**
 * Allocate memory based on the OS_MALLOC_HANDLE. **Non-thread-safe**
 *
 * @param[in] _handle The return value of os_malloc_keep(...).
 * @param[in] _size   The size of the memory block to be allocated.
 *
 * @return A pointer to the allocated memory block.
 */
void *os_malloc_usr(OS_MALLOC_HANDLE _handle, unsigned int _size)
{
    if (_handle == 0)
        return NULL;

    return __os_malloc_base((struct list_head *)_handle, _size);
}

__os_static void __os_free_base(struct list_head *_heap_list_head,
                                void *_ptr)
{
    if (_ptr == NULL)
        return;

    struct small_memory_header *smh = (struct small_memory_header *)((unsigned char *)_ptr -
                                                                     sizeof(struct small_memory_header));
    smh->_used = false;

    struct list_head *_tmp_nd = &(smh->_nd);
    struct list_head *_assist_nd = NULL;

    struct small_memory_header *smh_this = NULL;
    struct small_memory_header *smh_next = NULL;
    
    while (_tmp_nd->prev != _heap_list_head) {
        smh_this = os_list_entry(_tmp_nd, struct small_memory_header, _nd);
        smh_next = os_list_entry(_tmp_nd->prev, struct small_memory_header, _nd);
        _assist_nd = _tmp_nd->prev;
        if (smh_next->_magic == HEAP_MEM_MAGIC && smh_next->_used == false) {
            smh_next->_size += smh_this->_size;
            list_del(_tmp_nd);
        } else {
            break;
        }
        _tmp_nd = _assist_nd;
    }
    
    while (_tmp_nd->next != _heap_list_head) {
        smh_this = os_list_entry(_tmp_nd, struct small_memory_header, _nd);
        smh_next = os_list_entry(_tmp_nd->next, struct small_memory_header, _nd);
        //_assist_nd = _tmp_nd->next;
        if (smh_next->_magic == HEAP_MEM_MAGIC && smh_next->_used == false) {
            smh_this->_size += smh_next->_size;
            list_del(_tmp_nd->next);
        } else {
            break;
        }
        //_tmp_nd = _assist_nd;
    }
}

/**
 * Free memory [MUST BE] allocated by os_malloc. **Non-thread-safe**
 *
 * @param[in/out] _ptr A pointer to the memory block to be freed.
 */
void os_free(void *_ptr)
{
    __os_free_base(&heap_list_head, _ptr);
}

/**
 * Free memory [MUST BE] allocated by os_malloc_usr. **Non-thread-safe**
 *
 * @param[in] _handle The return value of os_malloc_keep(...).
 * @param[in/out] _ptr A pointer to the memory block to be freed.
 */
void os_free_usr(OS_MALLOC_HANDLE _handle, void *_ptr)
{
    __os_free_base((struct list_head *)_handle, _ptr);
}

OS_CMD_PROCESS_FN(memory_used)
{
    unsigned int _used_m = 0;
    struct list_head *_current_pos;
    struct small_memory_header *smh_this = NULL;
    OS_ENTER_CRITICAL;
    list_for_each(_current_pos, &heap_list_head)
    {
        smh_this = os_list_entry(_current_pos, struct small_memory_header, _nd);
        if (smh_this->_used == true)
            _used_m += smh_this->_size;
    }
    OS_EXIT_CRITICAL;
    unsigned int _mleft = CONFIG_HEAP_SIZE - _used_m;
    os_printk(":heap used->%d\r\n", _used_m);
    os_printk(":heap left->%d\r\n", _mleft);
    return _mleft;
}

OS_CMD_EXPORT(free, memory_used, "List the usage of heap.");
