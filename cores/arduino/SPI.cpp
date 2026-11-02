#include "SPI.h"

#define SPI0_BASE   0x20204000UL
#define SPI0_CS     (*(volatile uint32_t *)(SPI0_BASE + 0x00))
#define SPI0_FIFO   (*(volatile uint32_t *)(SPI0_BASE + 0x04))
#define SPI0_CLK    (*(volatile uint32_t *)(SPI0_BASE + 0x08))

#define CS_CLEAR    (3u << 4)
#define CS_TA       (1u << 7)
#define CS_DONE     (1u << 16)
#define CS_RXD      (1u << 17)
#define CS_TXD      (1u << 18)

#include "mailbox.h"
static uint32_t spiCoreHz = 400000000UL;       // refreshed in begin()

SPIClass SPI;

static uint8_t rev8(uint8_t b) {
  b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
  b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
  b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
  return b;
}

static void spi_configure(const SPISettings &s) {
  uint32_t div = (spiCoreHz + s.clock - 1) / s.clock;
  div = (div + 1) & ~1u;                 // must be even
  if (div < 2) div = 2;
  if (div > 65534) div = 0;              // 0 = 65536
  SPI0_CLK = div;
  SPI0_CS  = ((uint32_t)(s.dataMode & 3) << 2) | CS_CLEAR;   // CPHA = bit 2, CPOL = bit 3
}

void SPIClass::begin() {
  pinFunction(9, 4);                     // MISO  ALT0
  pinFunction(10, 4);                    // MOSI  ALT0
  pinFunction(11, 4);                    // SCLK  ALT0
  lsb = 0;
  uint32_t c = mbox_get_clock_rate(MBOX_CLK_CORE);
  if (c) spiCoreHz = c;
  spi_configure(SPISettings());
}

void SPIClass::end() {
  SPI0_CS = CS_CLEAR;
  pinFunction(9, 0); pinFunction(10, 0); pinFunction(11, 0);
}

void SPIClass::beginTransaction(SPISettings s) {
  lsb = (s.bitOrder == LSBFIRST);
  spi_configure(s);
  SPI0_CS |= CS_TA;
}

void SPIClass::endTransaction() {
  while (!(SPI0_CS & CS_DONE)) { }
  SPI0_CS &= ~CS_TA;
}

uint8_t SPIClass::transfer(uint8_t data) {
  if (!(SPI0_CS & CS_TA)) SPI0_CS |= CS_TA;        // allow use without beginTransaction
  if (lsb) data = rev8(data);
  while (!(SPI0_CS & CS_TXD)) { }
  SPI0_FIFO = data;
  while (!(SPI0_CS & CS_DONE)) { }
  while (!(SPI0_CS & CS_RXD)) { }
  uint8_t r = (uint8_t)SPI0_FIFO;
  return lsb ? rev8(r) : r;
}

uint16_t SPIClass::transfer16(uint16_t data) {
  uint16_t hi, lo;
  if (lsb) {
    lo = transfer(data & 0xFF);
    hi = transfer(data >> 8);
  } else {
    hi = transfer(data >> 8);
    lo = transfer(data & 0xFF);
  }
  return (hi << 8) | lo;
}

void SPIClass::transfer(void *buf, size_t len) {
  uint8_t *p = (uint8_t *)buf;
  for (size_t i = 0; i < len; i++) p[i] = transfer(p[i]);
}
