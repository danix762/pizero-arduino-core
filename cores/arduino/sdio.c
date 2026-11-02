// sdio.c - Minimal driver for the BCM2835 Arasan EMMC controller (Pi Zero / Pi 1)
//
// NOTE: this is NOT a standard SDHCI register layout. All registers are 32-bit,
// word-aligned. Never use 8/16-bit or unaligned accesses on this block.

#include "sdio.h"
#include <stddef.h>
#include "mailbox.h"
// ---------------------------------------------------------------------------
// Config
// ---------------------------------------------------------------------------
#define PERI_BASE        0x20000000u
#define EMMC_BASE        (PERI_BASE + 0x300000u)

// EMMC base clock. 250 MHz on Pi 1 / Zero. Set SD_QUERY_CLOCK_MAILBOX to 1 to
// ask the firmware instead (needs uncached memory or a cache clean, see below).
#define SD_BASE_CLOCK_HZ 200000000u
#define SD_QUERY_CLOCK_MAILBOX 0

#define SD_ID_CLOCK_HZ   400000u
#define SD_RUN_CLOCK_HZ  25000000u

// ---------------------------------------------------------------------------
// Registers
// ---------------------------------------------------------------------------
#define REG(off)   (*(volatile uint32_t *)(EMMC_BASE + (off)))

#define EMMC_ARG2        0x00
#define EMMC_BLKSIZECNT  0x04
#define EMMC_ARG1        0x08
#define EMMC_CMDTM       0x0C
#define EMMC_RESP0       0x10
#define EMMC_RESP1       0x14
#define EMMC_RESP2       0x18
#define EMMC_RESP3       0x1C
#define EMMC_DATA        0x20
#define EMMC_STATUS      0x24
#define EMMC_CONTROL0    0x28
#define EMMC_CONTROL1    0x2C
#define EMMC_INTERRUPT   0x30
#define EMMC_IRPT_MASK   0x34
#define EMMC_IRPT_EN     0x38

// STATUS
#define ST_CMD_INHIBIT   (1u << 0)
#define ST_DAT_INHIBIT   (1u << 1)

// CONTROL0
#define C0_HCTL_DWIDTH   (1u << 1)      // 4-bit mode

// CONTROL1
#define C1_CLK_INTLEN    (1u << 0)
#define C1_CLK_STABLE    (1u << 1)
#define C1_CLK_EN        (1u << 2)
#define C1_TOUNIT_MAX    (0xEu << 16)
#define C1_SRST_HC       (1u << 24)
#define C1_SRST_CMD      (1u << 25)
#define C1_SRST_DATA     (1u << 26)

// INTERRUPT
#define INT_CMD_DONE     (1u << 0)
#define INT_DATA_DONE    (1u << 1)
#define INT_WRITE_RDY    (1u << 4)
#define INT_READ_RDY     (1u << 5)
#define INT_CARD         (1u << 8)
#define INT_ERR_ANY      0x017F8000u    // bit 15 (summary) + error bits 16..22, 24

// CMDTM fields
#define RSP_NONE         (0u << 16)
#define RSP_136          (1u << 16)
#define RSP_48           (2u << 16)
#define RSP_48B          (3u << 16)     // 48-bit with busy
#define CMD_CRC          (1u << 19)
#define CMD_IDX          (1u << 20)
#define CMD_ISDATA       (1u << 21)
#define TM_READ          (1u << 4)      // card -> host

#define R1   (RSP_48  | CMD_CRC | CMD_IDX)
#define R1B  (RSP_48B | CMD_CRC | CMD_IDX)
#define R2   (RSP_136)                  // CRC stripped by hw, no checks
#define R3   (RSP_48)                   // OCR: no CRC / index check!
#define R6   (RSP_48  | CMD_CRC | CMD_IDX)
#define R7   (RSP_48  | CMD_CRC | CMD_IDX)

// SD commands
#define CMD0   0
#define CMD2   2
#define CMD3   3
#define CMD7   7
#define CMD8   8
#define CMD16  16
#define CMD17  17
#define CMD24  24
#define CMD55  55
#define ACMD6  6
#define ACMD41 41

