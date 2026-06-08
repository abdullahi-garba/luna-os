/* arch/x86/idt.c — Luna OS Interrupt Descriptor Table */

#include "idt.h"
#include "pic.h"
#include "../../include/types.h"
#include "../../include/panic.h"
#include "../../kernel/string.h"
#include "../../kernel/syscall.h"

/* ── IDT Entry (8 bytes) ─────────────────────────────────────────────────── */
typedef struct PACKED {
    uint16_t offset_low;   /* bits 0-15 of handler address */
    uint16_t selector;     /* code segment selector */
    uint8_t  zero;         /* always 0 */
    uint8_t  type_attr;    /* gate type + DPL + present bit */
    uint16_t offset_high;  /* bits 16-31 of handler address */
} IDT_Entry;

typedef struct PACKED {
    uint16_t limit;
    uint32_t base;
} IDT_Ptr;

/* ── Gate type flags ──────────────────────────────────────────────────────── */
#define IDT_PRESENT    0x80
#define IDT_DPL0       0x00
#define IDT_DPL3       0x60   /* accessible from Ring 3 (for int 0x80) */
#define IDT_GATE_INT32 0x0E   /* 32-bit interrupt gate (clears IF) */
#define IDT_GATE_TRAP  0x0F   /* 32-bit trap gate (preserves IF) */

static IDT_Entry idt[256];
static IDT_Ptr   idt_ptr;

/* IRQ handler table (16 hardware IRQs: IRQ0-IRQ15) */
static IRQHandler irq_handlers[16];

/* ── External ISR stubs (from interrupts.asm) ─────────────────────────────── */
/* CPU exceptions 0–31 */
extern void isr0(void);  extern void isr1(void);  extern void isr2(void);
extern void isr3(void);  extern void isr4(void);  extern void isr5(void);
extern void isr6(void);  extern void isr7(void);  extern void isr8(void);
extern void isr9(void);  extern void isr10(void); extern void isr11(void);
extern void isr12(void); extern void isr13(void); extern void isr14(void);
extern void isr15(void); extern void isr16(void); extern void isr17(void);
extern void isr18(void); extern void isr19(void); extern void isr20(void);
extern void isr21(void); extern void isr22(void); extern void isr23(void);
extern void isr24(void); extern void isr25(void); extern void isr26(void);
extern void isr27(void); extern void isr28(void); extern void isr29(void);
extern void isr30(void); extern void isr31(void);

/* Hardware IRQs 0–15 → gates 32–47 */
extern void irq0(void);  extern void irq1(void);  extern void irq2(void);
extern void irq3(void);  extern void irq4(void);  extern void irq5(void);
extern void irq6(void);  extern void irq7(void);  extern void irq8(void);
extern void irq9(void);  extern void irq10(void); extern void irq11(void);
extern void irq12(void); extern void irq13(void); extern void irq14(void);
extern void irq15(void);

/* Syscall gate (int 0x80) */
extern void isr128(void);

/* External lidt */
extern void idt_flush(uint32_t idt_ptr_addr);

/* ── Set a single gate ────────────────────────────────────────────────────── */
void idt_set_gate(uint8_t num, uint32_t handler, uint16_t sel, uint8_t flags) {
    idt[num].offset_low  = (uint16_t)(handler & 0xFFFF);
    idt[num].offset_high = (uint16_t)((handler >> 16) & 0xFFFF);
    idt[num].selector    = sel;
    idt[num].zero        = 0;
    idt[num].type_attr   = flags;
}

