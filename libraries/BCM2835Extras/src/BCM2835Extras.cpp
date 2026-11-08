#include "BCM2835Extras.h"
#include "Arduino.h"
#include "mailbox.h"

BCM2835Extras BCM;

#define REG(a) (*(volatile uint32_t *)(a))

// ================================================================ hardware RNG
#define RNG_BASE      0x20104000UL
#define RNG_CTRL      REG(RNG_BASE + 0x00)
#define RNG_STATUS    REG(RNG_BASE + 0x04)
#define RNG_DATA      REG(RNG_BASE + 0x08)
#define RNG_INT_MASK  REG(RNG_BASE + 0x10)

static bool     rngStarted = false;
static bool     rngDead = false;
static uint32_t fallbackState = 0x9E3779B9u;

void BCM2835Extras::randomBegin() {
  if (rngStarted) return;
  RNG_STATUS = 0x40000;          // warm-up: discard the first 0x40000 samples
  RNG_INT_MASK |= 1;             // we poll, so mask the interrupt
  RNG_CTRL |= 1;                 // enable
  rngStarted = true;
}

bool BCM2835Extras::randomRead(uint32_t *out) {
  if (rngDead) return false;
  randomBegin();
  uint32_t t0 = micros();
  while ((RNG_STATUS >> 24) == 0) {
    if ((uint32_t)(micros() - t0) > 500000) { rngDead = true; return false; }
  }
  *out = RNG_DATA;
  return true;
}

uint32_t BCM2835Extras::random32() {
  uint32_t v;
  if (randomRead(&v)) return v;
  fallbackState ^= micros();     // hardware silent: xorshift seeded from the clock
  fallbackState ^= fallbackState << 13;
  fallbackState ^= fallbackState >> 17;
  fallbackState ^= fallbackState << 5;
  return fallbackState;
}

void BCM2835Extras::randomBytes(uint8_t *buf, size_t len) {
  while (len) {
    uint32_t v = random32();
    for (int i = 0; i < 4 && len; i++, len--) { *buf++ = (uint8_t)(v & 0xFF); v >>= 8; }
  }
}

// ================================================================ watchdog / reset
#define PM_BASE      0x20100000UL
#define PM_RSTC      REG(PM_BASE + 0x1C)
#define PM_WDOG      REG(PM_BASE + 0x24)
#define PM_PASSWORD  0x5A000000u
#define PM_WRCFG_CLR 0xFFFFFFCFu
#define PM_FULLRESET 0x00000020u
#define PM_RSTC_STOP 0x00000102u
#define PM_TIME_MASK 0x000FFFFFu         // 20-bit counter, 65536 ticks per second

static uint32_t wdTicks = 0;

static void wd_arm(uint32_t ticks) {
  PM_WDOG = PM_PASSWORD | (ticks & PM_TIME_MASK);
  PM_RSTC = PM_PASSWORD | (PM_RSTC & PM_WRCFG_CLR) | PM_FULLRESET;
}

void BCM2835Extras::reset() {
  wd_arm(10);                            // about 150 microseconds
  for (;;) { }
}

void BCM2835Extras::watchdogStart(uint32_t ms) {
  if (ms > 15999) ms = 15999;
  wdTicks = (ms * 65536u) / 1000u;
  if (wdTicks == 0) wdTicks = 1;
  wd_arm(wdTicks);
}

void BCM2835Extras::watchdogFeed()  { if (wdTicks) wd_arm(wdTicks); }

void BCM2835Extras::watchdogStop() {
  PM_RSTC = PM_PASSWORD | PM_RSTC_STOP;
  wdTicks = 0;
}

uint32_t BCM2835Extras::watchdogRemainingMs() {
  return ((PM_WDOG & PM_TIME_MASK) * 1000u) / 65536u;
}

// ================================================================ firmware info
static bool tag1(uint32_t tag, uint32_t *r, uint32_t n) { return mbox_tag(tag, 0, 0, r, n) == 0; }

uint32_t BCM2835Extras::firmwareRevision() { uint32_t r[1]; return tag1(0x00000001, r, 1) ? r[0] : 0; }
uint32_t BCM2835Extras::boardRevision()    { uint32_t r[1]; return tag1(0x00010002, r, 1) ? r[0] : 0; }

uint64_t BCM2835Extras::serialNumber() {
  uint32_t r[2];
  if (!tag1(0x00010004, r, 2)) return 0;
  return ((uint64_t)r[1] << 32) | r[0];
}

