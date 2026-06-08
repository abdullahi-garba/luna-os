/* arch/x86/timer.h */
#ifndef LUNA_TIMER_H
#define LUNA_TIMER_H
#include "../../include/types.h"
void     timer_init(uint32_t frequency);
uint64_t timer_ticks(void);
uint32_t timer_ms(void);
void     timer_sleep_ms(uint32_t ms);
#endif
