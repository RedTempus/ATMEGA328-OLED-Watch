#include "ssd1306.h"

#include <Wire.h>

namespace {
constexpr uint8_t CommandStream = 0x00;
constexpr uint8_t DataStream = 0x40;
constexpr uint8_t TransferChunkSize = 16;

// 5 columns by 7 rows. The least-significant bit is the top pixel.
// Lowercase input is displayed as uppercase to keep the font compact.
const uint8_t Font[][5] PROGMEM = {
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, {0x7F, 0x49, 0x49, 0x49, 0x36},
    {0x3E, 0x41, 0x41, 0x41, 0x22}, {0x7F, 0x41, 0x41, 0x22, 0x1C},
    {0x7F, 0x49, 0x49, 0x49, 0x41}, {0x7F, 0x09, 0x09, 0x09, 0x01},
    {0x3E, 0x41, 0x49, 0x49, 0x7A}, {0x7F, 0x08, 0x08, 0x08, 0x7F},
    {0x00, 0x41, 0x7F, 0x41, 0x00}, {0x20, 0x40, 0x41, 0x3F, 0x01},
    {0x7F, 0x08, 0x14, 0x22, 0x41}, {0x7F, 0x40, 0x40, 0x40, 0x40},
    {0x7F, 0x02, 0x0C, 0x02, 0x7F}, {0x7F, 0x04, 0x08, 0x10, 0x7F},
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, {0x7F, 0x09, 0x09, 0x09, 0x06},
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, {0x7F, 0x09, 0x19, 0x29, 0x46},
    {0x46, 0x49, 0x49, 0x49, 0x31}, {0x01, 0x01, 0x7F, 0x01, 0x01},
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, {0x1F, 0x20, 0x40, 0x20, 0x1F},
    {0x7F, 0x20, 0x18, 0x20, 0x7F}, {0x63, 0x14, 0x08, 0x14, 0x63},
    {0x03, 0x04, 0x78, 0x04, 0x03}, {0x61, 0x51, 0x49, 0x45, 0x43},
};

const uint8_t Digits[][5] PROGMEM = {
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, {0x00, 0x42, 0x7F, 0x40, 0x00},
    {0x42, 0x61, 0x51, 0x49, 0x46}, {0x21, 0x41, 0x45, 0x4B, 0x31},
    {0x18, 0x14, 0x12, 0x7F, 0x10}, {0x27, 0x45, 0x45, 0x45, 0x39},
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, {0x01, 0x71, 0x09, 0x05, 0x03},
    {0x36, 0x49, 0x49, 0x49, 0x36}, {0x06, 0x49, 0x49, 0x29, 0x1E},
};

void copyGlyph(const uint8_t* source, uint8_t destination[5]) {
  for (uint8_t index = 0; index < 5; ++index) {
    destination[index] = pgm_read_byte(source + index);
  }
}

void punctuationGlyph(char character, uint8_t glyph[5]) {
  const uint8_t* source = nullptr;
  static const uint8_t Space[] PROGMEM = {0, 0, 0, 0, 0};
  static const uint8_t Colon[] PROGMEM = {0, 0x36, 0x36, 0, 0};
  static const uint8_t Semicolon[] PROGMEM = {0, 0x56, 0x36, 0, 0};
  static const uint8_t DoubleQuote[] PROGMEM = {0, 0x07, 0, 0x07, 0};
  static const uint8_t SingleQuote[] PROGMEM = {0, 0x05, 0x03, 0, 0};
  static const uint8_t Period[] PROGMEM = {0, 0x60, 0x60, 0, 0};
  static const uint8_t Dash[] PROGMEM = {0x08, 0x08, 0x08, 0x08, 0x08};
  static const uint8_t Slash[] PROGMEM = {0x60, 0x18, 0x06, 0x01, 0};
  static const uint8_t Unknown[] PROGMEM = {0x02, 0x01, 0x59, 0x09, 0x06};

  switch (character) {
    case ' ': source = Space; break;
    case ':': source = Colon; break;
    case ';': source = Semicolon; break;
    case '"': source = DoubleQuote; break;
    case '\'': source = SingleQuote; break;
    case '.': source = Period; break;
    case '-': source = Dash; break;
    case '/': source = Slash; break;
    default: source = Unknown; break;
  }
  copyGlyph(source, glyph);
}
}  // namespace

