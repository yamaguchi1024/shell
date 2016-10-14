typedef struct _ringlist{
    struct _ringlist *prev = NULL;
    int num;
    struct _ringlist *next = NULL;
} ringlist;

ringlist* ringlist_init(void);
ringlist* ringlist_add(ringlist *target, int num);
ringlist* ringlist_erase(ringlist *target);
