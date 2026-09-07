#include <Arduino.h>

#include "boardPins.h"
#include "tps22917.h"

void tps22917Init() {
    // Keep the switched PMOS_D rail off while the rest of the board starts.
    pinMode(boardPins::tpsEnable, OUTPUT);
    digitalWrite(boardPins::tpsEnable, LOW);
}

void tps22917Enable() {
    pinMode(boardPins::tpsEnable, OUTPUT);
    digitalWrite(boardPins::tpsEnable, HIGH);
}

void tps22917Disable() {
    pinMode(boardPins::tpsEnable, OUTPUT);
    digitalWrite(boardPins::tpsEnable, LOW);
}

bool tps22917IsEnabled() {
    return digitalRead(boardPins::tpsEnable) == HIGH;
}