SSD1306::SSD1306(uint8_t address) : address_(address), column_(0), row_(0) {}

bool SSD1306::begin() {
  Wire.begin();

  const uint8_t initialization[] = {
      0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40,
      0x8D, 0x14, 0x20, 0x02, 0xA1, 0xC8, 0xDA, 0x12,
      0x81, 0x7F, 0xD9, 0xF1, 0xDB, 0x40, 0xA4, 0xA6, 0xAF,
  };

  for (uint8_t command : initialization) {
    if (!sendCommand(command)) {
      return false;
    }
  }

  clear();
  return true;
}

void SSD1306::clear() {
  for (uint16_t index = 0; index < sizeof(buffer_); ++index) {
    buffer_[index] = 0;
  }
  flushRegion(0, 0, Width, Rows);
  setCursor(0, 0);
}

void SSD1306::setCursor(uint8_t column, uint8_t row) {
  column_ = column % Columns;
  row_ = row % Rows;
}

void SSD1306::newLine() { setCursor(0, (row_ + 1) % Rows); }

bool SSD1306::setPixel(uint8_t x, uint8_t y) {
  if (x >= Width || y >= Height) {
    return false;
  }
  setPixelInBuffer(x, y, true);
  flushRegion(x, y / 8, 1, 1);
  return true;
}

bool SSD1306::clearPixel(uint8_t x, uint8_t y) {
  if (x >= Width || y >= Height) {
    return false;
  }
  setPixelInBuffer(x, y, false);
  flushRegion(x, y / 8, 1, 1);
  return true;
}

void SSD1306::clearRect(uint8_t x, uint8_t y, uint8_t width, uint8_t height) {
  if (x >= Width || y >= Height || width == 0 || height == 0) {
    return;
  }

  const uint8_t clippedWidth = (width > Width - x) ? Width - x : width;
  const uint8_t clippedHeight = (height > Height - y) ? Height - y : height;
  clearRectInBuffer(x, y, clippedWidth, clippedHeight);
  const uint8_t firstPage = y / 8;
  const uint8_t lastPage = (y + clippedHeight - 1) / 8;
  flushRegion(x, firstPage, clippedWidth, lastPage - firstPage + 1);
}

void SSD1306::write(char character) {
  if (character == '\n') {
    newLine();
    return;
  }
  if (character == '\r') {
    return;
  }

  uint8_t glyph[5];
  glyphFor(character, glyph);
  drawGlyph(column_ * 6, row_, glyph);
  flushRegion(column_ * 6, row_, 6, 1);

  ++column_;
  if (column_ >= Columns) {
    newLine();
  }
}

void SSD1306::print(const char* text) {
  if (text == nullptr) {
    return;
  }
  while (*text != '\0') {
    write(*text++);
  }
}

void SSD1306::print(int value) { print(static_cast<long>(value)); }
void SSD1306::print(unsigned int value) { print(static_cast<unsigned long>(value)); }

void SSD1306::print(long value) {
  char text[12];
  ltoa(value, text, 10);
  print(text);
}

void SSD1306::print(unsigned long value) {
  char text[11];
  ultoa(value, text, 10);
  print(text);
}

bool SSD1306::drawLargeDigit(uint8_t digit, uint8_t x, uint8_t page) {
  if (digit > 9 || page >= Rows - 1 || x > Width - LargeDigitWidth) {
    return false;
  }

  const uint8_t y = page * 8;
  uint8_t glyph[5];
  copyGlyph(Digits[digit], glyph);
  clearRectInBuffer(x, y, LargeDigitWidth, LargeDigitHeight);

  for (uint8_t sourceColumn = 0; sourceColumn < 5; ++sourceColumn) {
    for (uint8_t sourceRow = 0; sourceRow < 7; ++sourceRow) {
      if ((glyph[sourceColumn] & _BV(sourceRow)) == 0) {
        continue;
      }

      const uint8_t pixelX = x + sourceColumn * 2;
      const uint8_t pixelY = y + sourceRow * 2;
      setPixelInBuffer(pixelX, pixelY, true);
      setPixelInBuffer(pixelX + 1, pixelY, true);
      setPixelInBuffer(pixelX, pixelY + 1, true);
      setPixelInBuffer(pixelX + 1, pixelY + 1, true);
    }
  }

  flushRegion(x, page, LargeDigitWidth, 2);
  return true;
}

