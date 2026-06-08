/* kernel/process.h */
#ifndef LUNA_PROCESS_H
#define LUNA_PROCESS_H
#include "../include/types.h"

#define MAX_PROCESSES    64
#define PROCESS_STACK_SIZE (64 * 1024)  /* 64KB user stack per process */

typedef enum {
    PROC_READY,
    PROC_RUNNING,
    PROC_BLOCKED,
    PROC_ZOMBIE,
    PROC_DEAD
} ProcessState;

typedef struct {
    /* Saved CPU registers (on context switch) */
    uint32_t eax, ebx, ecx, edx;
    uint32_t esi, edi, ebp, esp;
    uint32_t eip, eflags;
    uint32_t cs, ds, es, fs, gs, ss;
} CPUContext;

typedef struct Process {
    uint32_t     pid;
    char         name[32];
    ProcessState state;
    uint8_t      ring;          /* 0 = kernel task, 3 = user process */
    uint8_t      priority;      /* 0 (lowest) – 15 (highest) */
    CPUContext   ctx;
    uint32_t*    page_dir;      /* physical address of page directory */
    uint32_t     user_stack;    /* top of user stack (virtual) */
    uint32_t     kernel_stack;  /* top of kernel stack for this process */
    uint32_t     ticks_run;     /* total ticks consumed */
    uint32_t     ticks_alloc;   /* ticks per time-slice */
    struct Process* next;       /* scheduler linked list */
} Process;

void     process_init(void);
Process* process_create(const char* name, void* entry, uint8_t ring, uint8_t priority);
void     process_kill(uint32_t pid);
Process* process_get_current(void);
Process* process_get_by_pid(uint32_t pid);
uint32_t process_next_pid(void);

/* Launch a Ring 3 binary at given entry point in its own address space */
Process* process_spawn_ring3(const char* name, void* entry);

#endif /* LUNA_PROCESS_H */
