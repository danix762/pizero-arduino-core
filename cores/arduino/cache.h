// cache.h
#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
void icache_enable(void);
void mmu_enable(void);
void dcache_clean_range(const void *p, uint32_t len);
void dcache_invalidate_range(void *p, uint32_t len);
#ifdef __cplusplus
}
#endif
