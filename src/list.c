#include "../h/list.h"

void list_insert(list_t *head, list_t *new_node)
{
    new_node->prev = head->prev;
    new_node->next = head;

    head->prev->next = new_node;
    head->prev = new_node;
}
