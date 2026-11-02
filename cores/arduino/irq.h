#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*irq_handler_t)(void);

void irq_init(void);                               // vectors, stacks, controller, unmask
void irq_register(unsigned irq, irq_handler_t fn); // irq = 0..63 (BCM2835 numbering)
void irq_enable(unsigned irq);
void irq_disable(unsigned irq);

static inline void interrupts(void)   { __asm__ volatile("cpsie i" ::: "memory"); }
static inline void noInterrupts(void) { __asm__ volatile("cpsid i" ::: "memory"); }

#ifdef __cplusplus
}
#endif
