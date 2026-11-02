// Mouse.cpp
#include "Mouse.h"
#include "usb/usb_core.h"
#include "tusb.h"
#include "Arduino.h"
#include "usb_bg.h"
extern "C" const uint8_t usb_has_mouse = 1;   // linking this file turns the HID mouse on
Mouse_ Mouse;

void Mouse_::send(signed char x, signed char y, signed char wheel) {
    UsbGuard g;
    uint32_t t0 = millis();
    while (!tud_hid_ready()) { usb_task(); if (millis() - t0 > 50) return; }
    tud_hid_mouse_report(2 /* REPORT_ID_MOUSE */, _buttons, x, y, wheel, 0);
}
void Mouse_::move(signed char x, signed char y, signed char w) { send(x, y, w); }
void Mouse_::press(uint8_t b)   { _buttons |= b;  send(0, 0, 0); }
void Mouse_::release(uint8_t b) { _buttons &= ~b; send(0, 0, 0); }
void Mouse_::click(uint8_t b)   { press(b); delay(10); release(b); }
bool Mouse_::isPressed(uint8_t b) { return (_buttons & b) != 0; }
