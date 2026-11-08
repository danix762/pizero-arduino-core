#pragma once
#include <stdint.h>
#include <stddef.h>

// Extras for the Raspberry Pi Zero (BCM2835). Use the global object BCM.
class BCM2835Extras {
public:
  // ---- hardware random number generator (not for cryptography)
  void     randomBegin();                       // optional, called automatically
  bool     randomRead(uint32_t *out);           // false if the hardware does not answer
  uint32_t random32();                          // falls back to a software generator if the hardware is silent
  void     randomBytes(uint8_t *buf, size_t len);

  // ---- watchdog and reset
  void     reset();                             // immediate full reset (the board reboots from the SD card)
  void     watchdogStart(uint32_t ms);          // reboot unless watchdogFeed() is called within ms (max ~15999)
  void     watchdogFeed();
  void     watchdogStop();
  uint32_t watchdogRemainingMs();

  // ---- firmware information (mailbox). 0 / very negative values mean "firmware did not answer"
  uint32_t    firmwareRevision();
  uint32_t    boardRevision();
  const char *boardName();
  uint64_t    serialNumber();
  uint32_t    armMemoryBytes();
  uint32_t    gpuMemoryBytes();
  int32_t     temperatureMilliC();              // INT32_MIN on failure
  float       temperatureC();
  uint32_t    clockHz(uint32_t id);             // 1 EMMC, 2 UART, 3 ARM, 4 CORE
  uint32_t    setClockHz(uint32_t id, uint32_t hz);  // returns the rate the firmware now reports.
                                                     // Changing CORE, EMMC or UART clocks while SPI/I2C/SD/UART are in use will break them.

  // ---- general-purpose clock output, 4.7 kHz to 50 MHz on GPIO 4 (clock 0), 20 (clock 0),
  //      5 (clock 1) or 6 (clock 2). Returns the real frequency, 0 on error.
  uint32_t gpclkStart(uint8_t pin, uint32_t hz);
  void     gpclkStop(uint8_t pin);
};

extern BCM2835Extras BCM;
