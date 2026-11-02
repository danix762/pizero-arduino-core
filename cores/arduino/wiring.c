#include "Arduino.h"
#include "usb/usb_core.h"
#include "usb_bg.h"

#define GPIO_BASE  0x20200000UL
#define GPFSEL0    (*(volatile unsigned int *)(GPIO_BASE + 0x00))
#define GPSET0     (*(volatile unsigned int *)(GPIO_BASE + 0x1C))
#define GPSET1     (*(volatile unsigned int *)(GPIO_BASE + 0x20))
#define GPCLR0     (*(volatile unsigned int *)(GPIO_BASE + 0x28))
#define GPCLR1     (*(volatile unsigned int *)(GPIO_BASE + 0x2C))
#define GPLEV0     (*(volatile unsigned int *)(GPIO_BASE + 0x34))
#define GPLEV1     (*(volatile unsigned int *)(GPIO_BASE + 0x38))
#define GPPUD      (*(volatile unsigned int *)(GPIO_BASE + 0x94))
#define GPPUDCLK0  (*(volatile unsigned int *)(GPIO_BASE + 0x98))
#define GPPUDCLK1  (*(volatile unsigned int *)(GPIO_BASE + 0x9C))

// System Timer (1 MHz)
#define TIMER_BASE 0x20003000UL
#define TIMER_CLO  (*(volatile unsigned int *)(TIMER_BASE + 0x04))
#define TIMER_CHI  (*(volatile unsigned int *)(TIMER_BASE + 0x08))

// ---------------------------------------------------------------- GPIO

// fn: 0=input 1=output 4=ALT0 5=ALT1 6=ALT2 7=ALT3 3=ALT4 2=ALT5
void pinFunction(uint8_t pin, uint8_t fn) {
    if (pin > 53) return;
    volatile unsigned int *fsel = &GPFSEL0 + (pin / 10);
    unsigned int shift = (pin % 10) * 3;
    unsigned int v = *fsel;
    v &= ~(7u << shift);
    v |= ((unsigned int)fn & 7u) << shift;
    *fsel = v;
}

static void pin_pull(uint8_t pin, unsigned int mode) {   // 0=off 1=down 2=up
    GPPUD = mode;
    delayMicroseconds(5);
    if (pin < 32) GPPUDCLK0 = 1u << pin;
    else          GPPUDCLK1 = 1u << (pin - 32);
    delayMicroseconds(5);
    GPPUD = 0;
    GPPUDCLK0 = 0;
    GPPUDCLK1 = 0;
}

void pinMode(uint8_t pin, uint8_t mode) {
    if (pin > 53) return;
    if (mode == OUTPUT) { pinFunction(pin, 1); return; }
    pinFunction(pin, 0);
    pin_pull(pin, mode == INPUT_PULLUP ? 2 : mode == INPUT_PULLDOWN ? 1 : 0);
}

void digitalWrite(uint8_t pin, uint8_t val) {
    if (pin > 53) return;
    if (pin < 32) {
        if (val) GPSET0 = (1u << pin);
        else     GPCLR0 = (1u << pin);
    } else {
        if (val) GPSET1 = (1u << (pin - 32));
        else     GPCLR1 = (1u << (pin - 32));
    }
}

int digitalRead(uint8_t pin) {
    if (pin > 53) return LOW;
    if (pin < 32) {
        return (GPLEV0 & (1u << pin)) ? HIGH : LOW;
    } else {
        return (GPLEV1 & (1u << (pin - 32))) ? HIGH : LOW;
    }
}

// ---------------------------------------------------------------- Timing

static unsigned long long micros64(void) {
    unsigned int hi, lo;
    do {                              // re-read if CLO rolled over between reads
        hi = TIMER_CHI;
        lo = TIMER_CLO;
    } while (hi != TIMER_CHI);
    return ((unsigned long long)hi << 32) | lo;
}

unsigned long micros(void) {
    return TIMER_CLO;                 // wraps every ~71.6 min, same as on AVR
}

unsigned long millis(void) {
    return (unsigned long)(micros64() / 1000ULL);
}

void delay(unsigned long ms) {
    unsigned long start = millis();
    while (millis() - start < ms) usb_service_main();    // no-op once the ticker owns USB
}

void delayMicroseconds(unsigned int us) {
    unsigned int start = TIMER_CLO;
    while ((unsigned int)(TIMER_CLO - start) < us) { }   // too short to service USB
}