/* ── Install ──────────────────────────────────────────────────────────────── */
void idt_install(void) {
    luna_memset(idt, 0, sizeof(idt));
    luna_memset(irq_handlers, 0, sizeof(irq_handlers));

    /* CPU exceptions */
    idt_set_gate( 0, (uint32_t)isr0,  0x08, IDT_PRESENT|IDT_DPL0|IDT_GATE_INT32);
    idt_set_gate( 1, (uint32_t)isr1,  0x08, IDT_PRESENT|IDT_DPL0|IDT_GATE_INT32);
    idt_set_gate( 2, (uint32_t)isr2,  0x08, IDT_PRESENT|IDT_DPL0|IDT_GATE_INT32);
    idt_set_gate( 3, (uint32_t)isr3,  0x08, IDT_PRESENT|IDT_DPL0|IDT_GATE_TRAP); /* BP */
    idt_set_gate( 4, (uint32_t)isr4,  0x08, IDT_PRESENT|IDT_DPL0|IDT_GATE_INT32);
    idt_set_gate( 5, (uint32_t)isr5,  0x08, IDT_PRESENT|IDT_DPL0|IDT_GATE_INT32);
    idt_set_gate( 6, (uint32_t)isr6,  0x08, IDT_PRESENT|IDT_DPL0|IDT_GATE_INT32);
    idt_set_gate( 7, (uint32_t)isr7,  0x08, IDT_PRESENT|IDT_DPL0|IDT_GATE_INT32);
    idt_set_gate( 8, (uint32_t)isr8,  0x08, IDT_PRESENT|IDT_DPL0|IDT_GATE_INT32); /* DF */
    idt_set_gate( 9, (uint32_t)isr9,  0x08, IDT_PRESENT|IDT_DPL0|IDT_GATE_INT32);
    idt_set_gate(10, (uint32_t)isr10, 0x08, IDT_PRESENT|IDT_DPL0|IDT_GATE_INT32);
    idt_set_gate(11, (uint32_t)isr11, 0x08, IDT_PRESENT|IDT_DPL0|IDT_GATE_INT32);
    idt_set_gate(12, (uint32_t)isr12, 0x08, IDT_PRESENT|IDT_DPL0|IDT_GATE_INT32);
    idt_set_gate(13, (uint32_t)isr13, 0x08, IDT_PRESENT|IDT_DPL0|IDT_GATE_INT32); /* GPF */
    idt_set_gate(14, (uint32_t)isr14, 0x08, IDT_PRESENT|IDT_DPL0|IDT_GATE_INT32); /* PF */
    idt_set_gate(15, (uint32_t)isr15, 0x08, IDT_PRESENT|IDT_DPL0|IDT_GATE_INT32);
    idt_set_gate(16, (uint32_t)isr16, 0x08, IDT_PRESENT|IDT_DPL0|IDT_GATE_INT32);
    idt_set_gate(17, (uint32_t)isr17, 0x08, IDT_PRESENT|IDT_DPL0|IDT_GATE_INT32);
    idt_set_gate(18, (uint32_t)isr18, 0x08, IDT_PRESENT|IDT_DPL0|IDT_GATE_INT32);
    idt_set_gate(19, (uint32_t)isr19, 0x08, IDT_PRESENT|IDT_DPL0|IDT_GATE_INT32);
    idt_set_gate(20, (uint32_t)isr20, 0x08, IDT_PRESENT|IDT_DPL0|IDT_GATE_INT32);
    idt_set_gate(21, (uint32_t)isr21, 0x08, IDT_PRESENT|IDT_DPL0|IDT_GATE_INT32);
    idt_set_gate(22, (uint32_t)isr22, 0x08, IDT_PRESENT|IDT_DPL0|IDT_GATE_INT32);
    idt_set_gate(23, (uint32_t)isr23, 0x08, IDT_PRESENT|IDT_DPL0|IDT_GATE_INT32);
    idt_set_gate(24, (uint32_t)isr24, 0x08, IDT_PRESENT|IDT_DPL0|IDT_GATE_INT32);
    idt_set_gate(25, (uint32_t)isr25, 0x08, IDT_PRESENT|IDT_DPL0|IDT_GATE_INT32);
    idt_set_gate(26, (uint32_t)isr26, 0x08, IDT_PRESENT|IDT_DPL0|IDT_GATE_INT32);
    idt_set_gate(27, (uint32_t)isr27, 0x08, IDT_PRESENT|IDT_DPL0|IDT_GATE_INT32);
    idt_set_gate(28, (uint32_t)isr28, 0x08, IDT_PRESENT|IDT_DPL0|IDT_GATE_INT32);
    idt_set_gate(29, (uint32_t)isr29, 0x08, IDT_PRESENT|IDT_DPL0|IDT_GATE_INT32);
    idt_set_gate(30, (uint32_t)isr30, 0x08, IDT_PRESENT|IDT_DPL0|IDT_GATE_INT32);
    idt_set_gate(31, (uint32_t)isr31, 0x08, IDT_PRESENT|IDT_DPL0|IDT_GATE_INT32);

    /* Hardware IRQs 0-15 → gates 32-47 */
    idt_set_gate(32, (uint32_t)irq0,  0x08, IDT_PRESENT|IDT_DPL0|IDT_GATE_INT32);
    idt_set_gate(33, (uint32_t)irq1,  0x08, IDT_PRESENT|IDT_DPL0|IDT_GATE_INT32);
    idt_set_gate(34, (uint32_t)irq2,  0x08, IDT_PRESENT|IDT_DPL0|IDT_GATE_INT32);
    idt_set_gate(35, (uint32_t)irq3,  0x08, IDT_PRESENT|IDT_DPL0|IDT_GATE_INT32);
    idt_set_gate(36, (uint32_t)irq4,  0x08, IDT_PRESENT|IDT_DPL0|IDT_GATE_INT32);
    idt_set_gate(37, (uint32_t)irq5,  0x08, IDT_PRESENT|IDT_DPL0|IDT_GATE_INT32);
    idt_set_gate(38, (uint32_t)irq6,  0x08, IDT_PRESENT|IDT_DPL0|IDT_GATE_INT32);
    idt_set_gate(39, (uint32_t)irq7,  0x08, IDT_PRESENT|IDT_DPL0|IDT_GATE_INT32);
    idt_set_gate(40, (uint32_t)irq8,  0x08, IDT_PRESENT|IDT_DPL0|IDT_GATE_INT32);
    idt_set_gate(41, (uint32_t)irq9,  0x08, IDT_PRESENT|IDT_DPL0|IDT_GATE_INT32);
    idt_set_gate(42, (uint32_t)irq10, 0x08, IDT_PRESENT|IDT_DPL0|IDT_GATE_INT32);
    idt_set_gate(43, (uint32_t)irq11, 0x08, IDT_PRESENT|IDT_DPL0|IDT_GATE_INT32);
    idt_set_gate(44, (uint32_t)irq12, 0x08, IDT_PRESENT|IDT_DPL0|IDT_GATE_INT32);
    idt_set_gate(45, (uint32_t)irq13, 0x08, IDT_PRESENT|IDT_DPL0|IDT_GATE_INT32);
    idt_set_gate(46, (uint32_t)irq14, 0x08, IDT_PRESENT|IDT_DPL0|IDT_GATE_INT32);
    idt_set_gate(47, (uint32_t)irq15, 0x08, IDT_PRESENT|IDT_DPL0|IDT_GATE_INT32);

    /* Syscall gate (int 0x80) — DPL=3 so Ring 3 code can invoke it */
    idt_set_gate(0x80, (uint32_t)isr128, 0x08, IDT_PRESENT|IDT_DPL3|IDT_GATE_INT32);

    idt_ptr.limit = sizeof(idt) - 1;
    idt_ptr.base  = (uint32_t)&idt;

    idt_flush((uint32_t)&idt_ptr);
}

