/* kernel/syscall.h */
#ifndef LUNA_SYSCALL_H
#define LUNA_SYSCALL_H
#include "../include/types.h"
#include "../arch/x86/idt.h"

/* Syscall numbers (EAX on int 0x80) */
#define SYSCALL_EXIT            0
#define SYSCALL_READ            1
#define SYSCALL_WRITE           2
#define SYSCALL_OPEN            3
#define SYSCALL_CLOSE           4
#define SYSCALL_GETPID          5
#define SYSCALL_FORK            6
#define SYSCALL_MAP_SURFACE     7
#define SYSCALL_SUBMIT_SURFACE  8
#define SYSCALL_GET_INPUT       9
#define SYSCALL_SBRK            10
#define SYSCALL_SLEEP           11
#define SYSCALL_AI_QUERY        12
#define SYSCALL_LEDGER_READ     13
#define SYSCALL_COUNT           14

void     syscall_init(void);
uint32_t syscall_dispatch(CPU_Regs* regs);

/* Userspace helper macro (used in Ring 3 apps) */
#define LUNA_SYSCALL(num, a, b, c) ({ \
    uint32_t _r; \
    __asm__ volatile("int $0x80" \
        : "=a"(_r) \
        : "a"(num), "b"(a), "c"(b), "d"(c) \
        : "memory"); \
    _r; })

#endif