// ---------------------------------------------------------------------------
// GPIO / timer
// ---------------------------------------------------------------------------
#define GPFSEL4    (*(volatile uint32_t *)(PERI_BASE + 0x200010))
#define GPFSEL5    (*(volatile uint32_t *)(PERI_BASE + 0x200014))
#define GPPUD      (*(volatile uint32_t *)(PERI_BASE + 0x200094))
#define GPPUDCLK1  (*(volatile uint32_t *)(PERI_BASE + 0x20009C))
#define ST_CLO     (*(volatile uint32_t *)(PERI_BASE + 0x003004))   // 1 MHz free-running

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------
void (*sd_log)(const char *msg, uint32_t value) = NULL;

static uint32_t sd_rca = 0;
static bool     sd_ready = false;
static bool     sd_hc = false;          // high-capacity: block addressing
static uint32_t sd_base_clock = SD_BASE_CLOCK_HZ;

static void LOG(const char *m, uint32_t v) { if (sd_log) sd_log(m, v); }

static void sd_udelay(uint32_t us) {
    uint32_t start = ST_CLO;
    while ((ST_CLO - start) < us) { }
}

// ---------------------------------------------------------------------------
// Optional: query EMMC base clock via mailbox property interface
// ---------------------------------------------------------------------------
#if SD_QUERY_CLOCK_MAILBOX
#define MBOX_READ    (*(volatile uint32_t *)(PERI_BASE + 0xB880 + 0x00))
#define MBOX_STATUS  (*(volatile uint32_t *)(PERI_BASE + 0xB880 + 0x18))
#define MBOX_WRITE   (*(volatile uint32_t *)(PERI_BASE + 0xB880 + 0x20))

static uint32_t sd_query_base_clock(void) {
    static volatile uint32_t mb[8] __attribute__((aligned(16)));
    mb[0] = 8 * 4; mb[1] = 0;
    mb[2] = 0x00030002; mb[3] = 8; mb[4] = 0;
    mb[5] = 1;           // clock id 1 = EMMC
    mb[6] = 0; mb[7] = 0;
    // 0xC0000000 = uncached bus alias. If your D-cache is on, clean this buffer first.
    uint32_t addr = ((uint32_t)mb & ~0xFu) | 0xC0000000u | 8u;
    while (MBOX_STATUS & 0x80000000u) { }
    MBOX_WRITE = addr;
    for (;;) {
        while (MBOX_STATUS & 0x40000000u) { }
        if (MBOX_READ == addr) break;
    }
    return (mb[1] == 0x80000000u && mb[6]) ? mb[6] : SD_BASE_CLOCK_HZ;
}
#endif

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static void sd_gpio_init(void) {
    // GPIO48..53 -> ALT3 (0b111) = EMMC (CLK, CMD, DAT0..3)
    uint32_t r = GPFSEL4;
    r &= ~((7u << 24) | (7u << 27));
    r |=  ((7u << 24) | (7u << 27));
    GPFSEL4 = r;

    r = GPFSEL5;
    r &= ~0xFFFu;
    r |=  (7u << 0) | (7u << 3) | (7u << 6) | (7u << 9);
    GPFSEL5 = r;

    // Pull-ups on CMD + DAT0..3 (GPIO49..53), none on CLK (48)
    GPPUD = 2;
    sd_udelay(10);
    GPPUDCLK1 = (1u << (49 - 32)) | (1u << (50 - 32)) | (1u << (51 - 32)) |
                (1u << (52 - 32)) | (1u << (53 - 32));
    sd_udelay(10);
    GPPUD = 0;
    GPPUDCLK1 = 0;
}

static int sd_wait_status_clear(uint32_t mask, uint32_t timeout_us) {
    uint32_t start = ST_CLO;
    while (REG(EMMC_STATUS) & mask) {
        if ((ST_CLO - start) > timeout_us) return SD_TIMEOUT;
    }
    return SD_OK;
}

