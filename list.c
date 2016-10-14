#include <stdlib.h>
#include "list.h"

typedef struct list {
    struct list *prev = NULL;
    int num;
    struct list *next = NULL;
} list;

list* list_add(list *target, int num) {
    if(target->next == NULL) { // the size of target == 0
        assert(target->prev == NULL);
        target->next = target->prev = target;
        target->num = num;
        return target;
    }
    list *new = malloc(sizeof(list));
    new->num = num;
    new->prev = target;
    new->next = target->next;
    target->next = new;
    return new;
}
void list_erase (list *target) {
    if(target->next == target) { // the size of list == 1
        assert(target->prev == target);
        target->next = target->prev = NULL;
        return;
    }
    target->prev->next = target->next;
    target->next->prev = target->prev;
    free(target);
}
