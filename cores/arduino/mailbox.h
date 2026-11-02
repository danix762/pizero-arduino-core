#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum { MBOX_CLK_EMMC = 1, MBOX_CLK_UART = 2, MBOX_CLK_ARM = 3, MBOX_CLK_CORE = 4 };

// Current clock rate in Hz, or 0 if the firmware call fails.
uint32_t mbox_get_clock_rate(uint32_t clock_id);

#ifdef __cplusplus
}
#endif
