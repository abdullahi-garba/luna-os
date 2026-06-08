/* kernel/vmm.c — Luna OS Virtual Memory Manager
 * Manages the kernel's page directory and provides map_page/unmap_page.
 * Kernel lives in the higher half (virtual 0xC0000000+).
 * Each Ring 3 process gets its own page directory cloned from the kernel's.
 */
#include "vmm.h"
#include "pmm.h"
#include "string.h"
#include "../include/types.h"
#include "../include/panic.h"

#define PAGE_SIZE       4096
#define PAGE_PRESENT    0x001
#define PAGE_WRITABLE   0x002
#define PAGE_USER       0x004   /* set for Ring 3 accessible pages */
#define PAGE_HUGE       0x080   /* 4MB page (PSE) */

#define PD_INDEX(vaddr) ((vaddr) >> 22)
#define PT_INDEX(vaddr) (((vaddr) >> 12) & 0x3FF)
#define PAGE_ALIGN(a)   (((a) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))

/* Kernel page directory — physical address (CR3 holds this) */
static uint32_t* kernel_pd = NULL;

/* ── CR3 / TLB ops ───────────────────────────────────────────────────────── */
static ALWAYS_INLINE void set_cr3(uint32_t pd_phys) {
    __asm__ volatile("mov %0, %%cr3" :: "r"(pd_phys) : "memory");
}
static ALWAYS_INLINE void invlpg(uint32_t vaddr) {
    __asm__ volatile("invlpg (%0)" :: "r"(vaddr) : "memory");
}

/* ── Init ────────────────────────────────────────────────────────────────── */
void vmm_init(void) {
    /* Allocate a page directory for the kernel */
    uint32_t pd_phys = pmm_alloc_frame();
    kernel_pd = (uint32_t*)(pd_phys + 0xC0000000U);  /* virtual address */
    luna_memset(kernel_pd, 0, PAGE_SIZE);

    /* Identity-map and higher-half map the kernel using 4MB PSE pages
     * (same strategy as boot.asm, but now managed here) */
    /* We inherit the boot-time mapping; just record the PD pointer */

    set_cr3(pd_phys);
}

/* ── Map one 4KB page ────────────────────────────────────────────────────── */
void vmm_map_page(uint32_t* pd, uint32_t vaddr, uint32_t paddr, uint32_t flags) {
    uint32_t pdi = PD_INDEX(vaddr);
    uint32_t pti = PT_INDEX(vaddr);

    uint32_t* pt;

    if (!(pd[pdi] & PAGE_PRESENT)) {
        /* Allocate a new page table */
        uint32_t pt_phys = pmm_alloc_frame();
        uint32_t pt_virt = pt_phys + 0xC0000000U;
        luna_memset((void*)pt_virt, 0, PAGE_SIZE);
        pd[pdi] = pt_phys | PAGE_PRESENT | PAGE_WRITABLE | (flags & PAGE_USER);
        pt = (uint32_t*)pt_virt;
    } else {
        uint32_t pt_phys = pd[pdi] & ~0xFFF;
        pt = (uint32_t*)(pt_phys + 0xC0000000U);
    }

    pt[pti] = (paddr & ~0xFFF) | PAGE_PRESENT | flags;
    invlpg(vaddr);
}

/* ── Unmap one page ──────────────────────────────────────────────────────── */
void vmm_unmap_page(uint32_t* pd, uint32_t vaddr) {
    uint32_t pdi = PD_INDEX(vaddr);
    uint32_t pti = PT_INDEX(vaddr);
    if (!(pd[pdi] & PAGE_PRESENT)) return;
    uint32_t* pt = (uint32_t*)((pd[pdi] & ~0xFFF) + 0xC0000000U);
    pt[pti] = 0;
    invlpg(vaddr);
}

/* ── Create a new page directory for a Ring 3 process ───────────────────── */
uint32_t* vmm_create_user_pd(void) {
    uint32_t pd_phys = pmm_alloc_frame();
    uint32_t* pd = (uint32_t*)(pd_phys + 0xC0000000U);
    luna_memset(pd, 0, PAGE_SIZE);

    /* Copy kernel higher-half mappings (PD entries 768–1023) */
    for (int i = 768; i < 1024; i++) {
        pd[i] = kernel_pd[i];
    }
    return pd;
}

/* ── Switch to a process's address space ───────────────────────────────── */
void vmm_switch_pd(uint32_t* pd) {
    uint32_t pd_phys = (uint32_t)pd - 0xC0000000U;
    set_cr3(pd_phys);
}

/* ── Physical address lookup ────────────────────────────────────────────── */
uint32_t vmm_virt_to_phys(uint32_t* pd, uint32_t vaddr) {
    uint32_t pdi = PD_INDEX(vaddr);
    uint32_t pti = PT_INDEX(vaddr);
    if (!(pd[pdi] & PAGE_PRESENT)) return 0;
    uint32_t* pt = (uint32_t*)((pd[pdi] & ~0xFFF) + 0xC0000000U);
    if (!(pt[pti] & PAGE_PRESENT)) return 0;
    return (pt[pti] & ~0xFFF) | (vaddr & 0xFFF);
}

uint32_t* vmm_get_kernel_pd(void) { return kernel_pd; }
