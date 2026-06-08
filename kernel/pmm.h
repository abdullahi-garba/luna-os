/* kernel/pmm.h */
#ifndef LUNA_PMM_H
#define LUNA_PMM_H
#include "../include/types.h"
#include "multiboot2.h"
void     pmm_init(MB2_MmapEntry* entries, uint32_t count);
uint32_t pmm_alloc_frame(void);
uint32_t pmm_alloc_frames(uint32_t n);
void     pmm_free_frame(uint32_t phys_addr);
uint32_t pmm_free_frames(void);
uint32_t pmm_total_frames(void);
uint32_t pmm_free_kb(void);
#endif
