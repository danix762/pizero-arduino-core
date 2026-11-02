#include <stdint.h>
#include <stdbool.h>
#include "tusb.h"
#include "dwc2.h"
#include "usb_core.h"
#include "../Arduino.h"

#ifndef PCGCCTL
#define PCGCCTL (*(volatile uint32_t *)(USB_BASE + 0xE00))
#endif

#define USB_FIRMWARE_SETTLE_MS 750   // lower this once the mailbox 'wait' fix is proven

static volatile bool usb_ready = false;
static volatile bool in_task   = false;

void usb_start(void) {
    usb_hw_prepare();
    delay(USB_FIRMWARE_SETTLE_MS);   // let the firmware finish its own USB setup
    
    // FIFO setup BEFORE TinyUSB touches the controller
    tusb_rhport_init_t dev_init;
    dev_init.role  = TUSB_ROLE_DEVICE;
    dev_init.speed = TUSB_SPEED_AUTO;
    tusb_init(0, &dev_init);

    usb_ready = true;
}

void usb_task(void) {
    if (!usb_ready || in_task) return;   // not started yet, or called from inside a callback
    in_task = true;
    tud_int_handler(0);
    tud_task();
    in_task = false;
}
