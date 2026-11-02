// diskio.c - FatFs glue for the BCM2835 EMMC driver (sdio.c)
#include "ff.h"
#include "diskio.h"
#include "sdio.h"

static volatile DSTATUS stat = STA_NOINIT;

DSTATUS disk_status(BYTE pdrv) {
    if (pdrv != 0) return STA_NOINIT;
    return stat;
}

DSTATUS disk_initialize(BYTE pdrv) {
    if (pdrv != 0) return STA_NOINIT;
    if (stat == 0) return 0;                 // already initialised
    stat = (sd_init() == SD_OK) ? 0 : STA_NOINIT;
    return stat;
}

DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count) {
    if (pdrv != 0 || stat) return RES_NOTRDY;
    if (!count) return RES_PARERR;
    return (sd_read_sectors((uint32_t)sector, buff, count) == SD_OK) ? RES_OK : RES_ERROR;
}

#if !FF_FS_READONLY
DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count) {
    if (pdrv != 0 || stat) return RES_NOTRDY;
    if (!count) return RES_PARERR;
    for (UINT i = 0; i < count; i++) {
        if (sd_write_sector((uint32_t)sector + i, buff + i * 512) != SD_OK)
            return RES_ERROR;
    }
    return RES_OK;
}
#endif

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff) {
    if (pdrv != 0 || stat) return RES_NOTRDY;
    switch (cmd) {
        case CTRL_SYNC:        return RES_OK;
        case GET_SECTOR_SIZE:  *(WORD *)buff = 512; return RES_OK;
        case GET_BLOCK_SIZE:   *(DWORD *)buff = 1;  return RES_OK;
        default:               return RES_PARERR;
    }
}

// No RTC: fixed timestamp (2026-10-30 00:00:00)
DWORD get_fattime(void) {
    return ((DWORD)(2026 - 1980) << 25) | (10UL << 21) | (30UL << 16);
}
