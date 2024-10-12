#ifndef LIST_HEADER
#define LIST_HEADER

#define offsetof(type, member) \
        ((size_t)&(((type *)0)->member))

#define container_of(ptr, type, member) ({ \
        typeof(((type *)0)->member) *__ptr = (ptr); \
        (type *)((char *)__ptr - offsetof(type, member)); })

#define LIST_INIT(head) { &(head), &(head) }

#define list_for_each(pos, head) \
        for (pos = (head)->next; pos != (head); pos = pos->next)

#define list_entry(ptr, type, member) \
        container_of(ptr, type, member)

#define list_for_each_entry(pos, head, member) \
        for (pos = list_entry((head)->next, typeof(*pos), member); \
            &pos->member != (head); \
            pos = list_entry(pos->member.next, typeof(*pos), member)) 

struct __list_t
{
    struct __list_t *next;
    struct __list_t *prev;
};

typedef struct __list_t list_t;

void list_insert(list_t *head, list_t *new_node);

#endif //LIST_HEADER
