#pragma once

#include <Arduino.h>

// Calendar year is stored as 2000 through 2099.  Weekday follows the
// PCF85263A convention: Sunday = 0 through Saturday = 6.
struct RtcDateTime {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t weekday;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
};

// Driver for U4, the PCF85263A real-time clock at I2C address 0x51.
class PCF85263A {
public:
    static constexpr uint8_t DefaultAddress = 0x51;

    explicit PCF85263A(uint8_t address = DefaultAddress);

    // Starts I2C, selects 24-hour RTC mode, and enables VBAT switchover.
    // It does not set the clock; use setDateTime() once after programming.
    bool begin();
    bool isConnected();

    // Atomically writes the complete calendar.  The RTC is stopped, its
    // prescalers are cleared, all time/date registers are written in one I2C
    // transfer, and the RTC is restarted.
    bool setDateTime(const RtcDateTime& dateTime);

    // Convenience form.  The weekday is calculated automatically.
    bool setDateTime(uint16_t year, uint8_t month, uint8_t day,
                     uint8_t hour, uint8_t minute, uint8_t second);

    // Reads all time/date registers in one I2C transfer.  Returns false if the
    // RTC reports an oscillator stop, the data is invalid, or I2C fails.
    bool getDateTime(RtcDateTime& dateTime);
    bool isTimeValid();

    // Keeps the battery switch enabled and selects automatic VDD-threshold
    // switchover.  VBAT must remain powered in hardware for backup to work.
    bool enableBatteryBackup();

    // Sunday = 0 through Saturday = 6.
    static uint8_t weekdayForDate(uint16_t year, uint8_t month, uint8_t day);

private:
    static constexpr uint8_t RegisterSeconds = 0x01;
    static constexpr uint8_t RegisterOscillator = 0x25;
    static constexpr uint8_t RegisterBatterySwitch = 0x26;
    static constexpr uint8_t RegisterFunction = 0x28;
    static constexpr uint8_t RegisterStop = 0x2E;

    static constexpr uint8_t Oscillator12Hour = 0x20;
    static constexpr uint8_t FunctionRtcMode = 0x10;
    static constexpr uint8_t BatterySwitchOff = 0x10;
    static constexpr uint8_t BatteryModeMask = 0x06;
    static constexpr uint8_t OscillatorStopped = 0x80;
    static constexpr uint8_t StopRtc = 0x01;
    static constexpr uint8_t ClearPrescaler = 0xA4;

    bool configureRtcMode();
    bool use24HourMode();
    bool readRegister(uint8_t registerAddress, uint8_t& value);
    bool writeRegister(uint8_t registerAddress, uint8_t value);
    bool readRegisters(uint8_t registerAddress, uint8_t* values, uint8_t length);
    bool writeRegisters(uint8_t registerAddress, const uint8_t* values, uint8_t length);

    static bool isDateTimeValid(const RtcDateTime& dateTime);
    static uint8_t toBcd(uint8_t value);
    static uint8_t fromBcd(uint8_t value);

    uint8_t address_;
};