uint32_t BCM2835Extras::armMemoryBytes() { uint32_t r[2]; return tag1(0x00010005, r, 2) ? r[1] : 0; }
uint32_t BCM2835Extras::gpuMemoryBytes() { uint32_t r[2]; return tag1(0x00010006, r, 2) ? r[1] : 0; }

const char *BCM2835Extras::boardName() {
  uint32_t rev = boardRevision();
  if (rev == 0) return "unknown (no answer from firmware)";
  if (rev & (1u << 23)) {                         // new-style revision code
    switch ((rev >> 4) & 0xFF) {
      case 0x00: return "Pi 1 A";
      case 0x01: return "Pi 1 B";
      case 0x02: return "Pi 1 A+";
      case 0x03: return "Pi 1 B+";
      case 0x04: return "Pi 2 B";
      case 0x08: return "Pi 3 B";
      case 0x09: return "Pi Zero";
      case 0x0C: return "Pi Zero W";
      case 0x12: return "Pi Zero 2 W";
      default:   return "Raspberry Pi (other model)";
    }
  }
  return "Raspberry Pi 1 (old-style revision code)";
}

int32_t BCM2835Extras::temperatureMilliC() {
  uint32_t q[1] = { 0 };
  uint32_t r[2];
  if (mbox_tag(0x00030006, q, 1, r, 2) != 0) return INT32_MIN;
  return (int32_t)r[1];
}

float BCM2835Extras::temperatureC() { return temperatureMilliC() / 1000.0f; }

uint32_t BCM2835Extras::clockHz(uint32_t id) { return mbox_get_clock_rate(id); }

uint32_t BCM2835Extras::setClockHz(uint32_t id, uint32_t hz) {
  uint32_t q[3] = { id, hz, 0 };
  uint32_t r[2];
  return (mbox_tag(0x00038002, q, 3, r, 2) == 0) ? r[1] : 0;
}

// ================================================================ general-purpose clocks
#define CM_BASE  0x20101000UL
#define CM_PASS  0x5A000000u
#define CM_CTL(n) REG(CM_BASE + 0x70 + (n) * 8)
#define CM_DIV(n) REG(CM_BASE + 0x74 + (n) * 8)
#define CM_ENAB  (1u << 4)
#define CM_BUSY  (1u << 7)

static bool gp_map(uint8_t pin, int *n, uint8_t *fn) {
  switch (pin) {
    case 4:  *n = 0; *fn = 4; return true;   // ALT0
    case 20: *n = 0; *fn = 2; return true;   // ALT5
    case 5:  *n = 1; *fn = 4; return true;   // ALT0
    case 6:  *n = 2; *fn = 4; return true;   // ALT0
  }
  return false;
}

static void gp_stop(int n) {
  CM_CTL(n) = CM_PASS | ((CM_CTL(n) & 0x00FFFFFFu) & ~CM_ENAB);
  uint32_t t0 = micros();
  while ((CM_CTL(n) & CM_BUSY) && (uint32_t)(micros() - t0) < 10000) { }
}

uint32_t BCM2835Extras::gpclkStart(uint8_t pin, uint32_t hz) {
  int n; uint8_t fn;
  if (!gp_map(pin, &n, &fn)) return 0;
  if (hz < 4700 || hz > 50000000UL) return 0;

  uint32_t src, srcHz;
  if (hz > 9600000UL) { src = 6; srcHz = 500000000UL; }   // PLLD 500 MHz
  else                { src = 1; srcHz = 19200000UL; }    // oscillator 19.2 MHz

  uint64_t div = ((uint64_t)srcHz * 4096u + hz / 2) / hz; // 12.12 fixed point
  uint32_t divi = (uint32_t)(div >> 12), divf = (uint32_t)(div & 0xFFF);
  uint32_t mash = divf ? 1u : 0u;

  gp_stop(n);
  CM_DIV(n) = CM_PASS | (divi << 12) | divf;
  CM_CTL(n) = CM_PASS | src | (mash << 9);
  delayMicroseconds(10);
  CM_CTL(n) = CM_PASS | src | (mash << 9) | CM_ENAB;
  uint32_t t0 = micros();
  while (!(CM_CTL(n) & CM_BUSY) && (uint32_t)(micros() - t0) < 10000) { }

  pinFunction(pin, fn);
  return (uint32_t)(((uint64_t)srcHz * 4096u) / div);
}

void BCM2835Extras::gpclkStop(uint8_t pin) {
  int n; uint8_t fn;
  if (!gp_map(pin, &n, &fn)) return;
  gp_stop(n);
  pinFunction(pin, 0);
}
