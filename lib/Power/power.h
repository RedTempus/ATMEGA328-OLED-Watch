#pragma once

// Configure U3 and leave its switched rail off.  Call once from setup().
void powerInit();

// Direct controls for the TPS22917 switched PMOS_D rail.
void periphPowerDown();
void periphPowerOn();

// Enter ATmega328P power-down sleep.  BTN4 (PD3 / Arduino pin 3 / INT1) wakes
// the board on a press.  All GPIO are high-impedance while sleeping, except
// BTN4, which remains an input using its external pull-up.
void sleepBoard();
