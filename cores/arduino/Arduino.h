#ifndef ARDUINO_H
#define ARDUINO_H

#include <stdint.h>
#include <stdbool.h>

#include "pins_arduino.h"
#include "irq.h"
#ifdef __cplusplus
extern "C" {
#endif

#define HIGH 0x1
#define LOW  0x0

#define INPUT          0x0
#define OUTPUT         0x1
#define INPUT_PULLUP   0x2
#define INPUT_PULLDOWN 0x3
#ifndef B00000001
#define B00000001 1
#define B00000010 2
#define B00000100 4
#define B00001000 8
#define B00010000 16
#define B00100000 32
#define B01000000 64
#define B10000000 128
#endif
#define RISING 1
#define FALLING 2
#define CHANGE 3

void attachInterrupt(uint8_t pin, void (*userFunc)(void), int mode);
void detachInterrupt(uint8_t pin);

#define digitalPinToInterrupt(p) (p)

void pinMode(uint8_t pin, uint8_t mode);
void pinFunction(uint8_t pin, uint8_t fn);
void digitalWrite(uint8_t pin, uint8_t val);
int  digitalRead(uint8_t pin);
void analogWrite(uint8_t pin, int value);

unsigned long millis(void);
unsigned long micros(void);
void delayMicroseconds(unsigned int us);
void delay(unsigned long ms);

#ifdef __cplusplus
} // extern "C"
#endif

void setup(void);
void loop(void);

#ifdef __cplusplus
#include "USBSerial.h"
#include "WString.h"  // <-- Add this line so String is globally available
#include "HardwareSerial.h"
#endif

#endif
