#include "Arduino.h"
#include "usb/usb_core.h"
#include "ticker.h"
#include "usb_bg.h"

volatile uint32_t usb_app_depth = 0;
volatile uint32_t usb_bg_runs   = 0;
volatile uint32_t usb_bg_skips  = 0;

#if USB_BG_ENABLE
static void usb_bg_tick(void) {
    if (usb_app_depth) { usb_bg_skips++; return; }   // main is inside a USB call: try again next ms
    usb_bg_runs++;
    usb_task();
}
#endif

void usb_bg_start(void) {
#if USB_BG_ENABLE
    ticker_start(1000, usb_bg_tick);
#endif
}

void usb_service_main(void) {
#if USB_MAIN_SERVICE
    usb_app_depth++;
    usb_task();
    usb_app_depth--;
#endif
}
