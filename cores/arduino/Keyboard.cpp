// Keyboard.cpp
#include "Keyboard.h"
#include "usb/usb_core.h"
#include "tusb.h"
#include "Arduino.h"
#include "usb_bg.h"                 // once, near the top of each file

extern "C" const uint8_t usb_has_keyboard = 1;   // enables Keyboard in the descriptor

Keyboard_ Keyboard;

// Simple ASCII → HID keycode table (US layout)
static uint8_t ascii_to_hid(uint8_t ascii, uint8_t *modifier) {
    *modifier = 0;

    if (ascii >= 'a' && ascii <= 'z') return ascii - 'a' + 0x04;
    if (ascii >= 'A' && ascii <= 'Z') { *modifier = KEY_LEFT_SHIFT; return ascii - 'A' + 0x04; }
    if (ascii >= '1' && ascii <= '9') return ascii - '1' + 0x1E;
    if (ascii == '0') return 0x27;

    switch (ascii) {
        case '\n': return KEY_RETURN;
        case '\r': return KEY_RETURN;
        case '\t': return KEY_TAB;
        case ' ':  return 0x2C;
        case '-':  return 0x2D;
        case '=':  return 0x2E;
        case '[':  return 0x2F;
        case ']':  return 0x30;
        case '\\': return 0x31;
        case ';':  return 0x33;
        case '\'': return 0x34;
        case '`':  return 0x35;
        case ',':  return 0x36;
        case '.':  return 0x37;
        case '/':  return 0x38;
        // Shifted symbols
        case '!': *modifier = KEY_LEFT_SHIFT; return 0x1E;
        case '@': *modifier = KEY_LEFT_SHIFT; return 0x1F;
        case '#': *modifier = KEY_LEFT_SHIFT; return 0x20;
        case '$': *modifier = KEY_LEFT_SHIFT; return 0x21;
        case '%': *modifier = KEY_LEFT_SHIFT; return 0x22;
        case '^': *modifier = KEY_LEFT_SHIFT; return 0x23;
        case '&': *modifier = KEY_LEFT_SHIFT; return 0x24;
        case '*': *modifier = KEY_LEFT_SHIFT; return 0x25;
        case '(': *modifier = KEY_LEFT_SHIFT; return 0x26;
        case ')': *modifier = KEY_LEFT_SHIFT; return 0x27;
        case '_': *modifier = KEY_LEFT_SHIFT; return 0x2D;
        case '+': *modifier = KEY_LEFT_SHIFT; return 0x2E;
        case '{': *modifier = KEY_LEFT_SHIFT; return 0x2F;
        case '}': *modifier = KEY_LEFT_SHIFT; return 0x30;
        case '|': *modifier = KEY_LEFT_SHIFT; return 0x31;
        case ':': *modifier = KEY_LEFT_SHIFT; return 0x33;
        case '"': *modifier = KEY_LEFT_SHIFT; return 0x34;
        case '~': *modifier = KEY_LEFT_SHIFT; return 0x35;
        case '<': *modifier = KEY_LEFT_SHIFT; return 0x36;
        case '>': *modifier = KEY_LEFT_SHIFT; return 0x37;
        case '?': *modifier = KEY_LEFT_SHIFT; return 0x38;
    }
    return 0;
}

void Keyboard_::sendReport() {
    UsbGuard g;
    uint32_t t0 = millis();
    while (!tud_hid_ready()) {
        usb_task();
        if (millis() - t0 > 50) return;
    }
    tud_hid_keyboard_report(1 /* REPORT_ID_KEYBOARD */, _modifiers, _keys);
}

void Keyboard_::press(uint8_t k) {
    UsbGuard g;
    if (k >= 0x80 && k <= 0x87) {          // modifier
        _modifiers |= (1 << (k - 0x80));
    } else {
        // find empty slot
        for (int i = 0; i < 6; i++) {
            if (_keys[i] == 0) {
                _keys[i] = k;
                break;
            }
        }
    }
    sendReport();
}

void Keyboard_::release(uint8_t k) {
    UsbGuard g;
    if (k >= 0x80 && k <= 0x87) {
        _modifiers &= ~(1 << (k - 0x80));
    } else {
        for (int i = 0; i < 6; i++) {
            if (_keys[i] == k) _keys[i] = 0;
        }
    }
    sendReport();
}

void Keyboard_::releaseAll() {
    UsbGuard g;
    memset(_keys, 0, 6);
    _modifiers = 0;
    sendReport();
}
static void wait_ms(uint32_t ms) {
    uint32_t t = millis();
    while (millis() - t < ms) usb_task();
}
size_t Keyboard_::write(uint8_t c) {
    UsbGuard g;
    uint8_t mod = 0;
    uint8_t key = ascii_to_hid(c, &mod);

    if (key == 0) return 0;

    uint8_t old_mod = _modifiers;
    _modifiers |= mod;
    press(key);
    wait_ms(5);
    release(key);
    _modifiers = old_mod;
    sendReport();
    return 1;
}

size_t Keyboard_::write(const char *str) {
    UsbGuard g;
    size_t n = 0;
    while (*str) n += write((uint8_t)*str++);
    return n;
}

size_t Keyboard_::write(const uint8_t *buffer, size_t size) {
    UsbGuard g;
    size_t n = 0;
    for (size_t i = 0; i < size; i++) n += write(buffer[i]);
    return n;
}
