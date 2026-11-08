#include "Arduino.h"
#include "mailbox.h"
#include "cache.h"

#define MBOX_BASE    0x2000B880UL
#define MBOX_READ    (*(volatile uint32_t *)(MBOX_BASE + 0x00))
#define MBOX_STATUS  (*(volatile uint32_t *)(MBOX_BASE + 0x18))
#define MBOX_WRITE   (*(volatile uint32_t *)(MBOX_BASE + 0x20))
#define MBOX_FULL    0x80000000u
#define MBOX_EMPTY   0x40000000u
#define MBOX_CH_PROP 8

// Two 32-byte cache lines, so cache maintenance can't touch neighbouring data.
static uint32_t mbuf[16] __attribute__((aligned(32)));

static inline void dsb(void) { __asm__ volatile("mcr p15,0,%0,c7,c10,4" :: "r"(0) : "memory"); }

static int mbox_call(void) {
    uint32_t msg = (((uint32_t)mbuf) | 0xC0000000u) & ~0xFu;   // 0xC0000000 = uncached bus alias
    msg |= MBOX_CH_PROP;

    dcache_clean_range(mbuf, sizeof(mbuf));      // push our request out to RAM
    dsb();

    uint32_t t0 = micros();
    while (MBOX_STATUS & MBOX_FULL)
        if ((uint32_t)(micros() - t0) > 100000) return -1;
    MBOX_WRITE = msg;

    for (;;) {
        while (MBOX_STATUS & MBOX_EMPTY)
            if ((uint32_t)(micros() - t0) > 200000) return -1;
        if (MBOX_READ == msg) break;
    }
    dsb();
    dcache_invalidate_range(mbuf, sizeof(mbuf)); // drop the stale cached copy of the reply
    return (mbuf[1] == 0x80000000u) ? 0 : -1;
}

int mbox_tag(uint32_t tag, const uint32_t *req, uint32_t req_words,
             uint32_t *resp, uint32_t resp_words) {
    uint32_t n = req_words > resp_words ? req_words : resp_words;
    if (n > 8) return -1;
    for (uint32_t i = 0; i < 16; i++) mbuf[i] = 0;
    mbuf[0] = (6 + n) * 4;           // total message size in bytes
    mbuf[1] = 0;                     // request
    mbuf[2] = tag;
    mbuf[3] = n * 4;                 // value buffer size
    mbuf[4] = 0;                     // request code
    for (uint32_t i = 0; i < req_words; i++) mbuf[5 + i] = req[i];
    // mbuf[5 + n] is the end tag (already zero)
    if (mbox_call() != 0) return -1;
    if (!(mbuf[4] & 0x80000000u)) return -1;     // firmware did not answer this tag
    for (uint32_t i = 0; i < resp_words; i++) resp[i] = mbuf[5 + i];
    return 0;
}

uint32_t mbox_get_clock_rate(uint32_t clock_id) {
    uint32_t q[1] = { clock_id };
    uint32_t r[2];
    return (mbox_tag(0x00030002, q, 1, r, 2) == 0) ? r[1] : 0;
}
