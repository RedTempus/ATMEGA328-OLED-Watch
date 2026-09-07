#include <Arduino.h>
#include <ssd1306.h>
#include <tps22917.h>
#include <power.h>
#include <boardPins.h>
#include <rtc.h>
#include <buttons.h>

PCF85263A rtc;
SSD1306 display;

enum FieldID : uint8_t {
  YEAR, MONTH, DATE,
  DOW,
  HOUR, MINUTE, SECOND,
  FIELD_COUNT
};

struct FieldLayout {
  uint8_t x, y, textSize, clearWidth;
};

// will need pgm_read_byte to use these values
//                                x    y   sz  clearW
const FieldLayout LAYOUT[FIELD_COUNT] PROGMEM = {
  {0, 0, 16, 46},  // YEAR    → 4 large digits: 46 pixels wide
  {58, 0, 16, 22}, // MONTH   → 2 large digits: 22 pixels wide
  {86, 0, 16, 22}, // DAY     → 2 large digits: 22 pixels wide
  {0, 2, 8, 128},  // WEEKDAY → "Wednesday" (full row, size-1 text)
  {0, 4, 16, 22},  // HOUR    → 2 large digits
  {36, 4, 16, 22}, // MINUTE  → 2 large digits
  {72, 4, 16, 22}, // SECOND  → 2 large digits
};

// finding the day of the week 
const char D0[] PROGMEM = "Su";
const char D1[] PROGMEM = "Mo";
const char D2[] PROGMEM = "Tu";
const char D3[] PROGMEM = "We";
const char D4[] PROGMEM = "Th";
const char D5[] PROGMEM = "Fr";
const char D6[] PROGMEM = "Sa";
const char* const DAY_NAMES[7] PROGMEM = {D0, D1, D2, D3, D4, D5, D6};

struct TimeState {
  uint8_t dOW, date, month, hour, minute, second;
  uint16_t year;
};

TimeState curr = {}, prev = {};

void initializeButtons() {
  pinMode(boardPins::button1, INPUT); // increment hour
  pinMode(boardPins::button2, INPUT); // increment minute
  pinMode(boardPins::button3, INPUT); // reset seconds
  pinMode(boardPins::button4, INPUT); // wake interrupt
}

void drawGiven(FieldID id);
void drawAll();
bool getTimes(uint8_t *dOW, uint8_t *date, uint8_t *month, uint16_t *year, uint8_t *hour, uint8_t *minute, uint8_t *second);
void checkDraw();
void drawStaticSeparators();
void handleAdjustButtons();
void refreshTimeDisplay();

void setup() {
    powerInit();
    
    pinMode(boardPins::testLed, OUTPUT);

    initializeButtons();

    periphPowerOn();
    delay(2000);
    if (!rtc.begin()) {
      return;
    }
    
    if (!rtc.setDateTime(2026, 9, 7, 16, 34, 0)) {
      return;
    }
    
    if (!getTimes(&curr.dOW, &curr.date, &curr.month, &curr.year, &curr.hour, &curr.minute, &curr.second)) {
      periphPowerDown();
      return;
    }
    prev = curr;
    // drawAll();

    periphPowerDown();

}

constexpr uint16_t UPDATE_MS = 1000;
constexpr uint32_t WATCHON_MS = 10000;

void loop() {
  periphPowerOn(); // this is only required after baord sleep
  delay(100); // give rail time to power on

  // re enable communication
  if (!rtc.begin()) {
        sleepBoard();
        return; // rtc did not ack i2c
    }

  if (!display.begin()) {
        sleepBoard();
        return; // OLED did not ack its i2c
    }

  if (!getTimes(&curr.dOW, &curr.date, &curr.month, &curr.year, &curr.hour, &curr.minute, &curr.second)) {
    sleepBoard();
    return;
  }
  prev = curr;

  drawAll();
  drawStaticSeparators();

  const uint32_t watchOnStartedMs = millis();
  uint32_t lastUpdateMs = watchOnStartedMs;
  while (millis() - watchOnStartedMs < WATCHON_MS) {
    handleAdjustButtons();

    const uint32_t nowMs = millis();
    if (nowMs - lastUpdateMs >= UPDATE_MS) {
      lastUpdateMs = nowMs;
      refreshTimeDisplay();
    }
  }

  sleepBoard();
}

