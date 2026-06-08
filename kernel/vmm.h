/* kernel/vmm.h */
#ifndef LUNA_VMM_H
#define LUNA_VMM_H
#include "../include/types.h"

#define VMM_FLAG_PRESENT  0x001
#define VMM_FLAG_WRITE    0x002
#define VMM_FLAG_USER     0x004   /* Ring 3 accessible */

void      vmm_init(void);
void      vmm_map_page(uint32_t* pd, uint32_t vaddr, uint32_t paddr, uint32_t flags);
void      vmm_unmap_page(uint32_t* pd, uint32_t vaddr);
uint32_t* vmm_create_user_pd(void);
void      vmm_switch_pd(uint32_t* pd);
uint32_t  vmm_virt_to_phys(uint32_t* pd, uint32_t vaddr);
uint32_t* vmm_get_kernel_pd(void);
#endif
