// cache.c - ARM1176 cache / MMU setup for BCM2835
#include <stdint.h>

#define SCTLR_M   (1u << 0)    // MMU enable
#define SCTLR_C   (1u << 2)    // D-cache enable
#define SCTLR_Z   (1u << 11)   // branch prediction
#define SCTLR_I   (1u << 12)   // I-cache
#define SCTLR_XP  (1u << 23)   // ARMv6 page table format

static inline uint32_t sctlr_read(void) {
    uint32_t r; asm volatile("mrc p15,0,%0,c1,c0,0" : "=r"(r)); return r;
}
static inline void sctlr_write(uint32_t r) {
    asm volatile("mcr p15,0,%0,c1,c0,0" :: "r"(r) : "memory");
}
static inline void isb(void) {
    asm volatile("mcr p15,0,%0,c7,c5,4" :: "r"(0) : "memory");
}
static inline void dsb(void) {
    asm volatile("mcr p15,0,%0,c7,c10,4" :: "r"(0) : "memory");
}

void icache_enable(void) {
    asm volatile("mcr p15,0,%0,c7,c5,0" :: "r"(0) : "memory");   // invalidate I-cache
    asm volatile("mcr p15,0,%0,c7,c5,6" :: "r"(0) : "memory");   // flush branch target cache
    dsb();
    sctlr_write(sctlr_read() | SCTLR_I | SCTLR_Z);
    isb();
}
// ---- MMU / page tables (identity map, 1 MB sections) ----
// Section descriptor, ARMv6 format (SCTLR.XP = 1)
#define SEC_BASE     0x2u            // bits[1:0] = 10
#define SEC_B        (1u << 2)
#define SEC_C        (1u << 3)
#define SEC_XN       (1u << 4)
#define SEC_AP_RW    (3u << 10)      // full access, domain 0
#define SEC_TEX(x)   ((uint32_t)(x) << 12)

// Normal RAM, write-back (C=1, B=1)
#define MEM_CACHED   (SEC_BASE | SEC_AP_RW | SEC_C | SEC_B)
// Normal RAM, non-cacheable (TEX=001): for DMA buffers, framebuffer, GPU memory
#define MEM_UNCACHED (SEC_BASE | SEC_AP_RW | SEC_TEX(1))
// Strongly ordered: same behaviour as running with the MMU off (use for peripherals)
#define MEM_PERIPH   (SEC_BASE | SEC_AP_RW | SEC_XN)

// MUST be <= the ARM/GPU memory split, or the GPU framebuffer ends up cached.
#ifndef CACHEABLE_MB
#define CACHEABLE_MB 128
#endif

static uint32_t l1_table[4096] __attribute__((aligned(16384)));

void mmu_enable(void) {
    for (uint32_t i = 0; i < 4096; i++) {
        uint32_t pa = i << 20;
        uint32_t attr;
        if (i < CACHEABLE_MB)                          attr = MEM_CACHED;
        else if (pa >= 0x20000000u && pa < 0x21000000u) attr = MEM_PERIPH;   // BCM2835 peripherals
        else                                           attr = MEM_UNCACHED;
        l1_table[i] = pa | attr;
    }

    // Invalidate everything before turning it on
    asm volatile("mcr p15,0,%0,c7,c7,0" :: "r"(0) : "memory");   // I + D cache
    asm volatile("mcr p15,0,%0,c7,c5,6" :: "r"(0) : "memory");   // branch predictor
    asm volatile("mcr p15,0,%0,c8,c7,0" :: "r"(0) : "memory");   // TLB
    dsb();

    asm volatile("mcr p15,0,%0,c2,c0,2" :: "r"(0));               // TTBCR = 0 (use TTBR0 only)
    asm volatile("mcr p15,0,%0,c2,c0,0" :: "r"((uint32_t)l1_table));
    asm volatile("mcr p15,0,%0,c3,c0,0" :: "r"(1u));              // domain 0 = client
    isb();

    sctlr_write(sctlr_read() | SCTLR_XP | SCTLR_M | SCTLR_C | SCTLR_Z | SCTLR_I);
    isb();
}

// ---- Cache maintenance helpers (32-byte lines) ----
void dcache_clean_range(const void *p, uint32_t len) {          // flush CPU writes to RAM
    uint32_t a = (uint32_t)p & ~31u, end = (uint32_t)p + len;
    for (; a < end; a += 32)
        asm volatile("mcr p15,0,%0,c7,c10,1" :: "r"(a) : "memory");
    dsb();
}
void dcache_invalidate_range(void *p, uint32_t len) {           // discard stale cached data
    uint32_t a = (uint32_t)p & ~31u, end = (uint32_t)p + len;
    for (; a < end; a += 32)
        asm volatile("mcr p15,0,%0,c7,c6,1" :: "r"(a) : "memory");
    dsb();
}
