#pragma once
#include <stdint.h>
#include <stddef.h>
#include "Arduino.h"

#ifndef MSBFIRST
#define LSBFIRST 0
#define MSBFIRST 1
#endif

#define SPI_MODE0 0
#define SPI_MODE1 1
#define SPI_MODE2 2
#define SPI_MODE3 3

class SPISettings {
public:
  SPISettings(uint32_t clock = 4000000, uint8_t bitOrder = MSBFIRST, uint8_t dataMode = SPI_MODE0)
    : clock(clock), bitOrder(bitOrder), dataMode(dataMode) {}
  uint32_t clock;
  uint8_t  bitOrder;
  uint8_t  dataMode;
};

class SPIClass {
public:
  void begin();
  void end();
  void beginTransaction(SPISettings s);
  void endTransaction();
  uint8_t  transfer(uint8_t data);
  uint16_t transfer16(uint16_t data);
  void     transfer(void *buf, size_t len);       // in-place
private:
  uint8_t lsb = 0;
};

extern SPIClass SPI;
