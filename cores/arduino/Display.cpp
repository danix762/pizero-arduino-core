// Display.cpp
#include "Display.h"
#include "dbg_font.h"          // your existing 8x16 font
#include <stddef.h>
#include "cache.h"
// Mailbox registers (same as your dbg_console)
#define MBOX_BASE   0x2000B880u
#define MBOX_READ   (*(volatile uint32_t *)(MBOX_BASE + 0x00))
#define MBOX_STATUS (*(volatile uint32_t *)(MBOX_BASE + 0x18))
#define MBOX_WRITE  (*(volatile uint32_t *)(MBOX_BASE + 0x20))
#define MBOX_FULL   0x80000000u
#define MBOX_EMPTY  0x40000000u

static volatile uint32_t fbmsg[32] __attribute__((aligned(32), section(".data")));

static int mbox_call8(void) {
    uint32_t bus = (((uint32_t)(uintptr_t)fbmsg) & ~0xC0000000u) | 0xC0000000u;
    uint32_t msg = (bus & ~0xFu) | 8u;

    dcache_clean_range((const void*)fbmsg, sizeof(fbmsg));        // push request out to RAM

    unsigned t = 2000000;
    while ((MBOX_STATUS & MBOX_FULL) && --t);
    if (!t) return 0;
    MBOX_WRITE = msg;
    t = 2000000;
    while (--t) {
        if (!(MBOX_STATUS & MBOX_EMPTY)) {
            if (MBOX_READ == msg) {
                dcache_invalidate_range((void*)fbmsg, sizeof(fbmsg));   // drop stale cached copy
                return fbmsg[1] == 0x80000000u;
            }
        }
    }
    return 0;
}

// ---------------------------------------------------------------
// Display implementation
// ---------------------------------------------------------------

bool Display::begin(uint16_t width, uint16_t height) {
    unsigned i = 0;
    fbmsg[i++] = 0;            // size
    fbmsg[i++] = 0;            // request
    fbmsg[i++] = 0x00048003; fbmsg[i++] = 8; fbmsg[i++] = 0; fbmsg[i++] = width;  fbmsg[i++] = height; // physical
    fbmsg[i++] = 0x00048004; fbmsg[i++] = 8; fbmsg[i++] = 0; fbmsg[i++] = width;  fbmsg[i++] = height; // virtual
    fbmsg[i++] = 0x00048005; fbmsg[i++] = 4; fbmsg[i++] = 0; fbmsg[i++] = 32;                        // depth
    fbmsg[i++] = 0x00040001; fbmsg[i++] = 8; fbmsg[i++] = 0; fbmsg[i++] = 16; fbmsg[i++] = 0;       // allocate
    fbmsg[i++] = 0x00040008; fbmsg[i++] = 4; fbmsg[i++] = 0; fbmsg[i++] = 0;                         // pitch
    fbmsg[i++] = 0;
    fbmsg[0] = i * 4;

    if (!mbox_call8()) return false;

    uint32_t base = fbmsg[19] & 0x3FFFFFFFu;
    if (!base) return false;

    _fb     = (uint32_t *)(uintptr_t)base;
    _pitch  = fbmsg[24];
    _width  = fbmsg[5];
    _height = fbmsg[6];

    _cursor_x = _cursor_y = 0;
    _text_fg  = 0xFFFFFFFFu;
    _text_bg  = 0x00000000u;
    _text_size = 1;
    _text_wrap = true;
    _rotation  = 0;

    fillScreen(0x00000000);
    return true;
}

void Display::end() {
    _fb = nullptr;
}

uint16_t Display::width()  const { return _width;  }
uint16_t Display::height() const { return _height; }
uint32_t Display::getPitch() const { return _pitch; }

uint32_t*       Display::getFramebuffer()       { return _fb; }
const uint32_t* Display::getFramebuffer() const { return _fb; }

void Display::fillScreen(uint32_t color) {
    if (!_fb) return;

    uint32_t *p = _fb;
    uint32_t count = (_pitch / 4) * _height;

    // Unrolled 32-bit fill (much faster than per-pixel)
    while (count >= 8) {
        p[0] = color; p[1] = color; p[2] = color; p[3] = color;
        p[4] = color; p[5] = color; p[6] = color; p[7] = color;
        p += 8;
        count -= 8;
    }
    while (count--) {
        *p++ = color;
    }
}

