/* kernel/ledger_queue.c — Lock-free single-producer/single-consumer ring buffer
 * for ledger events. The VFS hot path pushes; the scheduler background pops.
 * Power-of-2 size enables modulo via bitmask (no division).
 */

#include "ledger_queue.h"
#include "string.h"
#include "../include/types.h"

/* ── Static ring buffer ──────────────────────────────────────────────────── */
#define QUEUE_MASK (LEDGER_QUEUE_SIZE - 1)

/* Verified at compile time that size is power of two */
typedef struct {
    LedgerEvent      events[LEDGER_QUEUE_SIZE];
    volatile uint32_t head;   /* written by producer (VFS hot path) */
    volatile uint32_t tail;   /* written by consumer (background task) */
} LedgerQueue;

static LedgerQueue lq;

/* ── Init ────────────────────────────────────────────────────────────────── */
void ledger_queue_init(void) {
    lq.head = 0;
    lq.tail = 0;
    luna_memset(lq.events, 0, sizeof(lq.events));
}

/* ── Push (producer — called from VFS, interrupt context safe) ──────────── */
bool ledger_queue_push(const LedgerEvent* ev) {
    uint32_t head = lq.head;
    uint32_t next = (head + 1) & QUEUE_MASK;

    if (next == lq.tail) {
        /* Queue full — drop event silently (forensic gap noted in design) */
        /* In production: increment a saturation counter and flush urgently */
        return false;
    }

    lq.events[head] = *ev;

    /* Memory barrier: ensure event is written before head advances */
    __asm__ volatile("" ::: "memory");
    lq.head = next;
    return true;
}

/* ── Pop (consumer — called from background scheduler task only) ─────────── */
bool ledger_queue_pop(LedgerEvent* ev) {
    uint32_t tail = lq.tail;

    if (tail == lq.head) {
        return false; /* empty */
    }

    *ev = lq.events[tail];

    /* Memory barrier: ensure event is read before tail advances */
    __asm__ volatile("" ::: "memory");
    lq.tail = (tail + 1) & QUEUE_MASK;
    return true;
}

/* ── Query ───────────────────────────────────────────────────────────────── */
uint32_t ledger_queue_depth(void) {
    return (lq.head - lq.tail) & QUEUE_MASK;
}
