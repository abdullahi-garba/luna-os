/* drivers/input/mouse.c — PS/2 Mouse Driver */
#include "../../include/types.h"
#include "../../arch/x86/idt.h"
#include "../../arch/x86/pic.h"

#define PS2_DATA_PORT 0x60
#define PS2_CMD_PORT  0x64

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" :: "a"(val), "Nd"(port));
}

static void mouse_wait(uint8_t a_type) {
    uint32_t timeout = 100000;
    if (a_type == 0) {
        while (timeout--) { if ((inb(PS2_CMD_PORT) & 1) == 1) return; }
    } else {
        while (timeout--) { if ((inb(PS2_CMD_PORT) & 2) == 0) return; }
    }
}

static void mouse_write(uint8_t write) {
    mouse_wait(1);
    outb(PS2_CMD_PORT, 0xD4);
    mouse_wait(1);
    outb(PS2_DATA_PORT, write);
}

static void mouse_callback(CPU_Regs* regs) {
    (void)regs;
    uint8_t status = inb(PS2_CMD_PORT);
    if (status & 0x01) {
        uint8_t mouse_in = inb(PS2_DATA_PORT);
        /* TODO: Process 3-byte mouse packet for future LVGL pointer */
        (void)mouse_in;
    }
    pic_send_eoi(12); /* Send End of Interrupt for IRQ12 */
}

void mouse_install(void) {
    /* Enable auxiliary mouse device */
    mouse_wait(1);
    outb(PS2_CMD_PORT, 0xA8);
    
    mouse_wait(1);
    outb(PS2_CMD_PORT, 0x20);
    uint8_t status = (inb(PS2_DATA_PORT) | 2);
    mouse_wait(1);
    outb(PS2_CMD_PORT, 0x60);
    mouse_wait(1);
    outb(PS2_DATA_PORT, status);
    
    mouse_write(0xF6); /* Set defaults */
    mouse_write(0xF4); /* Enable data reporting */
    
    irq_register(12, mouse_callback);
}