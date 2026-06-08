/* drivers/serial/uart.c — Bare-metal COM1 Serial logging */
#include "../../include/types.h"
#include "../../kernel/string.h"

#define COM1_PORT 0x3F8

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" :: "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void uart_init(void) {
    outb(COM1_PORT + 1, 0x00);    /* Disable all interrupts */
    outb(COM1_PORT + 3, 0x80);    /* Enable DLAB (set baud rate divisor) */
    outb(COM1_PORT + 0, 0x03);    /* Set divisor to 3 (lo byte) 38400 baud */
    outb(COM1_PORT + 1, 0x00);    /* (hi byte) */
    outb(COM1_PORT + 3, 0x03);    /* 8 bits, no parity, one stop bit */
    outb(COM1_PORT + 2, 0xC7);    /* Enable FIFO, clear them, with 14-byte threshold */
    outb(COM1_PORT + 4, 0x0B);    /* IRQs enabled, RTS/DSR set */
}

void uart_putc(char c) {
    while ((inb(COM1_PORT + 5) & 0x20) == 0); /* Wait for transmit empty */
    outb(COM1_PORT, c);
}

void uart_puts(const char* str) {
    while (*str) {
        uart_putc(*str++);
    }
}