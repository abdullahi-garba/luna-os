/* arch/x86/gdt.h */
#ifndef LUNA_GDT_H
#define LUNA_GDT_H

#include "../../include/types.h"

/* Segment selector values (index << 3 | RPL) */
#define GDT_NULL_SEG      0x00   /* index 0 */
#define GDT_KERNEL_CODE   0x08   /* index 1, RPL=0 */
#define GDT_KERNEL_DATA   0x10   /* index 2, RPL=0 */
#define GDT_USER_CODE     0x1B   /* index 3, RPL=3 */
#define GDT_USER_DATA     0x23   /* index 4, RPL=3 */
#define GDT_TSS_SEG       0x28   /* index 5 */

void gdt_install(void);
void gdt_set_tss_stack(uint32_t esp0);  /* called on every Ring 3 → Ring 0 switch */

#endif /* LUNA_GDT_H */