void drawGiven(FieldID id) {
  if (id >= FIELD_COUNT) {
    return;
  }

  FieldLayout field;
  memcpy_P(&field, &LAYOUT[id], sizeof(field));

  // Large-number Y coordinates are SSD1306 pages; clearRect() uses pixels.
  display.clearRect(field.x, field.y * 8, field.clearWidth, field.textSize);

  switch (id) {
    case YEAR:
      display.printLargeNumber(curr.year, field.x, field.y);
      break;
    case MONTH:
      display.printLargeNumber(curr.month, field.x, field.y);
      break;
    case DATE:
      display.printLargeNumber(curr.date, field.x, field.y);
      break;
    case HOUR:
      display.drawLargeDigit(curr.hour / 10, field.x, field.y);
      display.drawLargeDigit(curr.hour % 10,
                             field.x + SSD1306::LargeDigitWidth + SSD1306::LargeDigitSpacing,
                             field.y);
      break;
    case MINUTE:
      display.drawLargeDigit(curr.minute / 10, field.x, field.y);
      display.drawLargeDigit(curr.minute % 10,
                             field.x + SSD1306::LargeDigitWidth + SSD1306::LargeDigitSpacing,
                             field.y);
      break;
    case SECOND:
      display.drawLargeDigit(curr.second / 10, field.x, field.y);
      display.drawLargeDigit(curr.second % 10,
                             field.x + SSD1306::LargeDigitWidth + SSD1306::LargeDigitSpacing,
                             field.y);
      break;
    case DOW: {
      char dayName[3];
      strcpy_P(dayName, reinterpret_cast<const char*>(pgm_read_word(&DAY_NAMES[curr.dOW])));
      display.setCursor(field.x, field.y);
      display.print(dayName);
      break;
    }
    default:
      break;
  }
}

void drawAll() {
  for (uint8_t i=0; i < FIELD_COUNT; i++) {
    drawGiven(FieldID(i));
  }
}

bool getTimes(uint8_t *dOW, uint8_t *date, uint8_t *month, uint16_t *year, uint8_t *hour, uint8_t *minute, uint8_t *second) {
  RtcDateTime now;
  if (!rtc.getDateTime(now)) {
    return false;
  }

  *year = now.year;
  *month = now.month;
  *date = now.day;
  *dOW = now.weekday;
  *hour = now.hour;
  *minute = now.minute;
  *second = now.second;
  return true;
}

void checkDraw() {
  if(curr.dOW != prev.dOW) drawGiven(DOW);
  if(curr.date != prev.date) drawGiven(DATE);
  if(curr.month != prev.month) drawGiven(MONTH);
  if(curr.year != prev.year) drawGiven(YEAR);
  if(curr.hour != prev.hour) drawGiven(HOUR);
  if(curr.minute != prev.minute) drawGiven(MINUTE);
  if(curr.second != prev.second) drawGiven(SECOND);
  prev = curr;
}

void drawStaticSeparators() {
  // Large slash marks between YYYY/MM/DD.  Each fits in the existing gaps.
  for (uint8_t row = 0; row < 16; ++row) {
    display.setPixel(54 - row / 4, row);
    display.setPixel(84 - row / 4, row);
  }

  // Large colon marks between HH:MM:SS on display pages 4 and 5.
  for (uint8_t dot = 0; dot < 2; ++dot) {
    display.setPixel(27 + dot, 36);
    display.setPixel(27 + dot, 37);
    display.setPixel(27 + dot, 42);
    display.setPixel(27 + dot, 43);
    display.setPixel(63 + dot, 36);
    display.setPixel(63 + dot, 37);
    display.setPixel(63 + dot, 42);
    display.setPixel(63 + dot, 43);
  }
}

void refreshTimeDisplay() {
  if (getTimes(&curr.dOW, &curr.date, &curr.month, &curr.year, &curr.hour, &curr.minute, &curr.second)) {
    checkDraw();
  }
}

void handleAdjustButtons() {
  static bool hourButtonWasPressed = false;
  static bool minuteButtonWasPressed = false;
  static bool secondButtonWasPressed = false;

  const bool hourButtonPressed = digitalRead(boardPins::button1) == LOW;
  const bool minuteButtonPressed = digitalRead(boardPins::button2) == LOW;
  const bool secondButtonPressed = digitalRead(boardPins::button3) == LOW;

  RtcDateTime now;
  bool updateRtc = false;

  if (hourButtonPressed && !hourButtonWasPressed && rtc.getDateTime(now)) {
    now.hour = (now.hour + 1) % 24;
    updateRtc = true;
  } else if (minuteButtonPressed && !minuteButtonWasPressed && rtc.getDateTime(now)) {
    now.minute = (now.minute + 1) % 60;
    updateRtc = true;
  } else if (secondButtonPressed && !secondButtonWasPressed && rtc.getDateTime(now)) {
    now.second = 0;
    updateRtc = true;
  }

  hourButtonWasPressed = hourButtonPressed;
  minuteButtonWasPressed = minuteButtonPressed;
  secondButtonWasPressed = secondButtonPressed;

  if (updateRtc && rtc.setDateTime(now)) {
    refreshTimeDisplay();
  }
}
