// Mouse.h
#pragma once
#include <stdint.h>
#define MOUSE_LEFT   1
#define MOUSE_RIGHT  2
#define MOUSE_MIDDLE 4
class Mouse_ {
public:
    void begin() {}
    void end() {}
    void click(uint8_t b = MOUSE_LEFT);
    void press(uint8_t b = MOUSE_LEFT);
    void release(uint8_t b = MOUSE_LEFT);
    bool isPressed(uint8_t b = MOUSE_LEFT);
    void move(signed char x, signed char y, signed char wheel = 0);
private:
    uint8_t _buttons;       // zero-initialised, no constructor needed
    void send(signed char x, signed char y, signed char wheel);
};
extern Mouse_ Mouse;
