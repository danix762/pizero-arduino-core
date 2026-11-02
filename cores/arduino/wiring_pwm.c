#include "Arduino.h"

#define PERI        0x20000000UL
#define REG(a)      (*(volatile unsigned int *)(a))

#define CM_PWMCTL   REG(PERI + 0x1010A0)
#define CM_PWMDIV   REG(PERI + 0x1010A4)
#define CM_PASS     0x5A000000u

#define PWM_BASE    (PERI + 0x20C000)
#define PWM_CTL     REG(PWM_BASE + 0x00)
#define PWM_RNG1    REG(PWM_BASE + 0x10)
#define PWM_DAT1    REG(PWM_BASE + 0x14)
#define PWM_RNG2    REG(PWM_BASE + 0x20)
#define PWM_DAT2    REG(PWM_BASE + 0x24)

static int pwm_ready = 0;

static void pwm_init(void) {
    PWM_CTL = 0;
    delayMicroseconds(10);

    CM_PWMCTL = CM_PASS | (1u << 5);                       // kill the clock
    for (int i = 0; i < 100000 && (CM_PWMCTL & (1u << 7)); i++) { }

    CM_PWMDIV = CM_PASS | (75u << 12);                     // 19.2 MHz / 75 = 256 kHz
    CM_PWMCTL = CM_PASS | 1u;                              // source = oscillator
    delayMicroseconds(10);
    CM_PWMCTL = CM_PASS | 1u | (1u << 4);                  // enable
    for (int i = 0; i < 100000 && !(CM_PWMCTL & (1u << 7)); i++) { }

    PWM_RNG1 = 256; PWM_DAT1 = 0;                          // 256 kHz / 256 = 1 kHz
    PWM_RNG2 = 256; PWM_DAT2 = 0;
    // PWEN1 | MSEN1 | PWEN2 | MSEN2 (mark/space mode: duty = DAT / RNG)
    PWM_CTL = (1u << 0) | (1u << 7) | (1u << 8) | (1u << 15);
    pwm_ready = 1;
}

void analogWrite(uint8_t pin, int value) {
    if (value < 0) value = 0;
    if (value > 255) value = 255;

    int ch, fn;
    switch (pin) {
        case 12: ch = 0; fn = 4; break;     // ALT0
        case 13: ch = 1; fn = 4; break;     // ALT0
        case 18: ch = 0; fn = 2; break;     // ALT5
        case 19: ch = 1; fn = 2; break;     // ALT5
        default:                            // no hardware PWM: digital approximation
            pinMode(pin, OUTPUT);
            digitalWrite(pin, value >= 128);
            return;
    }

    if (!pwm_ready) pwm_init();
    pinFunction(pin, fn);
    unsigned int dat = (value == 255) ? 256u : (unsigned int)value;   // 255 -> 100%
    if (ch == 0) PWM_DAT1 = dat; else PWM_DAT2 = dat;
}
