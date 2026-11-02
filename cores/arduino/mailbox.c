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

// Exactly one 32-byte cache line, so cache maintenance can't touch neighbouring data.
static uint32_t mbuf[8] __attribute__((aligned(32)));

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
    dcache_invalidate_range(mbuf, sizeof(mbuf)); // drop stale cached copy of the reply
    return (mbuf[1] == 0x80000000u) ? 0 : -1;
}

uint32_t mbox_get_clock_rate(uint32_t clock_id) {
    mbuf[0] = sizeof(mbuf);          // total size in bytes
    mbuf[1] = 0;                     // request
    mbuf[2] = 0x00030002;            // tag: get clock rate
    mbuf[3] = 8;                     // value buffer size
    mbuf[4] = 0;                     // request code
    mbuf[5] = clock_id;
    mbuf[6] = 0;                     // rate comes back here
    mbuf[7] = 0;                     // end tag
    if (mbox_call() != 0) return 0;
    return mbuf[6];
}
