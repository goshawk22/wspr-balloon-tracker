#include "telemetry.h"
#include "gps.h"
#include "config.h"
#include "console.h"
#include "sensors.h"
#include "led.h"
#include <IWatchdog.h>

GPS gps;
Si5351 si5351;
Telemetry telemetry(si5351);
Sensors sensors;

char call[] = CALLSIGN;

uint32_t lastPrintTime = 0;

char tx_loc[11]; // locator at start of type 1 frame, needs to stay the same for all 3 frames.

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

    setupLED();
    gps.begin();
    delay(1000);
    telemetry.init();
    sensors.begin();
    IWatchdog.reload();


    // System started
    blink(1, 1000);

#ifdef DEBUG
    telemetry.sendType1(call, tx_loc, 13);
#endif
}

void loop()
{
    IWatchdog.reload();
    gps.update();
    if (telemetry.isExtendedSent() && !telemetry.isTransmitting()) {
        gps.enable(); // Re-enable GPS if extended telemetry was sent
        telemetry.setExtendedSent(false); // Reset the flag
    }

    // Common preconditions for any transmission
    const bool secIsOne = (rtc.getSeconds() == 1);
    const bool notTransmitting = !telemetry.isTransmitting();
    const bool rtcSynced = gps.isRTCSynced();

    if (secIsOne && notTransmitting && rtcSynced) {
        // Compute the difference between the current 10-minute slot and the telemetry base minute
        int minuteMod = rtc.getMinutes() % 10;
        int baseMinute = telemetry.getMinute();
        int slotOffset = (minuteMod - baseMinute + 10) % 10; // 0..9

        switch (slotOffset) {
            case 0:
                // Type 1 packet: take fresh sensor measurements and a GPS fix
                sensors.update();
                gps.get_m10(tx_loc);
                gps.disable(); // Disable GPS to save power during transmission
                telemetry.sendType1(call, tx_loc, 13);
                break;

            case 2:
                // Slot 1
                telemetry.sendBasic(tx_loc, gps.getAltitude(), sensors.getIntTemperature(), sensors.getVoltage(), gps.getSpeed());
                break;

            case 4:
                // Slot 2
                telemetry.sendExtended(tx_loc, gps.getSatellites(), gps.getFixTime());
                break;

            default:
                // Not a transmit slot; fall through to other periodic work
                break;
        }
    } else if ((lastPrintTime == 0 || millis() - lastPrintTime > 10000) && !telemetry.isTransmitting())
    {
        lastPrintTime = millis();
        DEBUG_PRINTLN("GPS Data:");
        DEBUG_PRINTF("Lat: %.6f, Lon: %.6f, Alt: %.2f, Speed: %.2f, Time: %02d:%02d:%02d, Satellites: %u\n",
                     gps.getLatitude(), gps.getLongitude(), gps.getAltitude(),
                     gps.getSpeed(), gps.getHour(), gps.getMinute(), gps.getSec(), gps.getSatellites());
    }
}