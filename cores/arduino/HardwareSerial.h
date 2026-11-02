#pragma once
#include <stdint.h>
#include <stddef.h>
#include "Print.h"

// Config constants use the same bit layout as AVR Arduino:
// bits 2:1 = data bits - 5, bit 3 = 2 stop bits, bits 5:4 = parity (0 none, 2 even, 3 odd)
#define SERIAL_8N1 0x06
#define SERIAL_8N2 0x0E
#define SERIAL_7N1 0x04
#define SERIAL_6N1 0x02
#define SERIAL_5N1 0x00
#define SERIAL_8E1 0x26
#define SERIAL_8O1 0x36
#define SERIAL_7E1 0x24
#define SERIAL_7O1 0x34

class HardwareSerial : public Print {
public:
  void begin(unsigned long baud, uint8_t config = SERIAL_8N1);
  void end();
  int  available(void);
  int  peek(void);
  int  read(void);
  void flush(void);                 // waits until everything has been transmitted
  size_t write(uint8_t c);
  using Print::write;
  operator bool() { return true; }
};

extern HardwareSerial Serial1;      // PL011 UART0: GPIO14 = TX (pin 8), GPIO15 = RX (pin 10)
