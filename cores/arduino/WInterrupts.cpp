#include "Arduino.h"

#define GPIO_BASE 0x20200000UL
#define GPEDS0    (*(volatile uint32_t *)(GPIO_BASE + 0x40))   // event detect status
#define GPREN0    (*(volatile uint32_t *)(GPIO_BASE + 0x4C))   // rising edge enable
#define GPFEN0    (*(volatile uint32_t *)(GPIO_BASE + 0x58))   // falling edge enable

static void (*interruptHandlers[32])(void) = { NULL };

void attachInterrupt(uint8_t pin, void (*userFunc)(void), int mode) {
  if (pin >= 32) return;
  uint32_t bit = 1u << pin;

  interruptHandlers[pin] = userFunc;

  GPREN0 &= ~bit;                       // disarm while changing mode
  GPFEN0 &= ~bit;
  GPEDS0 = bit;                         // discard any stale event

  if (mode == RISING  || mode == CHANGE) GPREN0 |= bit;
  if (mode == FALLING || mode == CHANGE) GPFEN0 |= bit;
}

void detachInterrupt(uint8_t pin) {
  if (pin >= 32) return;
  uint32_t bit = 1u << pin;
  GPREN0 &= ~bit;
  GPFEN0 &= ~bit;
  GPEDS0 = bit;
  interruptHandlers[pin] = NULL;
}

// Called from the IRQ dispatcher (or by polling) when GPIO events are pending
extern "C" void handle_gpio_interrupt() {
  uint32_t pending = GPEDS0;
  while (pending) {
    int pin = __builtin_ctz(pending);
    pending &= pending - 1;
    GPEDS0 = 1u << pin;                 // clear the event first (write 1 to clear)
    if (interruptHandlers[pin]) interruptHandlers[pin]();
  }
}
