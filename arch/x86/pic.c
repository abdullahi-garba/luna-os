/* arch/x86/pic.c — 8259A Programmable Interrupt Controller */
#include "pic.h"
#include "../../include/types.h"

#define PIC1_CMD  0x20
#define PIC1_DATA 0x21
#define PIC2_CMD  0xA0
#define PIC2_DATA 0xA1
#define PIC_EOI   0x20
#define ICW1_ICW4 0x01
#define ICW1_INIT 0x10
#define ICW4_8086 0x01

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" :: "a"(val), "Nd"(port));
}
static inline uint8_t inb(uint16_t port) {
    uint8_t val;
    __asm__ volatile("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}
static inline void io_wait(void) { outb(0x80, 0); }

void pic_remap(uint8_t master_offset, uint8_t slave_offset) {
    uint8_t m = inb(PIC1_DATA); /* save masks */
    uint8_t s = inb(PIC2_DATA);

    outb(PIC1_CMD,  ICW1_INIT | ICW1_ICW4); io_wait();
    outb(PIC2_CMD,  ICW1_INIT | ICW1_ICW4); io_wait();
    outb(PIC1_DATA, master_offset);          io_wait();
    outb(PIC2_DATA, slave_offset);           io_wait();
    outb(PIC1_DATA, 4);  io_wait(); /* tell master: slave at IRQ2 */
    outb(PIC2_DATA, 2);  io_wait(); /* tell slave: cascade identity */
    outb(PIC1_DATA, ICW4_8086); io_wait();
    outb(PIC2_DATA, ICW4_8086); io_wait();

    outb(PIC1_DATA, m); /* restore saved masks */
    outb(PIC2_DATA, s);
}

void pic_mask_irq(uint8_t irq) {
    uint16_t port = (irq < 8) ? PIC1_DATA : PIC2_DATA;
    if (irq >= 8) irq -= 8;
    outb(port, inb(port) | (1 << irq));
}

void pic_unmask_irq(uint8_t irq) {
    uint16_t port = (irq < 8) ? PIC1_DATA : PIC2_DATA;
    if (irq >= 8) irq -= 8;
    outb(port, inb(port) & ~(1 << irq));
}

void pic_send_eoi(uint8_t irq) {
    if (irq >= 8) outb(PIC2_CMD, PIC_EOI);
    outb(PIC1_CMD, PIC_EOI);
}
