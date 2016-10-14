typedef struct list {
    struct list *prev = NULL;
    int num;
    struct list *next = NULL;
} list;

list* list_add(list *target, int num);
list_erase (list *target);
