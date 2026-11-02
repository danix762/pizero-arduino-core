#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Calls fn() every period_us microseconds from interrupt context (system timer match 1).
// period_us must be >= 20. Returns 0 on success, -1 on bad arguments.
int      ticker_start(uint32_t period_us, void (*fn)(void));
void     ticker_stop(void);
uint32_t ticker_overruns(void);    // ticks that arrived late (the handler took longer than the period)

#ifdef __cplusplus
}
#endif