static int sd_wait_ctrl1_clear(uint32_t mask, uint32_t timeout_us) {
    uint32_t start = ST_CLO;
    while (REG(EMMC_CONTROL1) & mask) {
        if ((ST_CLO - start) > timeout_us) return SD_TIMEOUT;
    }
    return SD_OK;
}

// Reset the CMD and DATA circuits after an error
static void sd_reset_lines(void) {
    REG(EMMC_CONTROL1) |= (C1_SRST_CMD | C1_SRST_DATA);
    sd_wait_ctrl1_clear(C1_SRST_CMD | C1_SRST_DATA, 100000);
    REG(EMMC_INTERRUPT) = 0xFFFFFFFFu;
}

static int sd_wait_int(uint32_t mask, uint32_t timeout_us) {
    uint32_t start = ST_CLO;
    for (;;) {
        uint32_t s = REG(EMMC_INTERRUPT);
        if (s & INT_ERR_ANY) {
            LOG("int error status", s);
            REG(EMMC_INTERRUPT) = 0xFFFFFFFFu;
            return SD_ERROR;
        }
        if (s & mask) {
            REG(EMMC_INTERRUPT) = mask;
            return SD_OK;
        }
        if ((ST_CLO - start) > timeout_us) {
            LOG("int timeout, status", s);
            return SD_TIMEOUT;
        }
    }
}

static int sd_set_clock(uint32_t hz) {
    if (sd_wait_status_clear(ST_CMD_INHIBIT | ST_DAT_INHIBIT, 100000) != SD_OK)
        return SD_TIMEOUT;

    uint32_t c = REG(EMMC_CONTROL1);
    c &= ~C1_CLK_EN;
    REG(EMMC_CONTROL1) = c;
    sd_udelay(20);

    // SDCLK = base / (2 * N), N = 10-bit divider
    uint32_t n = (sd_base_clock + (2u * hz) - 1u) / (2u * hz);
    if (n < 1) n = 1;
    if (n > 1023) n = 1023;

    c &= ~0xFFE0u;                                   // clear divider + GENSEL (divided mode)
    c |= ((n & 0xFFu) << 8) | (((n >> 8) & 3u) << 6);
    c |= C1_CLK_INTLEN;
    REG(EMMC_CONTROL1) = c;
    sd_udelay(20);

    uint32_t start = ST_CLO;
    while (!(REG(EMMC_CONTROL1) & C1_CLK_STABLE)) {
        if ((ST_CLO - start) > 100000) return SD_TIMEOUT;
    }

    REG(EMMC_CONTROL1) |= C1_CLK_EN;
    sd_udelay(100);
    return SD_OK;
}

static int sd_cmd(uint32_t idx, uint32_t flags, uint32_t arg, uint32_t *resp) {
    uint32_t inhibit = ST_CMD_INHIBIT;
    if ((flags & CMD_ISDATA) || ((flags & (3u << 16)) == RSP_48B))
        inhibit |= ST_DAT_INHIBIT;

    if (sd_wait_status_clear(inhibit, 500000) != SD_OK) {
        LOG("cmd inhibit stuck, status", REG(EMMC_STATUS));
        return SD_TIMEOUT;
    }

    REG(EMMC_INTERRUPT) = 0xFFFFFFFFu;
    REG(EMMC_ARG1) = arg;
    REG(EMMC_CMDTM) = (idx << 24) | flags;

    int r = sd_wait_int(INT_CMD_DONE, 1000000);
    if (r != SD_OK) {
        sd_reset_lines();
        return r;
    }

    if (resp) {
        resp[0] = REG(EMMC_RESP0);
        resp[1] = REG(EMMC_RESP1);
        resp[2] = REG(EMMC_RESP2);
        resp[3] = REG(EMMC_RESP3);
    }

    // R1b: wait for the card to release DAT0
    if ((flags & (3u << 16)) == RSP_48B) {
        r = sd_wait_int(INT_DATA_DONE, 1000000);
        if (r != SD_OK) { sd_reset_lines(); return r; }
    }
    return SD_OK;
}

static int sd_app_cmd(uint32_t acmd, uint32_t flags, uint32_t arg, uint32_t *resp) {
    int r = sd_cmd(CMD55, R1, sd_rca << 16, NULL);
    if (r != SD_OK) return r;
    return sd_cmd(acmd, flags, arg, resp);
}