bool SSD1306::printLargeNumber(unsigned long value, uint8_t x, uint8_t page) {
  char text[11];
  ultoa(value, text, 10);

  uint8_t digitCount = 0;
  while (text[digitCount] != '\0') {
    ++digitCount;
  }

  const uint16_t requiredWidth = digitCount * LargeDigitWidth +
                                 (digitCount - 1) * LargeDigitSpacing;
  if (page >= Rows - 1 || requiredWidth > Width - x) {
    return false;
  }

  for (uint8_t index = 0; index < digitCount; ++index) {
    drawLargeDigit(text[index] - '0',
                   x + index * (LargeDigitWidth + LargeDigitSpacing), page);
  }
  return true;
}

bool SSD1306::sendCommand(uint8_t command) {
  Wire.beginTransmission(address_);
  Wire.write(CommandStream);
  Wire.write(command);
  return Wire.endTransmission() == 0;
}

bool SSD1306::sendData(const uint8_t* data, uint8_t length) {
  Wire.beginTransmission(address_);
  Wire.write(DataStream);
  Wire.write(data, length);
  return Wire.endTransmission() == 0;
}

void SSD1306::setPageColumn(uint8_t page, uint8_t column) {
  sendCommand(0xB0 | (page & 0x07));
  sendCommand(column & 0x0F);
  sendCommand(0x10 | ((column >> 4) & 0x0F));
}

void SSD1306::flushRegion(uint8_t x, uint8_t firstPage, uint8_t width,
                          uint8_t pageCount) {
  if (x >= Width || firstPage >= Rows || width == 0 || pageCount == 0) {
    return;
  }

  const uint8_t clippedWidth = (width > Width - x) ? Width - x : width;
  const uint8_t clippedPages = (pageCount > Rows - firstPage) ? Rows - firstPage : pageCount;
  for (uint8_t pageOffset = 0; pageOffset < clippedPages; ++pageOffset) {
    const uint8_t page = firstPage + pageOffset;
    setPageColumn(page, x);

    uint8_t sent = 0;
    while (sent < clippedWidth) {
      const uint8_t chunkLength = (clippedWidth - sent > TransferChunkSize)
                                      ? TransferChunkSize
                                      : clippedWidth - sent;
      sendData(&buffer_[static_cast<uint16_t>(page) * Width + x + sent], chunkLength);
      sent += chunkLength;
    }
  }
}

void SSD1306::setPixelInBuffer(uint8_t x, uint8_t y, bool enabled) {
  uint8_t& pageColumn = buffer_[static_cast<uint16_t>(y / 8) * Width + x];
  const uint8_t mask = _BV(y & 0x07);
  if (enabled) {
    pageColumn |= mask;
  } else {
    pageColumn &= ~mask;
  }
}

void SSD1306::clearRectInBuffer(uint8_t x, uint8_t y, uint8_t width, uint8_t height) {
  for (uint8_t row = y; row < y + height; ++row) {
    for (uint8_t column = x; column < x + width; ++column) {
      setPixelInBuffer(column, row, false);
    }
  }
}

void SSD1306::drawGlyph(uint8_t x, uint8_t page, const uint8_t glyph[5]) {
  const uint16_t start = static_cast<uint16_t>(page) * Width + x;
  for (uint8_t index = 0; index < 5; ++index) {
    buffer_[start + index] = glyph[index];
  }
  buffer_[start + 5] = 0;
}

void SSD1306::glyphFor(char character, uint8_t glyph[5]) const {
  if (character >= 'a' && character <= 'z') {
    character -= 'a' - 'A';
  }
  if (character >= 'A' && character <= 'Z') {
    copyGlyph(Font[character - 'A'], glyph);
  } else if (character >= '0' && character <= '9') {
    copyGlyph(Digits[character - '0'], glyph);
  } else {
    punctuationGlyph(character, glyph);
  }
}
