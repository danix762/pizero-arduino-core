#include "Arduino.h"
#include "HardwareSerial.h"
#include "irq.h"
#include "mailbox.h"

#define UART0_BASE  0x20201000UL
#define UART_DR     (*(volatile uint32_t *)(UART0_BASE + 0x00))
#define UART_FR     (*(volatile uint32_t *)(UART0_BASE + 0x18))
#define UART_IBRD   (*(volatile uint32_t *)(UART0_BASE + 0x24))
#define UART_FBRD   (*(volatile uint32_t *)(UART0_BASE + 0x28))
#define UART_LCRH   (*(volatile uint32_t *)(UART0_BASE + 0x2C))
#define UART_CR     (*(volatile uint32_t *)(UART0_BASE + 0x30))
#define UART_IFLS   (*(volatile uint32_t *)(UART0_BASE + 0x34))
#define UART_IMSC   (*(volatile uint32_t *)(UART0_BASE + 0x38))
#define UART_ICR    (*(volatile uint32_t *)(UART0_BASE + 0x44))

#define FR_BUSY     (1u << 3)
#define FR_RXFE     (1u << 4)
#define FR_TXFF     (1u << 5)

#define LCRH_PEN    (1u << 1)
#define LCRH_EPS    (1u << 2)
#define LCRH_STP2   (1u << 3)
#define LCRH_FEN    (1u << 4)

#define CR_UARTEN   (1u << 0)
#define CR_TXE      (1u << 8)
#define CR_RXE      (1u << 9)

#define INT_RX      (1u << 4)
#define INT_RT      (1u << 6)       // receive timeout: flushes partial FIFO contents

#define IRQ_UART    57

#define RX_SIZE     256             // must be a power of two
static volatile uint8_t  rxBuf[RX_SIZE];
static volatile uint16_t rxHead = 0, rxTail = 0;

HardwareSerial Serial1;

static void uart0_isr(void) {
  while (!(UART_FR & FR_RXFE)) {
    uint8_t c = (uint8_t)UART_DR;               // error flags in the upper bits are ignored
    uint16_t next = (rxHead + 1) & (RX_SIZE - 1);
    if (next != rxTail) { rxBuf[rxHead] = c; rxHead = next; }   // drop if the buffer is full
  }
  UART_ICR = 0x7FF;                              // clear all UART interrupt flags
}

void HardwareSerial::begin(unsigned long baud, uint8_t config) {
  if (baud == 0) return;

  uint32_t clk = mbox_get_clock_rate(MBOX_CLK_UART);   // normally 48 MHz
  if (clk == 0) clk = 48000000UL;                      // firmware call failed: assume the default

  uint32_t div16 = 16UL * (uint32_t)baud;
  uint32_t ibrd = clk / div16;
  uint32_t fbrd = ((clk % div16) * 64 + div16 / 2) / div16;
  if (fbrd >= 64) { ibrd++; fbrd = 0; }
  if (ibrd == 0 || ibrd > 65535) return;               // baud rate not reachable

  // Let any previous transmission finish, then disable
  uint32_t t0 = micros();
  while ((UART_FR & FR_BUSY) && (uint32_t)(micros() - t0) < 100000) { }
  UART_CR = 0;

  // GPIO14 = TXD0, GPIO15 = RXD0 (ALT0). RX gets a pull-up so it idles high if unconnected.
  pinMode(14, INPUT);
  pinMode(15, INPUT_PULLUP);
  pinFunction(14, 4);
  pinFunction(15, 4);

  UART_ICR  = 0x7FF;
  UART_IBRD = ibrd;
  UART_FBRD = fbrd;

  uint32_t lcrh = LCRH_FEN | (((uint32_t)(config >> 1) & 3u) << 5);   // word length
  if (config & 0x08) lcrh |= LCRH_STP2;
  switch ((config >> 4) & 3u) {
    case 2: lcrh |= LCRH_PEN | LCRH_EPS; break;     // even
    case 3: lcrh |= LCRH_PEN;            break;     // odd
  }
  UART_LCRH = lcrh;                                  // must be written after IBRD/FBRD

  UART_IFLS = 0;                                     // RX/TX FIFO interrupt at 1/8 full
  while (!(UART_FR & FR_RXFE)) (void)UART_DR;        // drain stale RX data
  rxHead = rxTail = 0;

  irq_register(IRQ_UART, uart0_isr);
  UART_IMSC = INT_RX | INT_RT;
  irq_enable(IRQ_UART);

  UART_CR = CR_UARTEN | CR_TXE | CR_RXE;
}

void HardwareSerial::end() {
  flush();
  UART_IMSC = 0;
  irq_disable(IRQ_UART);
  UART_CR = 0;
}

int HardwareSerial::available(void) {
  return (rxHead - rxTail) & (RX_SIZE - 1);
}

int HardwareSerial::peek(void) {
  return (rxHead == rxTail) ? -1 : rxBuf[rxTail];
}

int HardwareSerial::read(void) {
  if (rxHead == rxTail) return -1;
  uint8_t c = rxBuf[rxTail];
  rxTail = (rxTail + 1) & (RX_SIZE - 1);
  return c;
}

void HardwareSerial::flush(void) {
  uint32_t t0 = micros();
  while ((UART_FR & FR_BUSY) && (uint32_t)(micros() - t0) < 1000000) { }
}

size_t HardwareSerial::write(uint8_t c) {
  while (UART_FR & FR_TXFF) { }                      // wait for FIFO space
  UART_DR = c;
  return 1;
}