// ---------------------------------------------------------------------------
// Init
// ---------------------------------------------------------------------------
int sd_init(void) {
    uint32_t resp[4] = {0};
    sd_ready = false;
    sd_hc = false;
    sd_rca = 0;

    sd_base_clock = mbox_get_clock_rate(MBOX_CLK_EMMC);
    if (sd_base_clock == 0) sd_base_clock = SD_BASE_CLOCK_HZ;
    
    LOG("base clock", sd_base_clock);

    // 1. Pins
    LOG("gpio", 0);
    sd_gpio_init();

    // 2. Reset host controller
    LOG("reset", 0);
    REG(EMMC_CONTROL0) = 0;
    uint32_t c = REG(EMMC_CONTROL1);
    c |= C1_SRST_HC;
    c &= ~(C1_CLK_EN | C1_CLK_INTLEN);
    REG(EMMC_CONTROL1) = c;
    if (sd_wait_ctrl1_clear(C1_SRST_HC | C1_SRST_CMD | C1_SRST_DATA, 1000000) != SD_OK) {
        LOG("reset never cleared, CONTROL1", REG(EMMC_CONTROL1));
        return -2;
    }

    // 3. Max data timeout, identification clock
    REG(EMMC_CONTROL1) |= C1_TOUNIT_MAX;
    LOG("clock 400k", 0);
    if (sd_set_clock(SD_ID_CLOCK_HZ) != SD_OK) {
        LOG("clock never stable, CONTROL1", REG(EMMC_CONTROL1));
        return -3;
    }

    // 4. Interrupt flags (polled; nothing is wired to the IRQ controller)
    REG(EMMC_IRPT_MASK) = 0xFFFFFFFFu & ~INT_CARD;
    REG(EMMC_IRPT_EN)   = 0xFFFFFFFFu & ~INT_CARD;
    REG(EMMC_INTERRUPT) = 0xFFFFFFFFu;
    sd_udelay(5000);

    // 5. CMD0
    LOG("CMD0", 0);
    if (sd_cmd(CMD0, RSP_NONE, 0, NULL) != SD_OK) return -10;
    sd_udelay(2000);

    // 6. CMD8 (fails on v1 cards: that's fine)
    LOG("CMD8", 0);
    bool v2 = false;
    if (sd_cmd(CMD8, R7, 0x1AA, resp) == SD_OK) {
        if ((resp[0] & 0xFFF) != 0x1AA) {
            LOG("CMD8 echo mismatch", resp[0]);
            return -11;
        }
        v2 = true;
    }
    LOG("card is v2", v2);

    // 7. ACMD41 loop (R3: no CRC/index check)
    LOG("ACMD41", 0);
    uint32_t ocr_arg = 0x00FF8000u | (v2 ? (1u << 30) : 0);
    bool ready = false;
    uint32_t start = ST_CLO;
    while ((ST_CLO - start) < 2000000) {
        if (sd_app_cmd(ACMD41, R3, ocr_arg, resp) == SD_OK && (resp[0] & (1u << 31))) {
            ready = true;
            sd_hc = (resp[0] & (1u << 30)) != 0;
            break;
        }
        sd_udelay(10000);
    }
    if (!ready) { LOG("ACMD41 never ready", resp[0]); return -13; }
    LOG("OCR", resp[0]);

    // 8. CMD2 (CID)
    LOG("CMD2", 0);
    if (sd_cmd(CMD2, R2, 0, resp) != SD_OK) return -14;

    // 9. CMD3 (RCA)
    LOG("CMD3", 0);
    if (sd_cmd(CMD3, R6, 0, resp) != SD_OK) return -15;
    sd_rca = resp[0] >> 16;
    LOG("RCA", sd_rca);

    // 10. CMD7 (select)
    LOG("CMD7", 0);
    if (sd_cmd(CMD7, R1B, sd_rca << 16, NULL) != SD_OK) return -16;

    // 11. Block length 512 for standard-capacity cards
    if (!sd_hc) {
        if (sd_cmd(CMD16, R1, 512, NULL) != SD_OK) return -17;
    }

    // 12. 4-bit bus
    LOG("4-bit", 0);
    if (sd_app_cmd(ACMD6, R1, 2, NULL) != SD_OK) return -18;
    REG(EMMC_CONTROL0) |= C0_HCTL_DWIDTH;

    // 13. Full speed
    LOG("clock 25M", 0);
    if (sd_set_clock(SD_RUN_CLOCK_HZ) != SD_OK) return -19;

    sd_ready = true;
    LOG("init done", 0);
    return SD_OK;
}

