#include "sensors.h"

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

    float v_adc = readVoltage(readVref(), ADC_PIN);
    voltage = v_adc * VOLTAGE_DIVIDER_MULTIPLIER / 1000.0; // Convert to volts

    temperature = __LL_ADC_CALC_TEMPERATURE(readVref(), analogRead(ATEMP), LL_ADC_RESOLUTION_12B);
}

uint32_t Sensors::readVref() {
    uint32_t Vref = 3000;
    Vref = __LL_ADC_CALC_VREFANALOG_VOLTAGE(analogRead(AVREF), LL_ADC_RESOLUTION_12B);
    return Vref;
}

int32_t Sensors::readVoltage(int32_t VRef, uint32_t pin) {
    return (__LL_ADC_CALC_DATA_TO_VOLTAGE(VRef, analogRead(pin), LL_ADC_RESOLUTION_12B));
}
