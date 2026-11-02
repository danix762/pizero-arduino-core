#include "ticker.h"
#include "irq.h"

#define ST_BASE        0x20003000UL
#define ST_CS          (*(volatile uint32_t *)(ST_BASE + 0x00))
#define ST_CLO         (*(volatile uint32_t *)(ST_BASE + 0x04))
#define ST_C1          (*(volatile uint32_t *)(ST_BASE + 0x10))
#define IRQ_SYSTIMER1  1

static void (*tick_fn)(void);
static uint32_t period;
static volatile uint32_t late_count;

static void ticker_isr(void) {
    ST_CS = 1u << 1;                          // acknowledge match 1

    uint32_t next = ST_C1 + period;           // schedule the next tick first, so the
    if ((int32_t)(next - ST_CLO) < 5) {       // handler's duration doesn't shift the rhythm
        late_count++;                         // already behind (or about to be): resync
        next = ST_CLO + period;
    }
    ST_C1 = next;

    if (tick_fn) tick_fn();
}

int ticker_start(uint32_t period_us, void (*fn)(void)) {
    if (period_us < 20 || !fn) return -1;
    noInterrupts();
    period = period_us;
    tick_fn = fn;
    late_count = 0;
    ST_CS = 1u << 1;
    ST_C1 = ST_CLO + period_us;
    irq_register(IRQ_SYSTIMER1, ticker_isr);
    irq_enable(IRQ_SYSTIMER1);
    interrupts();
    return 0;
}

void ticker_stop(void) {
    irq_disable(IRQ_SYSTIMER1);
    ST_CS = 1u << 1;
    tick_fn = 0;
}

uint32_t ticker_overruns(void) { return late_count; }
