#include "rtc.h"

#include <Wire.h>

namespace {
constexpr uint8_t RegisterTimeStart = 0x00;
constexpr uint8_t TimeRegisterCount = 8;
constexpr uint8_t DaysInMonth[] = {
    31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31,
};

bool isLeapYear(uint16_t year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}
}  // namespace

PCF85263A::PCF85263A(uint8_t address) : address_(address) {}

bool PCF85263A::begin() {
    Wire.begin();

    return isConnected() && configureRtcMode() && use24HourMode() &&
           enableBatteryBackup();
}

bool PCF85263A::isConnected() {
    Wire.beginTransmission(address_);
    return Wire.endTransmission() == 0;
}

bool PCF85263A::setDateTime(const RtcDateTime& dateTime) {
    if (!isDateTimeValid(dateTime) || !configureRtcMode() || !use24HourMode()) {
        return false;
    }

    // Register 2F wraps to 00, so this one write sets STOP, clears the
    // prescalers, and writes 100ths through years as required by the datasheet.
    const uint8_t values[] = {
        StopRtc,
        ClearPrescaler,
        0x00,
        toBcd(dateTime.second),
        toBcd(dateTime.minute),
        toBcd(dateTime.hour),
        toBcd(dateTime.day),
        dateTime.weekday,
        toBcd(dateTime.month),
        toBcd(static_cast<uint8_t>(dateTime.year - 2000)),
    };

    return writeRegisters(RegisterStop, values, sizeof(values)) &&
           writeRegister(RegisterStop, 0x00);
}

bool PCF85263A::setDateTime(uint16_t year, uint8_t month, uint8_t day,
                             uint8_t hour, uint8_t minute, uint8_t second) {
    // Validate before calculating the weekday so an invalid month cannot be
    // used to index the weekday lookup table.
    RtcDateTime dateTime = {
        year,
        month,
        day,
        0,
        hour,
        minute,
        second,
    };
    if (!isDateTimeValid(dateTime)) {
        return false;
    }

    dateTime.weekday = weekdayForDate(year, month, day);
    return setDateTime(dateTime);
}

bool PCF85263A::getDateTime(RtcDateTime& dateTime) {
    uint8_t values[TimeRegisterCount];
    if (!readRegisters(RegisterTimeStart, values, sizeof(values)) ||
        (values[1] & OscillatorStopped) != 0) {
        return false;
    }

    RtcDateTime result = {
        static_cast<uint16_t>(2000 + fromBcd(values[7])),
        fromBcd(values[6] & 0x1F),
        fromBcd(values[4] & 0x3F),
        static_cast<uint8_t>(values[5] & 0x07),
        fromBcd(values[3] & 0x3F),
        fromBcd(values[2] & 0x7F),
        fromBcd(values[1] & 0x7F),
    };

    if (!isDateTimeValid(result)) {
        return false;
    }

    dateTime = result;
    return true;
}

bool PCF85263A::isTimeValid() {
    uint8_t seconds = 0;
    return readRegister(RegisterSeconds, seconds) &&
           (seconds & OscillatorStopped) == 0;
}

bool PCF85263A::enableBatteryBackup() {
    uint8_t batterySwitch = 0;
    if (!readRegister(RegisterBatterySwitch, batterySwitch)) {
        return false;
    }

    // BSOFF = 0 enables switching; BSM = 00 selects VDD/Vth switching.  Keep
    // the hardware's threshold and refresh-rate settings unchanged.
    batterySwitch &= ~(BatterySwitchOff | BatteryModeMask);
    return writeRegister(RegisterBatterySwitch, batterySwitch);
}

uint8_t PCF85263A::weekdayForDate(uint16_t year, uint8_t month, uint8_t day) {
    // Sakamoto's algorithm; its result uses Sunday = 0, matching the RTC.
    static const uint8_t MonthOffsets[] = {
        0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4,
    };
    if (month < 3) {
        --year;
    }
    return static_cast<uint8_t>(
        (year + year / 4 - year / 100 + year / 400 + MonthOffsets[month - 1] + day) % 7);
}

bool PCF85263A::configureRtcMode() {
    uint8_t function = 0;
    if (!readRegister(RegisterFunction, function)) {
        return false;
    }
    function &= ~FunctionRtcMode;
    return writeRegister(RegisterFunction, function);
}

bool PCF85263A::use24HourMode() {
    uint8_t oscillator = 0;
    if (!readRegister(RegisterOscillator, oscillator)) {
        return false;
    }
    oscillator &= ~Oscillator12Hour;
    return writeRegister(RegisterOscillator, oscillator);
}

bool PCF85263A::readRegister(uint8_t registerAddress, uint8_t& value) {
    return readRegisters(registerAddress, &value, 1);
}

bool PCF85263A::writeRegister(uint8_t registerAddress, uint8_t value) {
    return writeRegisters(registerAddress, &value, 1);
}

bool PCF85263A::readRegisters(uint8_t registerAddress, uint8_t* values, uint8_t length) {
    Wire.beginTransmission(address_);
    Wire.write(registerAddress);
    if (Wire.endTransmission(false) != 0 ||
        Wire.requestFrom(address_, length) != length) {
        return false;
    }

    for (uint8_t index = 0; index < length; ++index) {
        if (!Wire.available()) {
            return false;
        }
        values[index] = Wire.read();
    }
    return true;
}

bool PCF85263A::writeRegisters(uint8_t registerAddress, const uint8_t* values,
                                uint8_t length) {
    Wire.beginTransmission(address_);
    Wire.write(registerAddress);
    for (uint8_t index = 0; index < length; ++index) {
        Wire.write(values[index]);
    }
    return Wire.endTransmission() == 0;
}

bool PCF85263A::isDateTimeValid(const RtcDateTime& dateTime) {
    if (dateTime.year < 2000 || dateTime.year > 2099 || dateTime.month < 1 ||
        dateTime.month > 12 || dateTime.hour > 23 || dateTime.minute > 59 ||
        dateTime.second > 59 || dateTime.weekday > 6) {
        return false;
    }

    uint8_t maximumDay = DaysInMonth[dateTime.month - 1];
    if (dateTime.month == 2 && isLeapYear(dateTime.year)) {
        maximumDay = 29;
    }
    return dateTime.day >= 1 && dateTime.day <= maximumDay;
}

uint8_t PCF85263A::toBcd(uint8_t value) {
    return static_cast<uint8_t>(((value / 10) << 4) | (value % 10));
}

uint8_t PCF85263A::fromBcd(uint8_t value) {
    return static_cast<uint8_t>(((value >> 4) * 10) + (value & 0x0F));
}
