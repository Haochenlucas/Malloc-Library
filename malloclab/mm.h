#include <stdio.h>

extern int mm_init (void);
extern void *mm_malloc (size_t size);
extern void *extend(size_t new_size);
extern void set_allocated(void *bp, size_t size);
extern void replace_list_node(void *p, size_t distance);
extern void remove_list_node(void *p);
extern void mm_free (void *ptr);
extern void *coalesce(void *bp);
