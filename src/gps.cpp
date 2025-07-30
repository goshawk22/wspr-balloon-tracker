#include "gps.h"

GPS::GPS()
{
}

void GPS::begin()
{
    // pinMode(GPS_VCC_ON, INPUT_PULLDOWN);
    // digitalWrite(GPS_VCC_ON, LOW);
    pinMode(GPS_ON, OUTPUT);
    digitalWrite(GPS_ON, HIGH);
    delay(1000); // Wait for GPS to power up
    DEBUG_PRINTLN("[GPS] Powering on...");

    delay(500); // Allow GPS to stabilize

    // Initialize the GPS serial port
    // clock must be in the range [3x baud rate..4096 x baud rate], so max clock is 9600 * 4096 = 39321600
    SerialLP1.setRx(GPS_SERIAL_RX);
    SerialLP1.setTx(GPS_SERIAL_TX);
    SerialLP1.begin(GPS_SERIAL_BAUD); // Set the baud rate for GPS
    delay(1000); // Wait for GPS to initialize

    /** Skip this for now
     *  Currently it's unclear whether these settings are preserved across resets
     *  and they aren't documented. Equally they aren't necessary.
     **/
    // DEBUG_PRINTLN("[GPS] Configuring NMEA sentences...");
    // Serial2.println(NMEA_CONFIG_STRING);
    // delay(100);
    // Serial2.println(NMEA_UPDATE_RATE);
    // delay(100);
    // Serial2.println(GPS_BAUD_CONFIG_STRING);
    // delay(100);
    // Serial2.flush(); // Ensure all data is sent
    // Serial2.end(); // End the serial connection to reset baud rate
    // Serial2.begin(GPS_BAUD); // Reinitialize with the new baud rate

    DEBUG_PRINTLN("[GPS] Initialized.");
}


void GPS::update()
{
    // Read data from the GPS serial port
    while (SerialLP1.available())
    {
        // DEBUG_PRINTLN("GPS Serial Data Available");
        char c = SerialLP1.read();
        //DEBUG_PRINT(c);
        // Serial.print(c);
        gps.encode(c);
    }
    // Update cached data if a valid fix is available
    if (gps.speed.isUpdated() && gps.satellites.isUpdated()) // ensures that GGA and RMC sentences have been received
    {
        // update the locator
        update_mh_8(gps.location.lat(), gps.location.lng());

        updated = true; // Mark as updated whenever we receive new GPS data
    }
}

// The following were taken from https://github.com/knormoyle/rp2040_si5351_wspr/blob/main/tracker/mh_functions.cpp
// and adapted for 8 character Maidenhead locator format
char GPS::letterize(int x)
{
    // KC3LBR 07/23/24 alternate/redundant fix
    // (the increased resolution may preclude a need)
    // this clamps the returned characters at 'X' or lower.
    // Original code sometimes returned a invalid Y for 5th or 6 char,
    // because of no bounds check.
    if (x < 24)
        return (char)x + 65;
    else
        return (char)23 + 65;
}

// call with double for more precision
// force size to be 8
// always return char[9] (null term)
// will always return upper case

// tinyGPS gives you doubles for lat long, so use doubles here
// only place we return a pointer to a static char array ! (locator)
void GPS::update_mh_8(double lat, double lon)
{
    double LON_F[] = {20, 2.0, 0.0833330, 0.008333, 0.0003472083333333333, 0.000014467};
    double LAT_F[] = {10, 1.0, 0.0416665, 0.004166, 0.0001735833333333333, 0.0000072333};
    int i;

    lon += 180;
    lat += 90;
    int size = 8;
    for (i = 0; i < size / 2; i++)
    {
        if (i % 2 == 1)
        {
            locator[i * 2] = (char)(lon / LON_F[i] + '0');
            locator[i * 2 + 1] = (char)(lat / LAT_F[i] + '0');
        }
        else
        {
            locator[i * 2] = letterize((int)(lon / LON_F[i]));
            locator[i * 2 + 1] = letterize((int)(lat / LAT_F[i]));
        }
        lon = fmod(lon, LON_F[i]);
        lat = fmod(lat, LAT_F[i]);
    }
    locator[8] = 0; // null term
}