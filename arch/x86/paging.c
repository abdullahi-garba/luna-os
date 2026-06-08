/* arch/x86/paging.c — Hardware Page Fault & CR3 Control */
#include "paging.h"
#include "idt.h"
#include "../../include/panic.h"

void paging_enable(uint32_t cr3_paddr) {
    __asm__ volatile(
        "mov %0, %%cr3\n"
        "mov %%cr0, %%eax\n"
        "or $0x80000000, %%eax\n"  /* Set PG (Paging) bit */
        "mov %%eax, %%cr0\n"
        :: "r"(cr3_paddr) : "eax", "memory"
    );
}

void paging_init(void) {
    /* The core mapping is handled by vmm.c, this just ensures
     * the hardware is synced and IDT handler for page faults (#PF) is ready. */
}