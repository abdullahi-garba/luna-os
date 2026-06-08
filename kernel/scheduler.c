/* kernel/scheduler.c — Luna OS Preemptive Priority Scheduler
 * Round-robin within priority levels. Context switch on every PIT tick.
 * Processes with higher priority numbers get more ticks per time-slice.
 */
#include "scheduler.h"
#include "process.h"
#include "vmm.h"
#include "string.h"
#include "../include/types.h"
#include "../arch/x86/gdt.h"

/* ── Run queue — per-priority level circular list ────────────────────────── */
#define NUM_PRIO 16
static Process* run_queues[NUM_PRIO];   /* heads of circular lists */
static Process* current_process = NULL;
static bool     scheduler_active = false;

/* ── Add to run queue ────────────────────────────────────────────────────── */
void scheduler_add(Process* p) {
    uint8_t prio = p->priority & (NUM_PRIO - 1);
    if (!run_queues[prio]) {
        run_queues[prio] = p;
        p->next = p;
    } else {
        /* Insert before the head (tail insert) */
        Process* tail = run_queues[prio];
        while (tail->next != run_queues[prio]) tail = tail->next;
        tail->next = p;
        p->next    = run_queues[prio];
    }
    p->state = PROC_READY;
}

/* ── Remove from run queue ───────────────────────────────────────────────── */
void scheduler_remove(Process* p) {
    uint8_t prio = p->priority & (NUM_PRIO - 1);
    if (!run_queues[prio]) return;

    Process* cur = run_queues[prio];
    Process* prev = NULL;
    do {
        if (cur == p) {
            if (cur->next == cur) {
                run_queues[prio] = NULL;
            } else {
                if (prev) prev->next = cur->next;
                if (run_queues[prio] == cur) run_queues[prio] = cur->next;
            }
            p->next = NULL;
            return;
        }
        prev = cur;
        cur  = cur->next;
    } while (cur != run_queues[prio]);
}

/* ── Pick next process (highest non-empty priority queue) ─────────────────── */
static Process* pick_next(void) {
    for (int prio = NUM_PRIO - 1; prio >= 0; prio--) {
        if (!run_queues[prio]) continue;
        Process* p = run_queues[prio];
        /* Advance queue head for round-robin */
        run_queues[prio] = p->next;
        if (p->state == PROC_READY) return p;
    }
    return NULL;
}

/* ── Low-level context switch (ASM) ─────────────────────────────────────── */
/* Saves current register state into 'from', restores 'to' */
extern void context_switch(CPUContext* from, CPUContext* to, uint32_t* new_pd_phys);

/* ── Scheduler tick — called from IRQ0 handler every 1ms ────────────────── */
void scheduler_tick(void) {
    if (!scheduler_active || !current_process) return;

    current_process->ticks_run++;

    /* Check if time-slice exhausted */
    if (current_process->ticks_run % current_process->ticks_alloc != 0) return;

    Process* next = pick_next();
    if (!next || next == current_process) return;

    Process* prev     = current_process;
    current_process   = next;
    next->state       = PROC_RUNNING;
    if (prev->state == PROC_RUNNING) prev->state = PROC_READY;

    /* Update TSS kernel stack for Ring 3 processes */
    if (next->ring == 3) {
        gdt_set_tss_stack(next->kernel_stack);
    }

    /* Switch virtual address space if different */
    if (prev->page_dir != next->page_dir) {
        vmm_switch_pd(next->page_dir);
    }

    context_switch(&prev->ctx, &next->ctx, NULL);
}

/* ── Init ────────────────────────────────────────────────────────────────── */
void scheduler_init(void) {
    luna_memset(run_queues, 0, sizeof(run_queues));
    scheduler_active = false;
}

void scheduler_start(void) {
    scheduler_active = true;
    current_process  = pick_next();
    if (!current_process) kernel_panic("scheduler_start: no processes");
    current_process->state = PROC_RUNNING;
    /* The first process will be switched to on the next PIT tick */
}

Process* scheduler_current(void) { return current_process; }

void scheduler_block(Process* p) {
    p->state = PROC_BLOCKED;
    scheduler_remove(p);
}

void scheduler_unblock(Process* p) {
    p->state = PROC_READY;
    scheduler_add(p);
}

void scheduler_yield(void) {
    /* Force a reschedule on the next tick by resetting the slice counter */
    if (current_process) current_process->ticks_run = 0;
    __asm__ volatile("int $0x20");  /* trigger PIT IRQ handler manually */
}