void Display::drawPixel(int16_t x, int16_t y, uint32_t color) {
    if (!_fb || x < 0 || y < 0 || x >= (int16_t)_width || y >= (int16_t)_height) return;
    _fb[y * (_pitch / 4) + x] = color;
}

// Fast horizontal / vertical lines
void Display::drawFastHLine(int16_t x, int16_t y, int16_t w, uint32_t color) {
    if (!_fb || y < 0 || y >= (int16_t)_height || w <= 0) return;

    if (x < 0) { w += x; x = 0; }
    if (x + w > (int16_t)_width) w = _width - x;
    if (w <= 0) return;

    uint32_t *p = _fb + y * (_pitch / 4) + x;

    while (w >= 8) {
        p[0] = color; p[1] = color; p[2] = color; p[3] = color;
        p[4] = color; p[5] = color; p[6] = color; p[7] = color;
        p += 8;
        w -= 8;
    }
    while (w--) {
        *p++ = color;
    }
}

void Display::drawFastVLine(int16_t x, int16_t y, int16_t h, uint32_t color) {
    if (!_fb || x < 0 || x >= (int16_t)_width) return;
    if (y < 0) { h += y; y = 0; }
    if (y + h > (int16_t)_height) h = _height - y;
    if (h <= 0) return;

    uint32_t pitch_words = _pitch / 4;
    uint32_t *p = _fb + y * pitch_words + x;
    for (int16_t i = 0; i < h; i++) {
        *p = color;
        p += pitch_words;
    }
}

void Display::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint32_t color) {
    int16_t dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    int16_t sx = (x0 < x1) ? 1 : -1;
    int16_t dy = (y1 > y0) ? (y0 - y1) : (y1 - y0);
    int16_t sy = (y0 < y1) ? 1 : -1;
    int16_t err = dx + dy;

    for (;;) {
        drawPixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int16_t e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void Display::drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint32_t color) {
    drawFastHLine(x, y, w, color);
    drawFastHLine(x, y + h - 1, w, color);
    drawFastVLine(x, y, h, color);
    drawFastVLine(x + w - 1, y, h, color);
}

void Display::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint32_t color) {
    if (!_fb || w <= 0 || h <= 0) return;

    // Clip
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > (int16_t)_width)  w = _width  - x;
    if (y + h > (int16_t)_height) h = _height - y;
    if (w <= 0 || h <= 0) return;

    uint32_t pitch_words = _pitch / 4;
    uint32_t *row = _fb + y * pitch_words + x;

    for (int16_t i = 0; i < h; i++) {
        uint32_t *p = row;
        int16_t remaining = w;

        while (remaining >= 8) {
            p[0] = color; p[1] = color; p[2] = color; p[3] = color;
            p[4] = color; p[5] = color; p[6] = color; p[7] = color;
            p += 8;
            remaining -= 8;
        }
        while (remaining--) {
            *p++ = color;
        }
        row += pitch_words;
    }
}

// Very simple filled circle
void Display::fillCircle(int16_t x0, int16_t y0, int16_t r, uint32_t color) {
    if (r <= 0) return;

    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;

    // Center vertical line
    drawFastVLine(x0, y0 - r, 2 * r + 1, color);

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        // Draw horizontal spans
        drawFastHLine(x0 - x, y0 + y, 2 * x + 1, color);
        drawFastHLine(x0 - x, y0 - y, 2 * x + 1, color);
        drawFastHLine(x0 - y, y0 + x, 2 * y + 1, color);
        drawFastHLine(x0 - y, y0 - x, 2 * y + 1, color);
    }
}

void Display::drawCircle(int16_t x0, int16_t y0, int16_t r, uint32_t color) {
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;

    drawPixel(x0, y0 + r, color);
    drawPixel(x0, y0 - r, color);
    drawPixel(x0 + r, y0, color);
    drawPixel(x0 - r, y0, color);

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        drawPixel(x0 + x, y0 + y, color);
        drawPixel(x0 - x, y0 + y, color);
        drawPixel(x0 + x, y0 - y, color);
        drawPixel(x0 - x, y0 - y, color);
        drawPixel(x0 + y, y0 + x, color);
        drawPixel(x0 - y, y0 + x, color);
        drawPixel(x0 + y, y0 - x, color);
        drawPixel(x0 - y, y0 - x, color);
    }
}

