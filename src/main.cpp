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
char tx_loc[8]; // locator at start of type 1 frame, needs to stay the same for all 3 frames.

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
        
        // Get sensor measurements at the same time as GPS fix so all the data matches
        sensors.update();

        // Turn off the GPS module to save power
        digitalWrite(GPS_ON, LOW);
        gps.get_m6(tx_loc);
        telemetry.sendType1(call, tx_loc, 13);
    } else if ((gps.getMinute() % 10 == (telemetry.getMinute() + 2) % 10) && (gps.getSec() == 1) && (lastMinute != gps.getMinute()) && !telemetry.isTransmitting())
    {
        lastMinute = gps.getMinute(); // Update last minute to prevent duplicate sends
        
        // Turn off the GPS module to save power
        digitalWrite(GPS_ON, LOW);
        telemetry.sendBasic(tx_loc, gps.getAltitude(), sensors.getTemperature(), sensors.getVoltage(), gps.getSpeed());
    } else if ((gps.getMinute() % 10 == (telemetry.getMinute() + 4) % 10) && (gps.getSec() == 1) && (lastMinute != gps.getMinute()) && !telemetry.isTransmitting())
    {
        lastMinute = gps.getMinute(); // Update last minute to prevent duplicate sends
        
        // Turn off the GPS module to save power
        digitalWrite(GPS_ON, LOW);
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