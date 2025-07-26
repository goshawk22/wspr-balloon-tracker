#ifndef SENSORS_H
#define SENSORS_H

#include "config.h"
#include "console.h"

class Sensors
{
public:
    Sensors();
    void begin();
    void update();

    // Accessors
    float getVoltage() const
    {
        return voltage;
    }
    float getTemperature() const
    {
        return temperature;
    }

private:
    float voltage; // Voltage read from ADC
    float temperature; // Temperature read from internal sensor

    static constexpr uint32_t TS_CAL1_ADDR = 0x1FFF75A8; // Temp calibration @ 30°C
    static constexpr uint32_t TS_CAL2_ADDR = 0x1FFF75CA; // Temp calibration @ 130°C
    static constexpr float TEMP30_CAL_TEMP = 30.0f;
    static constexpr float TEMP130_CAL_TEMP = 130.0f;

    static uint32_t readVref();
    static int32_t readVoltage(int32_t VRef, uint32_t pin);
};

#endif