#include <Arduino.h>
#include <avr/interrupt.h>
#include <avr/power.h>
#include <avr/sleep.h>

#include "boardPins.h"
#include "power.h"
#include "tps22917.h"

namespace {

volatile bool wakeRequested = false;

struct GpioState {
    uint8_t ddrb;
    uint8_t ddrc;
    uint8_t ddrd;
    uint8_t portb;
    uint8_t portc;
    uint8_t portd;
    uint8_t didr0;
    uint8_t didr1;
};

void onButton4Wake() {
    wakeRequested = true;
}

GpioState saveGpioState() {
    return {DDRB, DDRC, DDRD, PORTB, PORTC, PORTD, DIDR0, DIDR1};
}

void makePinsLowLeakage() {
    // Input with no internal pull-up prevents the MCU from driving any external
    // device while it is asleep.  BTN4 has the board's external 10 kOhm pull-up.
    DDRB = 0;
    DDRC = 0;
    DDRD = 0;
    PORTB = 0;
    PORTC = 0;
    PORTD = 0;

    // Disable the digital input buffers on ADC and comparator-capable pins.
    DIDR0 = 0x3F;
    DIDR1 = _BV(AIN1D) | _BV(AIN0D);
}

void restoreGpioState(const GpioState& state) {
    // Restore output latches before their direction registers so no pin briefly
    // drives the wrong level after wake-up.
    PORTB = state.portb;
    PORTC = state.portc;
    PORTD = state.portd;
    DDRB = state.ddrb;
    DDRC = state.ddrc;
    DDRD = state.ddrd;
    DIDR0 = state.didr0;
    DIDR1 = state.didr1;
}

}  // namespace

void powerInit() {
    tps22917Init();
}

void periphPowerDown() {
    tps22917Disable();
}

void periphPowerOn() {
    tps22917Enable();
}

void sleepBoard() {
    // Turn off the switched rail before changing PD5 into a high-impedance pin.
    periphPowerDown();

    const GpioState gpioState = saveGpioState();
    const uint8_t peripheralState = PRR;
    const bool adcWasEnabled = (ADCSRA & _BV(ADEN)) != 0;
    const bool comparatorWasDisabled = (ACSR & _BV(ACD)) != 0;

    makePinsLowLeakage();
    ADCSRA &= ~_BV(ADEN);  // ADC off
    ACSR |= _BV(ACD);      // analog comparator off
    power_all_disable();

    wakeRequested = false;
    attachInterrupt(digitalPinToInterrupt(boardPins::button4), onButton4Wake, FALLING);
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);

    cli();
    // Do not sleep while the button is already held down; it would not produce
    // another falling edge until released and pressed again.
    if (digitalRead(boardPins::button4) == HIGH && !wakeRequested) {
        sleep_enable();
        sleep_bod_disable();
        sei();
        sleep_cpu();
    }
    sleep_disable();
    sei();

    detachInterrupt(digitalPinToInterrupt(boardPins::button4));
    PRR = peripheralState;
    if (adcWasEnabled) {
        ADCSRA |= _BV(ADEN);
    }
    if (!comparatorWasDisabled) {
        ACSR &= ~_BV(ACD);
    }
    restoreGpioState(gpioState);
}