// ---------------- Text ----------------

void Display::setCursor(int16_t x, int16_t y) {
    _cursor_x = x;
    _cursor_y = y;
}

void Display::setTextColor(uint32_t fg) {
    _text_fg = fg;
    _text_bg = 0;          // transparent
}

void Display::setTextColor(uint32_t fg, uint32_t bg) {
    _text_fg = fg;
    _text_bg = bg;
}

void Display::setTextSize(uint8_t size) {
    _text_size = (size < 1) ? 1 : size;
}

void Display::setTextWrap(bool wrap) {
    _text_wrap = wrap;
}

void Display::drawChar(int16_t x, int16_t y, unsigned char c, uint32_t color, uint32_t bg, uint8_t size) {
    if (c < 32 || c > 127) c = '?';
    const uint8_t *glyph = dbg_font[(uint8_t)c];

    for (int8_t i = 0; i < 16; i++) {
        uint8_t line = glyph[i];
        for (int8_t j = 0; j < 8; j++) {
            if (line & (0x80 >> j)) {
                if (size == 1) {
                    drawPixel(x + j, y + i, color);
                } else {
                    fillRect(x + j * size, y + i * size, size, size, color);
                }
            } else if (bg != 0) {
                if (size == 1) {
                    drawPixel(x + j, y + i, bg);
                } else {
                    fillRect(x + j * size, y + i * size, size, size, bg);
                }
            }
        }
    }
}

void Display::write(uint8_t c) {
    if (c == '\n') {
        _cursor_x = 0;
        _cursor_y += 16 * _text_size;
        return;
    }
    if (c == '\r') return;

    if (_text_wrap && (_cursor_x + 8 * _text_size) > (int16_t)_width) {
        _cursor_x = 0;
        _cursor_y += 16 * _text_size;
    }

    drawChar(_cursor_x, _cursor_y, c, _text_fg, _text_bg, _text_size);
    _cursor_x += 8 * _text_size;
}

void Display::print(const char *str) {
    if (!str) return;
    while (*str) write(*str++);
}

void Display::println(const char *str) {
    if (str) print(str);
    write('\n');
}

void Display::print(int num) {
    char buf[12];
    int i = 0;
    bool neg = num < 0;
    if (neg) num = -num;
    do {
        buf[i++] = '0' + (num % 10);
        num /= 10;
    } while (num && i < 11);
    if (neg) buf[i++] = '-';
    while (i--) write(buf[i]);
}

void Display::print(unsigned int num) {
    print((long)num);
}

void Display::print(long num) {
    char buf[12];
    int i = 0;
    bool neg = num < 0;
    if (neg) num = -num;
    do {
        buf[i++] = '0' + (num % 10);
        num /= 10;
    } while (num && i < 11);
    if (neg) buf[i++] = '-';
    while (i--) write(buf[i]);
}

void Display::print(unsigned long num) {
    char buf[12];
    int i = 0;
    do {
        buf[i++] = '0' + (num % 10);
        num /= 10;
    } while (num && i < 11);
    while (i--) write(buf[i]);
}
// ---------------------------------------------------------------
// Remaining functions
// ---------------------------------------------------------------


uint8_t Display::getRotation() const {
    return _rotation;
}

void Display::setRotation(uint8_t r) {
    _rotation = r & 3;
    // Note: real rotation would swap width/height and transform coordinates.
    // Left as a simple stub for now – can be implemented later if needed.
}

void Display::getCursor(int16_t *x, int16_t *y) const {
    if (x) *x = _cursor_x;
    if (y) *y = _cursor_y;
}

// ---------------- Round Rect ----------------

void Display::drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint32_t color) {
    if (r > w/2) r = w/2;
    if (r > h/2) r = h/2;

    drawFastHLine(x + r, y, w - 2*r, color);                 // top
    drawFastHLine(x + r, y + h - 1, w - 2*r, color);         // bottom
    drawFastVLine(x, y + r, h - 2*r, color);                 // left
    drawFastVLine(x + w - 1, y + r, h - 2*r, color);         // right

    // four corners
    drawCircleHelper(x + r, y + r, r, 1, color);
    drawCircleHelper(x + w - r - 1, y + r, r, 2, color);
    drawCircleHelper(x + w - r - 1, y + h - r - 1, r, 4, color);
    drawCircleHelper(x + r, y + h - r - 1, r, 8, color);
}

