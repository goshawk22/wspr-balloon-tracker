#include "sensors.h"
#include "stm32yyxx_ll_adc.h"

//HardwareSerial gpsSerial(GPS_SERIAL_RX, GPS_SERIAL_TX);

Sensors::Sensors()
{
}

void Sensors::begin()
{
    analogReadResolution(12); // Set ADC resolution to 12 bits
}

void Sensors::update()
{
    //float v_adc = analogRead(ADC_PIN) * 3.3 / 4095.0; // 12-bit ADC
    //voltage = v_adc * VOLTAGE_DIVIDER_MULTIPLIER;

    //DEBUG_PRINTLN("Voltage: " + String(voltage) + " V");

    float v_adc = readVoltage(readVref(), ADC_PIN) * 3.3 / 4095.0; // 12-bit ADC
    voltage = v_adc * VOLTAGE_DIVIDER_MULTIPLIER;

    //DEBUG_PRINTLN("Voltage: " + String(voltage) + " V");

    //uint16_t TS_CAL1 = *((uint16_t*) TS_CAL1_ADDR); // Read calibration value @ 30°C
    //uint16_t TS_CAL2 = *((uint16_t*) TS_CAL2_ADDR); // Read calibration value @ 130°C

    //temperature = ((analogRead(ATEMP) - TS_CAL1) * (TEMP130_CAL_TEMP - TEMP30_CAL_TEMP)) /
    //     (TS_CAL2 - TS_CAL1) + TEMP30_CAL_TEMP;
    

    //DEBUG_PRINTLN("Temperature: " + String(temperature) + " °C");

    temperature = __LL_ADC_CALC_TEMPERATURE(readVref(), analogRead(ATEMP), LL_ADC_RESOLUTION_12B);
    //DEBUG_PRINTLN("Temperature: " + String(temperature) + " °C");
}


uint32_t Sensors::readVref() {
    uint32_t Vref = 3000;
    Vref = __LL_ADC_CALC_VREFANALOG_VOLTAGE(analogRead(AVREF), LL_ADC_RESOLUTION_12B);
    return Vref;
}

int32_t Sensors::readVoltage(int32_t VRef, uint32_t pin) {
    return (__LL_ADC_CALC_DATA_TO_VOLTAGE(VRef, analogRead(pin), LL_ADC_RESOLUTION_12B));
}
