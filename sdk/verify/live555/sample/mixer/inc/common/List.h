#ifndef _LIST_H_
#define _LIST_H_


/* Stripped down implementation of linked list taken
 * from the Linux Kernel.
 */

 /*
  * Simple doubly linked list implementation.
  *
  * Some of the internal functions ("__xxx") are useful when
  * manipulating whole lists rather than single entries, as
  * sometimes we already know the next/prev entries and we can
  * generate better code by using them directly rather than
  * using the generic single-entry routines.
  */

#ifdef __cplusplus
extern "C"
{
#endif

    struct list_head {
        struct list_head *next, *prev;
    };

#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define LIST_HEAD(name) \
    struct list_head name = LIST_HEAD_INIT(name)

    static inline void INIT_LIST_HEAD(struct list_head *list)
    {
        list->next = list;
        list->prev = list;
    }

    static inline void __list_add(struct list_head *_new, struct list_head *prev, struct list_head *next)
    {
        next->prev = _new;
        _new->next = next;
        _new->prev = prev;
        prev->next = _new;
    }

    static inline void list_add(struct list_head *_new, struct list_head *head)
    {
        __list_add(_new, head, head->next);
    }

    static inline void list_add_tail(struct list_head *pnew, struct list_head *head)
    {
        __list_add(pnew, head->prev, head);
    }

    static inline int list_empty(const struct list_head *head)
    {
        return head->next == head;
    }

    static inline void __list_del(struct list_head * prev, struct list_head * next)
    {
        next->prev = prev;
        prev->next = next;
    }

    static inline void __list_del_entry(struct list_head *entry)
    {
        __list_del(entry->prev, entry->next);
    }

    static inline void list_del(struct list_head *entry)
    {
        __list_del(entry->prev, entry->next);
        entry->next = NULL;
        entry->prev = NULL;
    }

#define list_entry(ptr, type, member) \
    container_of(ptr, type, member)

#define list_for_each(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)


#define list_for_each_safe(pos, n, head) \
    for (pos = (head)->next, n = pos->next; pos != (head); \
        pos = n, n = pos->next)

#define _offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#if 0
#define container_of(ptr, type, member) ({            \
    const typeof(((type *)0)->member) *__mptr = (ptr);    \
    (type *)( (char *)__mptr - _offsetof(type,member) );})
#else
#define container_of(ptr, type, member) \
    ((type *)((char *)(ptr)-(char *)(&((type *)0)->member)))

#endif

#ifdef __cplusplus
}
#endif

#endif

