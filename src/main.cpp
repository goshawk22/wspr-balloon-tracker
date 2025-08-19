#include "telemetry.h"
#include "gps.h"
#include "config.h"
#include "console.h"
#include "sensors.h"
#include <IWatchdog.h>

GPS gps;
Si5351 si5351;
Telemetry telemetry(si5351);
Sensors sensors;

char call[] = CALLSIGN;

uint32_t lastPrintTime = 0;
uint8_t lastMinute = 61; // Initialize to a value that won't match the first minute check
char tx_loc[8]; // locator at start of type 1 frame, needs to stay the same for all 3 frames.

bool shouldTransmit = true;

STM32RTC &rtc = STM32RTC::getInstance(); // Use STM32RTC for accurate timekeeping

void setup()
{
    IWatchdog.begin(16000000); // Set watchdog timeout to 16 seconds
#ifndef BMP_DEBUG
    // Initialize the GPS serial port
    SerialPC.setRx(PC_SERIAL_RX);
    SerialPC.setTx(PC_SERIAL_TX);
    SerialPC.begin(PC_SERIAL_BAUD); // Set the baud rate for SerialPC
#endif
    delay(1000);
    DEBUG_PRINTLN("Starting...");
    IWatchdog.reload();

    analogReadResolution(12); // Set ADC resolution to 12 bits
    float v_adc = sensors.get_v();

    DEBUG_PRINTF("Current voltage: %.2f V\n", v_adc);

    while (v_adc < THRESHOLD_VOLTAGE) {
        DEBUG_PRINTLN("Voltage is too low, waiting...");
        DEBUG_PRINTF("Current voltage: %.2f V\n", v_adc);
        delay(1000);
        v_adc = sensors.get_v();
        IWatchdog.reload();
    }

    gps.begin();

    v_adc = sensors.get_v();

    DEBUG_PRINTF("Current voltage: %.2f V", v_adc);

    while (v_adc < THRESHOLD_VOLTAGE) {
        DEBUG_PRINTLN("Voltage is too low, waiting...");
        DEBUG_PRINTF("Current voltage: %.2f V\n", v_adc);
        delay(1000);
        v_adc = sensors.get_v();
        IWatchdog.reload();
    }

    telemetry.init();

    v_adc = sensors.get_v();

    DEBUG_PRINTF("Current voltage: %.2f V\n", v_adc);


    while (v_adc < THRESHOLD_VOLTAGE) {
        DEBUG_PRINTLN("Voltage is too low, waiting...");
        DEBUG_PRINTF("Current voltage: %.2f V\n", v_adc);
        delay(1000);
        v_adc = sensors.get_v();
        IWatchdog.reload();
    }

    sensors.begin();
    IWatchdog.reload();
}

void loop()
{
    IWatchdog.reload();
    gps.update();

    if (telemetry.isExtendedSent() && !telemetry.isTransmitting()) {
        gps.enable(); // Re-enable GPS if extended telemetry was sent
        telemetry.setExtendedSent(false); // Reset the flag

    }

    if (shouldTransmit &&(rtc.getMinutes() % 10 == telemetry.getMinute()) && (rtc.getSeconds() == 1) && !telemetry.isTransmitting() && gps.isRTCSynced())
    {
        //lastMinute = gps.getMinute(); // Update last minute to prevent duplicate sends
        
        // Get sensor measurements at the same time as GPS fix so all the data matches
        sensors.update();

        // Turn off the GPS module to save power
        //digitalWrite(GPS_ON, LOW);
        gps.get_m8(tx_loc);
        gps.disable(); // Disable GPS to save power during transmission
        telemetry.sendType1(call, tx_loc, 13);
    } else if (shouldTransmit && (rtc.getMinutes() % 10 == (telemetry.getMinute() + 2) % 10) && (rtc.getSeconds() == 1) && !telemetry.isTransmitting() && gps.isRTCSynced())
    {
        //lastMinute = gps.getMinute(); // Update last minute to prevent duplicate sends
        
        // Turn off the GPS module to save power
        //digitalWrite(GPS_ON, LOW);
        telemetry.sendBasic(tx_loc, gps.getAltitude(), sensors.getTemperature(), sensors.getVoltage(), gps.getSpeed());
    } else if (shouldTransmit && (rtc.getMinutes() % 10 == (telemetry.getMinute() + 4) % 10) && (rtc.getSeconds() == 1) && !telemetry.isTransmitting() && gps.isRTCSynced())
    {
        //lastMinute = gps.getMinute(); // Update last minute to prevent duplicate sends
        
        // Turn off the GPS module to save power
        //digitalWrite(GPS_ON, LOW);
        telemetry.sendExtended(tx_loc, sensors.getPressure(), gps.getSatellites());
    }
    else if ((lastPrintTime == 0 || millis() - lastPrintTime > 10000) && !telemetry.isTransmitting())
    {
        lastPrintTime = millis();
        DEBUG_PRINTLN("GPS Data:");
        DEBUG_PRINTF("Lat: %.6f, Lon: %.6f, Alt: %.2f, Speed: %.2f, Time: %02d:%02d:%02d, Satellites: %u\n",
                     gps.getLatitude(), gps.getLongitude(), gps.getAltitude(),
                     gps.getSpeed(), gps.getHour(), gps.getMinute(), gps.getSec(), gps.getSatellites());
    }
}