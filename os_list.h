/***********************
 * @file: os_list.h
 * @author: Feijie Luo
 * @Change Logs:
 * Date           Author       Notes
 * 2022-09-10     Feijie Luo   First version
 * @note:
 ***********************/

#ifndef _OS_LIST_H_
#define _OS_LIST_H_

typedef unsigned int size_t;

#define os_offsetof(TYPE, MEMBER) ((size_t) (&((TYPE *)0)->MEMBER))

#define os_container_of(ptr, type, member) \
	((type *)(((unsigned char *)(ptr)) - os_offsetof(type, member)))

#define list_for_each(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)

#define list_for_each_safe(pos, n, head) \
    for (pos = (head)->next, n = pos->next; pos != (head); \
        pos = n, n = pos->next)

#define os_list_entry(ptr, type, member) \
    os_container_of(ptr, type, member)
	
#define os_list_first_entry(ptr, type, member) \
	os_container_of((ptr)->next, type, member)
	
struct list_head {
    struct list_head *next, *prev;
};

#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define LIST_HEAD(name) \
    struct list_head name = LIST_HEAD_INIT(name)

static inline void list_head_init(struct list_head *list)
{
    list->next = list;
    list->prev = list;
}

static inline void __list_add(struct list_head *new,
                  struct list_head *prev,
                  struct list_head *next)
{
    next->prev = new;
    new->next = next;
    new->prev = prev;
    prev->next = new;
}

static inline void list_add(struct list_head *head, struct list_head *new)
{
    __list_add(new, head, head->next);
}

static inline void list_add_tail(struct list_head *head, struct list_head *new)
{
    __list_add(new, head->prev, head);
}

static inline void __list_del(struct list_head * prev, struct list_head * next)
{
    next->prev = prev;
    prev->next = next;
}

static inline void list_del(struct list_head *entry)
{
    __list_del(entry->prev, entry->next);
}

static inline void __list_del_entry(struct list_head *entry)
{
    __list_del(entry->prev, entry->next);
}

static inline void list_del_init(struct list_head *entry)
{
    __list_del_entry(entry);
    list_head_init(entry);
}

static inline void list_replace(struct list_head *old,
                struct list_head *new)
{
    new->next = old->next;
    new->next->prev = new;
    new->prev = old->prev;
    new->prev->next = new;
}

static inline int list_empty(const struct list_head *head)
{
    return head->next == head;
}



#endif
