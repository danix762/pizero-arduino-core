#pragma once
#include <stdint.h>

// Defined (as 1) by Keyboard.cpp / Mouse.cpp. Only exist if the sketch links those files.
extern const uint8_t usb_has_keyboard __attribute__((weak));
extern const uint8_t usb_has_mouse    __attribute__((weak));

#define USB_HAS_KEYBOARD (&usb_has_keyboard != 0)
#define USB_HAS_MOUSE    (&usb_has_mouse != 0)
