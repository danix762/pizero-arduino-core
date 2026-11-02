#pragma once
#include <stdint.h>

// 0 = USB is serviced only from delay()/loop() (the original behaviour)
// 1 = USB is also serviced by a 1 kHz timer interrupt.
//     Set to 1 ONLY after "UsbGuard g;" is the first line of every public USB method
//     (USBSerial, Keyboard, Mouse).
#define USB_BG_ENABLE     0

// 1 = delay() and the main loop also service USB.
// Set to 0 once the background service is proven, so only the timer services USB.
#define USB_MAIN_SERVICE  1

#ifdef __cplusplus
extern "C" {
#endif

extern volatile uint32_t usb_app_depth;   // >0 while main code is inside a USB call
extern volatile uint32_t usb_bg_runs;     // diagnostics: ticks that serviced USB
extern volatile uint32_t usb_bg_skips;    // diagnostics: ticks skipped because main was in USB

void usb_bg_start(void);
void usb_service_main(void);              // guarded usb_task() for main-context loops

#ifdef __cplusplus
}

// Put "UsbGuard g;" as the first line of every public USB method.
struct UsbGuard {
  UsbGuard()  { usb_app_depth = usb_app_depth + 1; }
  ~UsbGuard() { usb_app_depth = usb_app_depth - 1; }
};
#endif
