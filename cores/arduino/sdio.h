#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Return codes
#define SD_OK           0
#define SD_ERROR        -1
#define SD_TIMEOUT      -2
#define SD_NO_CARD      -3

// Optional debug hook: set to a function that prints (msg, value)
extern void (*sd_log)(const char *msg, uint32_t value);

int sd_init(void);
int sd_read_sector(uint32_t sector, uint8_t *buffer);
int sd_write_sector(uint32_t sector, const uint8_t *buffer);
int sd_read_sectors(uint32_t sector, uint8_t *buffer, uint32_t count);


#ifdef __cplusplus
}
#endif
