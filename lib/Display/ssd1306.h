#pragma once

#include <Arduino.h>

// Text and pixel driver for 128x64 I2C SSD1306 OLED modules.  It maintains a
// 1 KB display buffer so individual pixels can be changed over I2C.
class SSD1306 {
public:
  static constexpr uint8_t DefaultAddress = 0x3C;
  static constexpr uint8_t Width = 128;
  static constexpr uint8_t Height = 64;
  static constexpr uint8_t Columns = 21;
  static constexpr uint8_t Rows = 8;
  static constexpr uint8_t LargeDigitWidth = 10;
  static constexpr uint8_t LargeDigitHeight = 16;
  static constexpr uint8_t LargeDigitSpacing = 2;

  explicit SSD1306(uint8_t address = DefaultAddress);

  bool begin();
  void clear();
  void setCursor(uint8_t column, uint8_t row);
  void newLine();

  // Pixel coordinates use x = 0..127 and y = 0..63.  These return false for
  // coordinates outside the display.
  bool setPixel(uint8_t x, uint8_t y);
  bool clearPixel(uint8_t x, uint8_t y);
  void clearRect(uint8_t x, uint8_t y, uint8_t width, uint8_t height);

  void write(char character);
  void print(const char* text);
  void print(int value);
  void print(unsigned int value);
  void print(long value);
  void print(unsigned long value);

  // Draws a 2x-scaled digit.  page is the top 8-pixel page (0..6); each digit
  // is 10 by 16 pixels and therefore spans the selected page and the next one.
  bool drawLargeDigit(uint8_t digit, uint8_t x, uint8_t page);

  // Draws a complete unsigned number using large digits.  x is a pixel
  // coordinate and page is 0..6.  Returns false when the number will not fit.
  bool printLargeNumber(unsigned long value, uint8_t x, uint8_t page);

private:
  bool sendCommand(uint8_t command);
  bool sendData(const uint8_t* data, uint8_t length);
  void setPageColumn(uint8_t page, uint8_t column);
  void flushRegion(uint8_t x, uint8_t firstPage, uint8_t width, uint8_t pageCount);
  void setPixelInBuffer(uint8_t x, uint8_t y, bool enabled);
  void clearRectInBuffer(uint8_t x, uint8_t y, uint8_t width, uint8_t height);
  void drawGlyph(uint8_t x, uint8_t page, const uint8_t glyph[5]);
  void glyphFor(char character, uint8_t glyph[5]) const;

  uint8_t address_;
  uint8_t column_;
  uint8_t row_;
  uint8_t buffer_[Width * Rows];
};
