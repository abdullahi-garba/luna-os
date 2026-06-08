/* arch/x86/gdt.c — Luna OS Global Descriptor Table
 * 6 entries:
 *   0: Null descriptor (required)
 *   1: Ring 0 code (kernel)
 *   2: Ring 0 data (kernel)
 *   3: Ring 3 code (user)
 *   4: Ring 3 data (user)
 *   5: TSS (hardware task state for syscall stack switch)
 */

#include "gdt.h"
#include "../../include/types.h"
#include "../../kernel/string.h"

/* ── GDT Entry format (8 bytes) ──────────────────────────────────────────── */
typedef struct PACKED {
    uint16_t limit_low;       /* bits 0-15 of segment limit */
    uint16_t base_low;        /* bits 0-15 of base address */
    uint8_t  base_middle;     /* bits 16-23 of base address */
    uint8_t  access;          /* access byte (present/DPL/type flags) */
    uint8_t  granularity;     /* flags[7:4] + limit_high[3:0] */
    uint8_t  base_high;       /* bits 24-31 of base address */
} GDT_Entry;

/* ── TSS (Task State Segment) — minimal, only esp0/ss0 matter for us ──────── */
typedef struct PACKED {
    uint32_t prev_tss;
    uint32_t esp0;            /* kernel stack pointer on Ring 0 entry */
    uint32_t ss0;             /* kernel stack segment = GDT_KERNEL_DATA */
    uint32_t esp1, ss1;
    uint32_t esp2, ss2;
    uint32_t cr3;
    uint32_t eip, eflags;
    uint32_t eax, ecx, edx, ebx;
    uint32_t esp, ebp, esi, edi;
    uint32_t es, cs, ss, ds, fs, gs;
    uint32_t ldt;
    uint16_t trap;
    uint16_t iomap_base;
} TSS;

/* ── GDT pointer (used by lgdt instruction) ──────────────────────────────── */
typedef struct PACKED {
    uint16_t limit;
    uint32_t base;
} GDT_Ptr;

/* ── Static tables ───────────────────────────────────────────────────────── */
#define GDT_ENTRIES 6
static GDT_Entry gdt[GDT_ENTRIES];
static GDT_Ptr   gdt_ptr;
static TSS       kernel_tss;

/* ── External ASM flush ──────────────────────────────────────────────────── */
extern void gdt_flush(uint32_t gdt_ptr_addr);
extern void tss_flush(void);

/* ── Access byte bit fields ──────────────────────────────────────────────── */
/* Access byte layout:
 *  7: Present
 *  6-5: DPL (0=kernel, 3=user)
 *  4: Descriptor type (1=code/data, 0=system)
 *  3: Executable (1=code, 0=data)
 *  2: Direction/Conforming
 *  1: Readable/Writable
 *  0: Accessed
 */
#define ACC_PRESENT   0x80
#define ACC_DPL0      0x00
#define ACC_DPL3      0x60
#define ACC_SEG       0x10   /* non-system */
#define ACC_EXEC      0x08   /* code */
#define ACC_RW        0x02   /* readable (code) / writable (data) */
#define ACC_TSS_32    0x09   /* 32-bit TSS, available */

/* Granularity byte: G=1 (4KB pages), DB=1 (32-bit), L=0, AVL=0 */
#define GRAN_4K_32    0xCF   /* 4KB granularity, 32-bit, limit high = 0xF */

static void gdt_set_entry(int idx, uint32_t base, uint32_t limit,
                           uint8_t access, uint8_t gran) {
    gdt[idx].base_low    = (uint16_t)(base & 0xFFFF);
    gdt[idx].base_middle = (uint8_t)((base >> 16) & 0xFF);
    gdt[idx].base_high   = (uint8_t)((base >> 24) & 0xFF);
    gdt[idx].limit_low   = (uint16_t)(limit & 0xFFFF);
    gdt[idx].granularity = (gran & 0xF0) | ((limit >> 16) & 0x0F);
    gdt[idx].access      = access;
}

static void tss_install(int idx) {
    uint32_t base  = (uint32_t)&kernel_tss;
    uint32_t limit = sizeof(TSS) - 1;

    luna_memset(&kernel_tss, 0, sizeof(TSS));
    kernel_tss.ss0       = GDT_KERNEL_DATA;
    kernel_tss.iomap_base = sizeof(TSS); /* no I/O permission bitmap */

    /* TSS descriptor has special format — access = 0x89, gran = 0x00 */
    gdt[idx].limit_low   = (uint16_t)(limit & 0xFFFF);
    gdt[idx].base_low    = (uint16_t)(base & 0xFFFF);
    gdt[idx].base_middle = (uint8_t)((base >> 16) & 0xFF);
    gdt[idx].access      = ACC_PRESENT | ACC_TSS_32;
    gdt[idx].granularity = (uint8_t)((limit >> 16) & 0x0F);
    gdt[idx].base_high   = (uint8_t)((base >> 24) & 0xFF);
}

void gdt_install(void) {
    /* 0: Null */
    gdt_set_entry(0, 0, 0, 0, 0);

    /* 1: Ring 0 Code — full 4GB, DPL=0, executable, readable */
    gdt_set_entry(1, 0, 0xFFFFF,
        ACC_PRESENT | ACC_DPL0 | ACC_SEG | ACC_EXEC | ACC_RW, GRAN_4K_32);

    /* 2: Ring 0 Data — full 4GB, DPL=0, writable */
    gdt_set_entry(2, 0, 0xFFFFF,
        ACC_PRESENT | ACC_DPL0 | ACC_SEG | ACC_RW, GRAN_4K_32);

    /* 3: Ring 3 Code — full 4GB, DPL=3, executable, readable */
    gdt_set_entry(3, 0, 0xFFFFF,
        ACC_PRESENT | ACC_DPL3 | ACC_SEG | ACC_EXEC | ACC_RW, GRAN_4K_32);

    /* 4: Ring 3 Data — full 4GB, DPL=3, writable */
    gdt_set_entry(4, 0, 0xFFFFF,
        ACC_PRESENT | ACC_DPL3 | ACC_SEG | ACC_RW, GRAN_4K_32);

    /* 5: TSS */
    tss_install(5);

    gdt_ptr.limit = sizeof(gdt) - 1;
    gdt_ptr.base  = (uint32_t)&gdt;

    gdt_flush((uint32_t)&gdt_ptr);
    tss_flush();
}

/* Called on every context switch into Ring 0 to set the kernel stack */
void gdt_set_tss_stack(uint32_t esp0) {
    kernel_tss.esp0 = esp0;
}
