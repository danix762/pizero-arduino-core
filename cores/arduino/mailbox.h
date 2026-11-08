#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum { MBOX_CLK_EMMC = 1, MBOX_CLK_UART = 2, MBOX_CLK_ARM = 3, MBOX_CLK_CORE = 4 };

// Current clock rate in Hz, or 0 if the firmware call fails.
uint32_t mbox_get_clock_rate(uint32_t clock_id);

// Generic firmware property call: one tag, up to 8 request words, up to 8 response words.
// Returns 0 on success, -1 on failure. Not safe to call from interrupt handlers.
int mbox_tag(uint32_t tag, const uint32_t *req, uint32_t req_words,
             uint32_t *resp, uint32_t resp_words);

#ifdef __cplusplus
}
#endif
