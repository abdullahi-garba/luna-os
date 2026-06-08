/* drivers/input/keyboard.c — PS/2 Keyboard Driver */
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

static void keyboard_callback(CPU_Regs* regs) {
    (void)regs;
    uint8_t scancode = inb(PS2_DATA_PORT);
    
    /* TODO: Map scancode to ASCII and pass to shell_on_keyboard_event() */
    (void)scancode;
    
    pic_send_eoi(1); /* Send End of Interrupt to PIC for IRQ1 */
}

void keyboard_install(void) {
    irq_register(1, keyboard_callback);
}