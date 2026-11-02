#pragma once
#include <stdint.h>
#include <stddef.h>

class USBSerial {
public:
    void   begin(unsigned long baud = 115200) { (void) baud; }   // baud is meaningless over USB
    void   end() {}
    int    available();
    int    read();
    int    peek();
    void   flush();
    int    availableForWrite();
    operator bool();

    size_t write(uint8_t c);
    size_t write(const uint8_t *buf, size_t n);
    size_t write(const char *s);

    size_t print(const char *s);
    size_t print(char c);
    size_t print(long v, int base = 10);
    size_t print(int v, int base = 10)            { return print((long) v, base); }
    size_t print(unsigned long v, int base = 10);
    size_t print(unsigned int v, int base = 10)   { return print((unsigned long) v, base); }
    size_t print(double v, int digits = 2);

    size_t println() { return write("\r\n"); }
    template <typename T> size_t println(T v)             { size_t n = print(v);    return n + println(); }
    template <typename T, typename U> size_t println(T v, U x) { size_t n = print(v, x); return n + println(); }
};

extern USBSerial Serial;
