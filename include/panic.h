/* include/panic.h — Luna OS kernel panic interface */
#ifndef LUNA_PANIC_H
#define LUNA_PANIC_H

#include "types.h"

NORETURN void kernel_panic(const char* msg);

/* Convenience macro that stamps file + line */
#define PANIC(msg) kernel_panic("[" __FILE__ ":" STRINGIFY(__LINE__) "] " msg)
#define STRINGIFY(x) _STRINGIFY(x)
#define _STRINGIFY(x) #x

/* Assertion that panics on failure */
#define ASSERT(cond) \
    do { if (!(cond)) PANIC("Assertion failed: " #cond); } while(0)

#endif /* LUNA_PANIC_H */
