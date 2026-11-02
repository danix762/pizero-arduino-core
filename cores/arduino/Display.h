#pragma once
#include <stdint.h>
#include <stddef.h>

class Display {
public:
  // --- Initialization ---
  bool begin(uint16_t width = 640, uint16_t height = 480);
  void end();

  // --- Screen info ---
  uint16_t width() const;
  uint16_t height() const;
  uint8_t  getRotation() const;          // 0–3
  void     setRotation(uint8_t r);       // 0–3

  // --- Direct framebuffer access (your special feature) ---
  uint32_t*       getFramebuffer();
  const uint32_t* getFramebuffer() const;
  uint32_t        getPitch() const;      // bytes per row

  // --- Basic drawing ---
  void fillScreen(uint32_t color);
  void clearDisplay() { fillScreen(0); } // common alias

  void drawPixel(int16_t x, int16_t y, uint32_t color);
  void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint32_t color);
  void drawFastVLine(int16_t x, int16_t y, int16_t h, uint32_t color);
  void drawFastHLine(int16_t x, int16_t y, int16_t w, uint32_t color);
  void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint32_t color);
  void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint32_t color);
  void drawCircle(int16_t x0, int16_t y0, int16_t r, uint32_t color);
  void fillCircle(int16_t x0, int16_t y0, int16_t r, uint32_t color);
  void drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint32_t color);
  void fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint32_t color);
  void drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint32_t color);
  void fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint32_t color);

  // --- Text ---
  void setCursor(int16_t x, int16_t y);
  void getCursor(int16_t *x, int16_t *y) const;
  void setTextColor(uint32_t fg);
  void setTextColor(uint32_t fg, uint32_t bg);   // bg = 0 means transparent
  void setTextSize(uint8_t size);                // 1 = normal, 2 = 2x, etc.
  void setTextWrap(bool wrap);

  void write(uint8_t c);                         // single character
  void print(const char *str);
  void println(const char *str = nullptr);
  void print(int num);
  void print(unsigned int num);
  void print(long num);
  void print(unsigned long num);
  void print(double num, int decimals = 2);

  // --- Optional but nice ---
  void invertDisplay(bool i);                    // can be empty for now
  void setFont(const void *font = nullptr);      // optional, can ignore at first

private:
  uint16_t _width, _height;
  uint32_t _pitch;                               // bytes per row
  uint32_t *_fb;
  int16_t  _cursor_x, _cursor_y;
  uint32_t _text_fg, _text_bg;
  uint8_t  _text_size;
  bool     _text_wrap;
  uint8_t  _rotation;

  // internal helpers
  void drawChar(int16_t x, int16_t y, unsigned char c, uint32_t color, uint32_t bg, uint8_t size);
  void drawCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, uint32_t color);
  void fillCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, int16_t delta, uint32_t color);
};
