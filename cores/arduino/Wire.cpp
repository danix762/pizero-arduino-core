#include "Wire.h"

#define BSC1        0x20804000UL
#define BSC_C       (*(volatile uint32_t *)(BSC1 + 0x00))
#define BSC_S       (*(volatile uint32_t *)(BSC1 + 0x04))
#define BSC_DLEN    (*(volatile uint32_t *)(BSC1 + 0x08))
#define BSC_A       (*(volatile uint32_t *)(BSC1 + 0x0C))
#define BSC_FIFO    (*(volatile uint32_t *)(BSC1 + 0x10))
#define BSC_DIV     (*(volatile uint32_t *)(BSC1 + 0x14))

#define C_I2CEN     (1u << 15)
#define C_ST        (1u << 7)
#define C_CLEAR     (1u << 4)
#define C_READ      (1u << 0)

#define S_DONE      (1u << 1)
#define S_TXD       (1u << 4)
#define S_RXD       (1u << 5)
#define S_ERR       (1u << 8)
#define S_CLKT      (1u << 9)

#include "mailbox.h"
static uint32_t i2cCoreHz = 400000000UL;       // refreshed in begin()
#define I2C_TIMEOUT_US 50000

TwoWire Wire;

static int bsc_write(uint8_t addr, const uint8_t *d, size_t n) {
  BSC_S = S_CLKT | S_ERR | S_DONE;
  BSC_C = C_I2CEN | C_CLEAR;
  BSC_A = addr;
  BSC_DLEN = n;
  BSC_C = C_I2CEN | C_ST;

  size_t i = 0;
  uint32_t t0 = micros();
  while (!(BSC_S & S_DONE)) {
    while (i < n && (BSC_S & S_TXD)) BSC_FIFO = d[i++];
    if ((uint32_t)(micros() - t0) > I2C_TIMEOUT_US) { BSC_C = C_I2CEN | C_CLEAR; return 4; }
  }
  uint32_t s = BSC_S;
  BSC_S = S_CLKT | S_ERR | S_DONE;
  if (s & S_ERR)  return 2;              // NACK
  if (s & S_CLKT) return 4;              // clock stretch timeout
  return 0;
}

static int bsc_read(uint8_t addr, uint8_t *d, size_t n, size_t *got) {
  BSC_S = S_CLKT | S_ERR | S_DONE;
  BSC_C = C_I2CEN | C_CLEAR;
  BSC_A = addr;
  BSC_DLEN = n;
  BSC_C = C_I2CEN | C_ST | C_READ;

  size_t i = 0;
  uint32_t t0 = micros();
  while (!(BSC_S & S_DONE)) {
    while (i < n && (BSC_S & S_RXD)) d[i++] = (uint8_t)BSC_FIFO;
    if ((uint32_t)(micros() - t0) > I2C_TIMEOUT_US) { BSC_C = C_I2CEN | C_CLEAR; *got = i; return 4; }
  }
  while (i < n && (BSC_S & S_RXD)) d[i++] = (uint8_t)BSC_FIFO;
  uint32_t s = BSC_S;
  BSC_S = S_CLKT | S_ERR | S_DONE;
  *got = i;
  if (s & S_ERR)  return 2;
  if (s & S_CLKT) return 4;
  return 0;
}

void TwoWire::begin() {
  uint32_t c = mbox_get_clock_rate(MBOX_CLK_CORE);
  if (c) i2cCoreHz = c;
  pinFunction(2, 4);
  pinFunction(3, 4);
  BSC_C = C_I2CEN;
  setClock(100000);
}

void TwoWire::setClock(uint32_t hz) {
  BSC_DIV = i2cCoreHz / hz;      // 400 MHz / 100 kHz = 4000, which fits the 16-bit divider
}
void TwoWire::beginTransmission(uint8_t addr) { txAddr = addr; txLen = 0; }

size_t TwoWire::write(uint8_t b) {
  if (txLen >= sizeof(txBuf)) return 0;
  txBuf[txLen++] = b;
  return 1;
}

size_t TwoWire::write(const uint8_t *d, size_t n) {
  size_t k = 0;
  while (k < n && write(d[k])) k++;
  return k;
}

uint8_t TwoWire::endTransmission(bool stop) {
  (void)stop;
  if (txLen == 0) {                      // address probe: 1-byte read
    uint8_t dummy; size_t got;
    return (uint8_t)bsc_read(txAddr, &dummy, 1, &got);
  }
  return (uint8_t)bsc_write(txAddr, txBuf, txLen);
}

uint8_t TwoWire::requestFrom(uint8_t addr, uint8_t qty, bool stop) {
  (void)stop;
  if (qty > sizeof(rxBuf)) qty = sizeof(rxBuf);
  size_t got = 0;
  bsc_read(addr, rxBuf, qty, &got);
  rxLen = (uint8_t)got;
  rxPos = 0;
  return rxLen;
}

int TwoWire::available() { return rxLen - rxPos; }
int TwoWire::read()      { return rxPos < rxLen ? rxBuf[rxPos++] : -1; }
int TwoWire::peek()      { return rxPos < rxLen ? rxBuf[rxPos] : -1; }
