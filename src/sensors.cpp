#include "sensors.h"

#ifdef HAS_PRESSURE_SENSOR
TwoWire WireSensors(SENSOR_SDA, SENSOR_SCL);
Adafruit_LPS22 lps;
#endif

Sensors::Sensors()
{
}

void Sensors::begin()
{
    analogReadResolution(12);          // Set ADC resolution to 12 bits
#ifdef HAS_PRESSURE_SENSOR
    setSensorVCC(true);
    WireSensors.begin();               // Initialize the I2C bus for sensors
    lps.begin_I2C(0x5C, &WireSensors); // Initialize the LPS22 sensor over I2C

    lps.setDataRate(LPS22_RATE_1_HZ);

    DEBUG_PRINTLN("[Sensors] Sensors initialized");
    sensors_event_t temp_event;
    sensors_event_t pressure_event;
    lps.getEvent(&pressure_event, &temp_event); // get pressure
    DEBUG_PRINTF("[Sensors] Pressure: %.2f hPa \n\r", pressure_event.pressure);
#endif
}

void Sensors::setSensorVCC(bool enabled) {
    if (enabled) {
        digitalWrite(SENSOR_VCC, HIGH);
    } else {
        digitalWrite(SENSOR_VCC, LOW);
    }
}

void Sensors::update()
{

    float v_adc = readVoltage(readVref(), ADC_PIN);
    voltage = v_adc * ADC_VOLTAGE_DIVIDER_MULTIPLIER / 1000.0; // Convert to volts

    int_temperature = __LL_ADC_CALC_TEMPERATURE(readVref(), analogRead(ATEMP), LL_ADC_RESOLUTION_12B);
#ifdef HAS_PRESSURE_SENSOR
    sensors_event_t temp_event;
    sensors_event_t pressure_event;
    lps.getEvent(&pressure_event, &temp_event); // get pressure
    temperature = temp_event.temperature;
    pressure = pressure_event.pressure;
#endif
    DEBUG_PRINTLN("Sensors updated:");
    DEBUG_PRINT("Voltage: ");
    DEBUG_PRINTLN(voltage);
    DEBUG_PRINT("Internal Temperature: ");
    DEBUG_PRINTLN(int_temperature);
#ifdef HAS_PRESSURE_SENSOR
    DEBUG_PRINT("LPS22 Temperature: ");
    DEBUG_PRINTLN(temperature);
    DEBUG_PRINT("Pressure: ");
    DEBUG_PRINTLN(pressure);
#endif
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
