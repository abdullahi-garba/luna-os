/* arch/x86/paging.h — Low-level x86 MMU setup */
#ifndef LUNA_PAGING_H
#define LUNA_PAGING_H
#include "../../include/types.h"

void paging_init(void);
void paging_enable(uint32_t cr3_paddr);

#endif /* LUNA_PAGING_H */