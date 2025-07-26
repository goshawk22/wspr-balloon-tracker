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

SoftwareSerial SerialPC(PC_SERIAL_RX, PC_SERIAL_TX);

uint32_t lastPrintTime = 0;
uint8_t lastMinute = 61; // Initialize to a value that won't match the first minute check

void setup()
{
    SerialPC.begin(9600);
    delay(1000);
    DEBUG_PRINTLN("Starting...");

    gps.begin();
    telemetry.init();
    sensors.begin();
}

void loop()
{
    gps.update();
    sensors.update();
    if ((gps.getMinute() % 10 == telemetry.getMinute()) && (gps.getSec() == 1) && (lastMinute != gps.getMinute()))
    {
        lastMinute = gps.getMinute(); // Update last minute to prevent duplicate sends
        char loc[6];
        gps.get_m6(loc);
        telemetry.sendType1(call, loc, 13);
    } else if ((gps.getMinute() % 10 == (telemetry.getMinute() + 2) % 10) && (gps.getSec() == 1) && (lastMinute != gps.getMinute()))
    {
        lastMinute = gps.getMinute(); // Update last minute to prevent duplicate sends
        char loc[6];
        gps.get_m6(loc);
        telemetry.sendBasic(loc, gps.getAltitude(), sensors.getTemperature(), sensors.getVoltage(), gps.getSpeed());
    }
    else if (lastPrintTime == 0 || millis() - lastPrintTime > 10000)
    {
        lastPrintTime = millis();
        DEBUG_PRINTLN("GPS Data:");
        DEBUG_PRINTF("Lat: %.6f, Lon: %.6f, Alt: %.2f, Speed: %.2f, Time: %02d:%02d:%02d, Satellites: %u\n",
                     gps.getLatitude(), gps.getLongitude(), gps.getAltitude(),
                     gps.getSpeed(), gps.getHour(), gps.getMinute(), gps.getSec(), gps.getSatellites());
        DEBUG_PRINTLN(gps.getSatellites());
    }
    //delay(500);
}