#include "telemetry.h"
#include "gps.h"
#include "config.h"
#include "console.h"
#include "sensors.h"

GPS gps;
Si5351 si5351;
Telemetry telemetry(si5351);
Sensors sensors;

char call[] = CALLSIGN;

uint32_t lastPrintTime = 0;
uint8_t lastMinute = 61; // Initialize to a value that won't match the first minute check

void setup()
{
#ifndef BMP_DEBUG
    // Initialize the GPS serial port
    SerialPC.setRx(PC_SERIAL_RX);
    SerialPC.setTx(PC_SERIAL_TX);
    SerialPC.begin(PC_SERIAL_BAUD); // Set the baud rate for SerialPC
#endif
    delay(1000);
    DEBUG_PRINTLN("Starting...");

    gps.begin();
    telemetry.init();
    sensors.begin();
}

void loop()
{
    gps.update();
    if ((gps.getMinute() % 10 == telemetry.getMinute()) && (gps.getSec() == 1) && (lastMinute != gps.getMinute()) && !telemetry.isTransmitting())
    {
        lastMinute = gps.getMinute(); // Update last minute to prevent duplicate sends

        // Turn off the GPS module to save power
        digitalWrite(GPS_ON, LOW);
        char loc[6];
        gps.get_m6(loc);
        telemetry.sendType1(call, loc, 13);
        // Turn on the GPS module again
        digitalWrite(GPS_ON, HIGH);
    } else if ((gps.getMinute() % 10 == (telemetry.getMinute() + 2) % 10) && (gps.getSec() == 1) && (lastMinute != gps.getMinute()) && !telemetry.isTransmitting())
    {
        sensors.update();
        lastMinute = gps.getMinute(); // Update last minute to prevent duplicate sends
        
        // Turn off the GPS module to save power
        digitalWrite(GPS_ON, LOW);
        char loc[6];
        gps.get_m6(loc);
        telemetry.sendBasic(loc, gps.getAltitude(), sensors.getTemperature(), sensors.getVoltage(), gps.getSpeed());
        // Turn on the GPS module again
        digitalWrite(GPS_ON, HIGH);
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