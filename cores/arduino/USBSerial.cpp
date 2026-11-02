#include "USBSerial.h"
#include "usb/usb_core.h"
#include "tusb.h"
#include "Arduino.h"
#include "usb_bg.h"

USBSerial Serial;     // no constructor, so no startup code is needed

int USBSerial::available() {UsbGuard g; usb_task(); return (int) tud_cdc_available(); }

int USBSerial::read() {
    UsbGuard g;
    usb_task();
    return tud_cdc_available() ? (int) tud_cdc_read_char() : -1;
}

int USBSerial::peek() {
    UsbGuard g;
    uint8_t c;
    return tud_cdc_peek(&c) ? (int) c : -1;
}

int USBSerial::availableForWrite() { UsbGuard g; return (int) tud_cdc_write_available(); }

USBSerial::operator bool() { UsbGuard g; usb_task(); return tud_cdc_connected(); }

void USBSerial::flush() {
    UsbGuard g;
    tud_cdc_write_flush();
    uint32_t t0 = millis();
    while (tud_cdc_write_available() < CFG_TUD_CDC_TX_BUFSIZE && (millis() - t0) < 50) usb_task();
}

size_t USBSerial::write(const uint8_t *buf, size_t n) {
    UsbGuard g;
    if (!tud_cdc_connected()) return n;           // nobody has the port open: drop, don't block
    size_t sent = 0;
    uint32_t t0 = millis();
    while (sent < n) {
        uint32_t w = tud_cdc_write(buf + sent, n - sent);
        sent += w;
        if (w == 0) {
            usb_task();
            if (millis() - t0 > 50) break;        // host isn't reading: give up
        } else {
            t0 = millis();
        }
    }
    tud_cdc_write_flush();
    return sent;
}

size_t USBSerial::write(uint8_t c) {UsbGuard g; return write(&c, 1); }

size_t USBSerial::write(const char *s) {
    UsbGuard g;
    size_t n = 0;
    while (s[n]) n++;
    return write((const uint8_t *) s, n);
}

size_t USBSerial::print(const char *s) {UsbGuard g; return write(s); }
size_t USBSerial::print(char c)        {UsbGuard g; return write((uint8_t) c); }

size_t USBSerial::print(unsigned long v, int base) {
    UsbGuard g;
    char buf[33];
    int i = 32;
    buf[i] = 0;
    if (base < 2) base = 10;
    do {
        unsigned d = v % base;
        buf[--i] = (d < 10) ? ('0' + d) : ('A' + d - 10);
        v /= base;
    } while (v && i > 0);
    return write(buf + i);
}

size_t USBSerial::print(long v, int base) {
    UsbGuard g;
    if (v < 0 && base == 10) {
        write((uint8_t) '-');
        return 1 + print((unsigned long) (-v), 10);
    }
    return print((unsigned long) v, base);
}

size_t USBSerial::print(double v, int digits) {
    UsbGuard g;
    size_t n = 0;
    if (v < 0) { n += write((uint8_t) '-'); v = -v; }
    double rounding = 0.5;
    for (int i = 0; i < digits; i++) rounding /= 10.0;
    v += rounding;
    unsigned long ip = (unsigned long) v;
    n += print(ip);
    if (digits > 0) {
        n += write((uint8_t) '.');
        double rem = v - (double) ip;
        while (digits-- > 0) {
            rem *= 10.0;
            int d = (int) rem;
            n += write((uint8_t) ('0' + d));
            rem -= d;
        }
    }
    return n;
}
