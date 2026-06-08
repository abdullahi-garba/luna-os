/* arch/x86/timer.c — Luna OS PIT 8253/8254 Timer Driver
 * Configures PIT channel 0 to fire at a given frequency via IRQ0.
 * Drives: scheduler preemption, LVGL tick, ledger flush background task,
 *         system uptime counter.
 */
#include "timer.h"
#include "idt.h"
#include "pic.h"
#include "../../include/types.h"
#include "../../kernel/scheduler.h"
#include "../../kernel/ledger.h"

#define PIT_CHANNEL0  0x40
#define PIT_CMD       0x43
#define PIT_BASE_HZ   1193182UL   /* PIT oscillator frequency */

static volatile uint64_t system_ticks = 0;
static uint32_t          tick_hz      = 0;

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0,%1" :: "a"(val), "Nd"(port));
}

/* IRQ0 handler — fires at 1000Hz */
static void timer_irq_handler(CPU_Regs* regs) {
    (void)regs;
    system_ticks++;

    /* LVGL tick (1ms resolution) — compiled in only if LVGL is present */
#ifdef LUNA_LVGL_PRESENT
    extern void lv_tick_inc(uint32_t);
    lv_tick_inc(1);
#endif

    /* Scheduler preemption tick */
    scheduler_tick();

    /* Ledger flush every 10ms */
    if ((system_ticks % 10) == 0) {
        ledger_flush_task();
    }
}

void timer_init(uint32_t frequency) {
    tick_hz = frequency;
    uint32_t divisor = (uint32_t)(PIT_BASE_HZ / (uint32_t)frequency);

    /* PIT channel 0, lobyte/hibyte, rate generator (mode 2) */
    outb(PIT_CMD, 0x36);
    outb(PIT_CHANNEL0, (uint8_t)(divisor & 0xFF));
    outb(PIT_CHANNEL0, (uint8_t)((divisor >> 8) & 0xFF));

    irq_register(0, timer_irq_handler);
    pic_unmask_irq(0);
}

uint64_t timer_ticks(void) { return system_ticks; }
uint32_t timer_ms(void)    { return (uint32_t)(system_ticks); }  /* 1 tick = 1ms at 1000Hz */

void timer_sleep_ms(uint32_t ms) {
    uint64_t target = system_ticks + ms;
    while (system_ticks < target) __asm__ volatile("hlt");
}
