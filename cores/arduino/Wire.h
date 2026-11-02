#pragma once
#include <stdint.h>
#include <stddef.h>
#include "Arduino.h"

class TwoWire {
public:
  void begin();
  void setClock(uint32_t hz);
  void beginTransmission(uint8_t addr);
  size_t write(uint8_t b);
  size_t write(const uint8_t *d, size_t n);
  uint8_t endTransmission(bool stop = true);
  uint8_t requestFrom(uint8_t addr, uint8_t qty, bool stop = true);
  int available();
  int read();
  int peek();
private:
  uint8_t txAddr = 0, txLen = 0, rxLen = 0, rxPos = 0;
  uint8_t txBuf[32], rxBuf[32];
};

extern TwoWire Wire;
