/* kernel/scheduler.h */
#ifndef LUNA_SCHEDULER_H
#define LUNA_SCHEDULER_H
#include "../include/types.h"
#include "process.h"
void     scheduler_init(void);
void     scheduler_start(void);
void     scheduler_tick(void);      /* called from PIT IRQ */
void     scheduler_add(Process* p);
void     scheduler_remove(Process* p);
void     scheduler_block(Process* p);
void     scheduler_unblock(Process* p);
void     scheduler_yield(void);
Process* scheduler_current(void);
extern void kernel_panic(const char*);
#endif
