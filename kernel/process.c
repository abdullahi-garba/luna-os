/* kernel/process.c — Luna OS Process Manager */
#include "process.h"
#include "heap.h"
#include "vmm.h"
#include "pmm.h"
#include "string.h"
#include "../include/types.h"
#include "../include/panic.h"
#include "../arch/x86/gdt.h"

static Process  process_table[MAX_PROCESSES];
static uint32_t pid_counter   = 1;
static Process* current_proc  = NULL;

void process_init(void) {
    luna_memset(process_table, 0, sizeof(process_table));
    current_proc = NULL;
}

uint32_t process_next_pid(void) { return pid_counter++; }

Process* process_get_current(void)          { return current_proc; }
void     process_set_current(Process* p)    { current_proc = p; }

Process* process_get_by_pid(uint32_t pid) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].state != PROC_DEAD &&
            process_table[i].pid == pid) return &process_table[i];
    }
    return NULL;
}

/* ── Find a free slot in the process table ───────────────────────────────── */
static Process* alloc_slot(void) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].state == PROC_DEAD) return &process_table[i];
    }
    kernel_panic("process_create: process table full");
    return NULL;
}

/* ── Create a Ring 0 kernel task ─────────────────────────────────────────── */
Process* process_create(const char* name, void* entry, uint8_t ring, uint8_t priority) {
    Process* p = alloc_slot();
    luna_memset(p, 0, sizeof(Process));

    p->pid        = process_next_pid();
    p->ring       = ring;
    p->priority   = priority;
    p->state      = PROC_READY;
    p->ticks_alloc = (uint32_t)(1 + priority);
    luna_strncpy(p->name, name, 31);

    /* Allocate a kernel stack for this process */
    uint32_t kstack_phys = pmm_alloc_frames(4);  /* 16KB kernel stack */
    p->kernel_stack = kstack_phys + 0xC0000000U + (4 * 4096);

    /* For Ring 0 tasks: ESP points directly into kernel stack */
    if (ring == 0) {
        p->page_dir    = vmm_get_kernel_pd();
        p->ctx.esp     = p->kernel_stack;
        p->ctx.eip     = (uint32_t)entry;
        p->ctx.eflags  = 0x202;   /* IF=1 */
        p->ctx.cs      = 0x08;
        p->ctx.ds = p->ctx.es = p->ctx.ss = 0x10;
    }
    return p;
}

/* ── Spawn a Ring 3 user process ─────────────────────────────────────────── */
Process* process_spawn_ring3(const char* name, void* entry) {
    Process* p = alloc_slot();
    luna_memset(p, 0, sizeof(Process));

    p->pid        = process_next_pid();
    p->ring       = 3;
    p->priority   = 8;
    p->state      = PROC_READY;
    p->ticks_alloc = 8;
    luna_strncpy(p->name, name, 31);

    /* Own page directory — kernel higher-half copied in */
    p->page_dir   = vmm_create_user_pd();

    /* Allocate kernel stack (used on Ring 3 → Ring 0 transitions) */
    uint32_t kstack_phys = pmm_alloc_frames(4);
    p->kernel_stack = kstack_phys + 0xC0000000U + (4 * 4096);

    /* Allocate user stack at 0xBFFFF000 (top of user address space) */
    uint32_t ustack_phys = pmm_alloc_frames(PROCESS_STACK_SIZE / 4096);
    uint32_t ustack_virt = 0xBFFFF000U;
    for (uint32_t i = 0; i < PROCESS_STACK_SIZE / 4096; i++) {
        vmm_map_page(p->page_dir,
            ustack_virt - PROCESS_STACK_SIZE + i * 4096,
            ustack_phys + i * 4096,
            VMM_FLAG_PRESENT | VMM_FLAG_WRITE | VMM_FLAG_USER);
    }
    p->user_stack = ustack_virt;

    /* Build a synthetic iret frame on the KERNEL stack.
     * When the scheduler first context-switches to this process it will
     * 'return' from the context switch right into the iret, which will
     * drop into Ring 3 at the entry point. */
    uint32_t* ksp = (uint32_t*)p->kernel_stack;
    *--ksp = 0x23;          /* SS (Ring 3 data, RPL=3) */
    *--ksp = ustack_virt;   /* ESP in user space */
    *--ksp = 0x202;         /* EFLAGS: IF=1 */
    *--ksp = 0x1B;          /* CS (Ring 3 code, RPL=3) */
    *--ksp = (uint32_t)entry; /* EIP */
    /* Registers pushed by context switch prologue: */
    *--ksp = 0; /* eax */ *--ksp = 0; /* ecx */ *--ksp = 0; /* edx */
    *--ksp = 0; /* ebx */ *--ksp = 0; /* esp_dummy */
    *--ksp = 0; /* ebp */ *--ksp = 0; /* esi */ *--ksp = 0; /* edi */

    p->ctx.esp  = (uint32_t)ksp;
    p->ctx.eip  = (uint32_t)entry;

    /* Tell TSS about this process's kernel stack for future syscalls */
    gdt_set_tss_stack(p->kernel_stack);

    return p;
}

/* ── Kill a process ──────────────────────────────────────────────────────── */
void process_kill(uint32_t pid) {
    Process* p = process_get_by_pid(pid);
    if (!p) return;
    p->state = PROC_ZOMBIE;
    /* TODO: free page dir, stacks, release held resources */
}