void Display::fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint32_t color) {
    if (r > w/2) r = w/2;
    if (r > h/2) r = h/2;

    fillRect(x + r, y, w - 2*r, h, color);

    fillCircleHelper(x + w - r - 1, y + r, r, 1, h - 2*r - 1, color);
    fillCircleHelper(x + r, y + r, r, 2, h - 2*r - 1, color);
}

// ---------------- Triangle ----------------

void Display::drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint32_t color) {
    drawLine(x0, y0, x1, y1, color);
    drawLine(x1, y1, x2, y2, color);
    drawLine(x2, y2, x0, y0, color);
}

void Display::fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint32_t color) {
    // Simple sort by Y
    if (y0 > y1) { int16_t t; t=y0; y0=y1; y1=t; t=x0; x0=x1; x1=t; }
    if (y1 > y2) { int16_t t; t=y1; y1=y2; y2=t; t=x1; x1=x2; x2=t; }
    if (y0 > y1) { int16_t t; t=y0; y0=y1; y1=t; t=x0; x0=x1; x1=t; }

    if (y0 == y2) { // degenerate
        drawFastHLine(x0, y0, x2 - x0 + 1, color);
        return;
    }

    int16_t dx01 = x1 - x0, dy01 = y1 - y0;
    int16_t dx02 = x2 - x0, dy02 = y2 - y0;
    int16_t dx12 = x2 - x1, dy12 = y2 - y1;

    int32_t sa = 0, sb = 0;
    int16_t a, b, y, last;

    if (y1 == y2) last = y1;
    else          last = y1 - 1;

    for (y = y0; y <= last; y++) {
        a = x0 + sa / dy01;
        b = x0 + sb / dy02;
        sa += dx01;
        sb += dx02;
        if (a > b) { int16_t t = a; a = b; b = t; }
        drawFastHLine(a, y, b - a + 1, color);
    }

    sa = dx12 * (y - y1);
    sb = dx02 * (y - y0);
    for (; y <= y2; y++) {
        a = x1 + sa / dy12;
        b = x0 + sb / dy02;
        sa += dx12;
        sb += dx02;
        if (a > b) { int16_t t = a; a = b; b = t; }
        drawFastHLine(a, y, b - a + 1, color);
    }
}

// ---------------- Helpers for round rects ----------------

void Display::drawCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, uint32_t color) {
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;
        if (cornername & 0x4) {
            drawPixel(x0 + x, y0 + y, color);
            drawPixel(x0 + y, y0 + x, color);
        }
        if (cornername & 0x2) {
            drawPixel(x0 + x, y0 - y, color);
            drawPixel(x0 + y, y0 - x, color);
        }
        if (cornername & 0x8) {
            drawPixel(x0 - y, y0 + x, color);
            drawPixel(x0 - x, y0 + y, color);
        }
        if (cornername & 0x1) {
            drawPixel(x0 - y, y0 - x, color);
            drawPixel(x0 - x, y0 - y, color);
        }
    }
}

void Display::fillCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, int16_t delta, uint32_t color) {
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        if (cornername & 0x1) {
            drawFastVLine(x0 + x, y0 - y, 2*y + 1 + delta, color);
            drawFastVLine(x0 + y, y0 - x, 2*x + 1 + delta, color);
        }
        if (cornername & 0x2) {
            drawFastVLine(x0 - x, y0 - y, 2*y + 1 + delta, color);
            drawFastVLine(x0 - y, y0 - x, 2*x + 1 + delta, color);
        }
    }
}

// ---------------- Double print ----------------

void Display::print(double num, int decimals) {
    if (num < 0) {
        write('-');
        num = -num;
    }

    long ipart = (long)num;
    print(ipart);
    write('.');

    double frac = num - (double)ipart;
    for (int i = 0; i < decimals; i++) {
        frac *= 10.0;
        int digit = (int)frac;
        write('0' + digit);
        frac -= digit;
    }
}

// ---------------- Optional stubs ----------------

void Display::invertDisplay(bool i) {
    (void)i;   // not implemented yet
}

void Display::setFont(const void *font) {
    (void)font; // not implemented yet (still using dbg_font)
}
