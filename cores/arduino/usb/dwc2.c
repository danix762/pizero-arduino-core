#include "dwc2.h"
#include "../Arduino.h"

#define MBOX_BASE   0x2000B880u
#define MBOX_READ   (*(volatile uint32_t *)(MBOX_BASE + 0x00))
#define MBOX_STATUS (*(volatile uint32_t *)(MBOX_BASE + 0x18))
#define MBOX_WRITE  (*(volatile uint32_t *)(MBOX_BASE + 0x20))
#define MBOX_FULL   0x80000000u
#define MBOX_EMPTY  0x40000000u

#define PCGCCTL     (*(volatile uint32_t *)(USB_BASE + 0xE00))
#define BUS_ADDRESS(addr) (((uint32_t)(addr) & ~0xC0000000u) | 0xC0000000u)

static volatile uint32_t __attribute__((aligned(16), section(".data"))) mbox_buf[8];

static void mbox_call(uint8_t channel) {
    uint32_t bus_addr = BUS_ADDRESS((uint32_t)(uintptr_t)mbox_buf);
    uint32_t addr = (bus_addr & ~0xF) | (channel & 0xF);

    unsigned int timeout = 1000000;
    while ((MBOX_STATUS & MBOX_FULL) && --timeout);
    if (timeout == 0) return;

    MBOX_WRITE = addr;

    timeout = 1000000;
    while (timeout--) {
        if (!(MBOX_STATUS & MBOX_EMPTY)) {
            uint32_t resp = MBOX_READ;
            if ((resp & 0xF) == channel && (resp & ~0xFu) == (addr & ~0xFu))
                return;
        }
    }
}

static void usb_power_on(void) {
    mbox_buf[0] = 8 * 4;
    mbox_buf[1] = 0;
    mbox_buf[2] = 0x00028001;
    mbox_buf[3] = 8;
    mbox_buf[4] = 8;
    mbox_buf[5] = 3;
    mbox_buf[6] = 1;
    mbox_buf[7] = 0;
    mbox_call(8);
}

static void usb_delay_ms(unsigned int ms) {
    unsigned long start = millis();
    while (millis() - start < ms);
}

void usb_hw_prepare(void) {
    usb_power_on();
    usb_delay_ms(10);
    PCGCCTL = 0;
    usb_delay_ms(1);
}