// ---------------------------------------------------------------------------
// Block I/O
// ---------------------------------------------------------------------------
int sd_read_sector(uint32_t sector, uint8_t *buffer) {
    if (!sd_ready || !buffer) return SD_ERROR;

    if (sd_wait_status_clear(ST_CMD_INHIBIT | ST_DAT_INHIBIT, 500000) != SD_OK)
        return SD_TIMEOUT;

    REG(EMMC_BLKSIZECNT) = (1u << 16) | 512u;      // 1 block of 512 bytes

    uint32_t addr = sd_hc ? sector : sector * 512u;
    int r = sd_cmd(CMD17, R1 | CMD_ISDATA | TM_READ, addr, NULL);
    if (r != SD_OK) return r;

    r = sd_wait_int(INT_READ_RDY, 500000);
    if (r != SD_OK) { sd_reset_lines(); return r; }

    for (int i = 0; i < 128; i++) {
        uint32_t w = REG(EMMC_DATA);
        buffer[i * 4 + 0] = (uint8_t)(w);
        buffer[i * 4 + 1] = (uint8_t)(w >> 8);
        buffer[i * 4 + 2] = (uint8_t)(w >> 16);
        buffer[i * 4 + 3] = (uint8_t)(w >> 24);
    }

    r = sd_wait_int(INT_DATA_DONE, 500000);
    if (r != SD_OK) { sd_reset_lines(); return r; }
    return SD_OK;
}
#define CMD13  13

static int sd_wait_ready(void) {
    uint32_t resp[4];
    uint32_t start = ST_CLO;
    while ((ST_CLO - start) < 500000) {
        if (sd_cmd(CMD13, R1, sd_rca << 16, resp) != SD_OK) return SD_ERROR;
        if (resp[0] & (1u << 8)) return SD_OK;     // READY_FOR_DATA
        sd_udelay(100);
    }
    return SD_TIMEOUT;
}
int sd_write_sector(uint32_t sector, const uint8_t *buffer) {
    if (!sd_ready || !buffer) return SD_ERROR;

    if (sd_wait_status_clear(ST_CMD_INHIBIT | ST_DAT_INHIBIT, 500000) != SD_OK)
        return SD_TIMEOUT;

    REG(EMMC_BLKSIZECNT) = (1u << 16) | 512u;

    uint32_t addr = sd_hc ? sector : sector * 512u;
    int r = sd_cmd(CMD24, R1 | CMD_ISDATA, addr, NULL);   // no TM_READ = write
    if (r != SD_OK) return r;

    r = sd_wait_int(INT_WRITE_RDY, 500000);
    if (r != SD_OK) { sd_reset_lines(); return r; }

    for (int i = 0; i < 128; i++) {
        uint32_t w = (uint32_t)buffer[i * 4 + 0]
                   | ((uint32_t)buffer[i * 4 + 1] << 8)
                   | ((uint32_t)buffer[i * 4 + 2] << 16)
                   | ((uint32_t)buffer[i * 4 + 3] << 24);
        REG(EMMC_DATA) = w;
    }

    r = sd_wait_int(INT_DATA_DONE, 1000000);
    if (r != SD_OK) { sd_reset_lines(); return r; }
    return sd_wait_ready();
}

int sd_read_sectors(uint32_t sector, uint8_t *buffer, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        int r = sd_read_sector(sector + i, buffer + i * 512);
        if (r != SD_OK) return r;
    }
    return SD_OK;
}
