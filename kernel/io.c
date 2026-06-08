#include "include/kernel.h"

void outb(uint16_t port, uint8_t val) {
    __asm__ __volatile__ ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}