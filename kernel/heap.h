/* kernel/heap.h */
#ifndef LUNA_HEAP_H
#define LUNA_HEAP_H
#include "../include/types.h"
void  heap_init(void);
void* k_malloc(size_t size);
void  k_free(void* ptr);
void* k_realloc(void* ptr, size_t new_size);
/* C++ operator new/delete wired to k_malloc/k_free in cpp_support.c */
#endif
