#include <stdlib.h>
#include <assert.h>
#include "ringlist.h"

ringlist* ringlist_init(void) {
    ringlist *res = (ringlist*)malloc(sizeof(ringlist));
    res->next = res->prev = NULL;
    res->num = 0;
    return res;
}

ringlist* ringlist_add(ringlist *target, int num) {
    if(target->next == NULL) { // the size of target == 0
        assert(target->prev == NULL);
        target->next = target->prev = target;
        target->num = num;
        return target;
    }
    ringlist *newl = (ringlist*)malloc(sizeof(ringlist));
    newl->num = num;
    newl->prev = target;
    newl->next = target->next;
    target->next->prev = newl;
    target->next = newl;
    return newl;
}
ringlist* ringlist_erase (ringlist *target) {
    if(target->next == target) { // the size of list == 1
        assert(target->prev == target);
        target->next = target->prev = NULL;
        return target;
    }
    ringlist *res = target->next;
    target->prev->next = target->next;
    target->next->prev = target->prev;
    free(target);
    return target->next;
}
