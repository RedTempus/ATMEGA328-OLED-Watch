#pragma once

#include <Arduino.h>

// ATmega328P-A board map derived from WatchMe.kicad_sch / WatchMe.kicad_pcb.
// Values in the main namespace are Arduino digital-pin numbers, not package
// pin numbers.  For example, PC2 is Arduino pin 16 and the physical QFP pin
// for PD3 is pin 1.
namespace boardPins {

// Port D: UART, buttons, load-switch enable, and spare GPIO.
constexpr uint8_t serialRx = 0;          // PD0 / RXD
constexpr uint8_t serialTx = 1;          // PD1 / TXD
constexpr uint8_t button1 = 2;           // PD2 / INT0, SW2 (HourSW)
constexpr uint8_t button4 = 3;           // PD3 / INT1, SW5 (AdjustSW / wake)
constexpr uint8_t button2 = 4;           // PD4, SW3 (MinuteSW)
constexpr uint8_t tpsEnable = 5;    // PD5, U3 ON / PMOSPin
constexpr uint8_t gpioExtra = 6;         // PD6 / GPIOextra
constexpr uint8_t button3 = 7;           // PD7, SW4 (SecondSW)

// Port B: battery signal and ISP/SPI programming interface.
constexpr uint8_t pb0Bat = 8;            // PB0 / PB0Bat
constexpr uint8_t unusedPb1 = 9;         // PB1, no board connection
constexpr uint8_t unusedPb2 = 10;        // PB2, no board connection
constexpr uint8_t spiMosi = 11;          // PB3 / MOSI
constexpr uint8_t spiMiso = 12;          // PB4 / MISO
constexpr uint8_t spiSck = 13;           // PB5 / SCK

// Port C: ADC signals, test LED, and the shared I2C bus for the RTC/display.
constexpr uint8_t pc0Bat = 14;           // PC0 / ADC0 / PC0Bat
constexpr uint8_t adc1 = 15;             // PC1 / ADC1
constexpr uint8_t testLed = 16;          // PC2 / ADC2 / LEDtst
constexpr uint8_t unusedPc3 = 17;        // PC3 / ADC3, no board connection
constexpr uint8_t sda = 18;           // PC4 / ADC4 / SDA
constexpr uint8_t scl = 19;           // PC5 / ADC5 / SCL

// ADC6 and ADC7 are analog-only channels on the TQFP-32 package, so these are
// analogRead() channel numbers rather than Arduino digital-pin numbers.
namespace AnalogChannel {
constexpr uint8_t adc6 = 6;              // ADC6 / A6
constexpr uint8_t ground = 7;            // ADC7, tied to GND on this board
}

// These pins are connected to fixed hardware and must not be used as GPIO.
namespace ReservedPortBit {
constexpr uint8_t reset = PC6;           // ~RESET / PC6
constexpr uint8_t xtal1 = PB6;           // XTAL1 / PB6
constexpr uint8_t xtal2 = PB7;           // XTAL2 / PB7
}
}
