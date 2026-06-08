/* arch/x86/idt.h */
#ifndef LUNA_IDT_H
#define LUNA_IDT_H

#include "../../include/types.h"

/* CPU register state pushed by ISR stubs — used by fault handlers */
typedef struct PACKED {
    /* Pushed by our stub (pusha order) */
    uint32_t edi, esi, ebp, esp_dummy;
    uint32_t ebx, edx, ecx, eax;
    /* Pushed by stub: interrupt number + error code */
    uint32_t int_no, err_code;
    /* Pushed automatically by CPU on interrupt */
    uint32_t eip, cs, eflags;
    /* Pushed by CPU only on privilege change (Ring 3 → Ring 0) */
    uint32_t user_esp, user_ss;
} CPU_Regs;

void idt_install(void);
void idt_set_gate(uint8_t num, uint32_t handler, uint16_t sel, uint8_t flags);

/* IRQ handler registration — used by drivers */
typedef void (*IRQHandler)(CPU_Regs* regs);
void irq_register(uint8_t irq, IRQHandler handler);

#endif /* LUNA_IDT_H */