/* ── IRQ handler registration ─────────────────────────────────────────────── */
void irq_register(uint8_t irq, IRQHandler handler) {
    if (irq < 16) irq_handlers[irq] = handler;
}

/* ── C-level exception handler ────────────────────────────────────────────── */
void isr_handler(CPU_Regs* regs) {
    static const char* exceptions[] = {
        "Division by Zero",    "Debug",             "NMI",
        "Breakpoint",          "Overflow",           "Bound Range",
        "Invalid Opcode",      "Device Not Avail",   "Double Fault",
        "Coprocessor Overrun", "Invalid TSS",        "Segment Not Present",
        "Stack Fault",         "General Protection", "Page Fault",
        "Reserved",            "x87 Float",          "Alignment Check",
        "Machine Check",       "SIMD Float",         "Virtualization",
    };

    if (regs->int_no == 14) {
        /* Page Fault — read CR2 for faulting address */
        uint32_t cr2;
        __asm__ volatile("mov %%cr2, %0" : "=r"(cr2));
        /* Check if this is a Ring 3 process fault — if so, kill process */
        if (regs->cs == 0x1B) {
            /* TODO: process_kill(current_pid) */
            return;
        }
        /* Kernel page fault = panic */
        char msg[128];
        luna_snprintf(msg, sizeof(msg),
            "Page Fault at EIP=0x%08x, CR2=0x%08x, ERR=0x%x",
            regs->eip, cr2, regs->err_code);
        kernel_panic(msg);
    }

    if (regs->int_no < 21) {
        kernel_panic(exceptions[regs->int_no]);
    }
}

/* ── C-level IRQ handler ─────────────────────────────────────────────────── */
void irq_handler(CPU_Regs* regs) {
    uint8_t irq = (uint8_t)(regs->int_no - 32);

    /* Call registered handler if any */
    if (irq < 16 && irq_handlers[irq]) {
        irq_handlers[irq](regs);
    }

    /* Send EOI (End of Interrupt) to PIC */
    if (irq >= 8) {
        /* Slave PIC */
        __asm__ volatile("outb %0, %1" :: "a"((uint8_t)0x20), "Nd"((uint16_t)0xA0));
    }
    /* Master PIC always gets EOI */
    __asm__ volatile("outb %0, %1" :: "a"((uint8_t)0x20), "Nd"((uint16_t)0x20));
}
