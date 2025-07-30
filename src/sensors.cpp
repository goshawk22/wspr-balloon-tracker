#include "sensors.h"

TwoWire WireSensors(PA12, PA11);
Adafruit_LPS22 lps;

Sensors::Sensors()
{
}

void Sensors::begin()
{
    analogReadResolution(12);          // Set ADC resolution to 12 bits
    WireSensors.begin();               // Initialize the I2C bus for sensors
    lps.begin_I2C(0x5C, &WireSensors); // Initialize the LPS22 sensor over I2C

    lps.setDataRate(LPS22_RATE_1_HZ);
}

void Sensors::update()
{

    float v_adc = readVoltage(readVref(), ADC_PIN);
    voltage = v_adc * VOLTAGE_DIVIDER_MULTIPLIER / 1000.0; // Convert to volts

    temperature = __LL_ADC_CALC_TEMPERATURE(readVref(), analogRead(ATEMP), LL_ADC_RESOLUTION_12B);

    sensors_event_t temp_event;
    sensors_event_t pressure_event;
    lps.getEvent(&pressure_event, &temp_event); // get pressure
    int_temperature = temp_event.temperature;
    pressure = pressure_event.pressure;
    DEBUG_PRINTLN("Sensors updated:");
    DEBUG_PRINT("Voltage: ");
    DEBUG_PRINTLN(voltage);
    DEBUG_PRINT("Internal Temperature: ");
    DEBUG_PRINTLN(int_temperature);
    DEBUG_PRINT("LPS22 Temperature: ");
    DEBUG_PRINTLN(temperature);
    DEBUG_PRINT("Pressure: ");
    DEBUG_PRINTLN(pressure);
}

uint32_t Sensors::readVref()
{
    uint32_t Vref = 3000;
    Vref = __LL_ADC_CALC_VREFANALOG_VOLTAGE(analogRead(AVREF), LL_ADC_RESOLUTION_12B);
    return Vref;
}

int32_t Sensors::readVoltage(int32_t VRef, uint32_t pin)
{
    return (__LL_ADC_CALC_DATA_TO_VOLTAGE(VRef, analogRead(pin), LL_ADC_RESOLUTION_12B));
}
