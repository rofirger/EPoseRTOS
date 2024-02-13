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
#include "stdlib.h"

#define HEAP_MEM_MAGIC (0x6870) // magic number
#define RTOS_HEAP_SIZE (5120)

LIST_HEAD(heap_list_head);

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
unsigned char tmp_mem[RTOS_HEAP_SIZE];
void os_memory_init(void)
{
    //    heap.heap_head = RTOS_HEAP_BEGIN;
    //    heap.heap_size = (unsigned int)((unsigned char*)RTOS_HEAP_END) - (unsigned int)((unsigned char*)RTOS_HEAP_BEGIN);
    // heap.heap_head = malloc(RTOS_HEAP_SIZE);
    heap.heap_head = tmp_mem;
    OS_ASSERT(heap.heap_head != NULL);
    heap.heap_size = RTOS_HEAP_SIZE;
    struct small_memory_header *_m = (struct small_memory_header *)heap.heap_head;
    _m->_magic = HEAP_MEM_MAGIC;
    _m->_size = heap.heap_size;
    _m->_used = false;
    list_add_tail(&heap_list_head, &(_m->_nd));
}

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

/* _size : bytes */
__os_inline void *os_malloc(unsigned int _size)
{
    return __os_malloc_base(&heap_list_head, _size);
}

/*
 * @note: _m_head 4 0.
 * */
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
    //
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
    //
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

void os_free(void *_ptr)
{
    __os_free_base(&heap_list_head, _ptr);
}

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
    unsigned int _mleft = RTOS_HEAP_SIZE - _used_m;
    os_printk(":heap used->%d\r\n", _used_m);
    os_printk(":heap left->%d\r\n", _mleft);
    return _mleft;
}

OS_CMD_EXPORT(free, memory_used, "List the usage of heap.");
