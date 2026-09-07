#pragma once

#include <Arduino.h>

// U3 is a TPS22917 (not the active-low TPS22917L variant), so its ON input is
// active high.  The schematic ties QOD directly to VOUT, which discharges the
// switched PMOS_D rail when this driver turns the switch off.
void tps22917Init();
void tps22917Enable();
void tps22917Disable();
bool tps22917IsEnabled();
