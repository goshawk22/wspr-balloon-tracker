#include "gps.h"

//HardwareSerial gpsSerial(GPS_SERIAL_RX, GPS_SERIAL_TX);

GPS::GPS()
{
}

void GPS::begin()
{
    //pinMode(GPS_VCC_ON, INPUT_PULLDOWN);
    //digitalWrite(GPS_VCC_ON, LOW);
    pinMode(GPS_ON, OUTPUT);
    digitalWrite(GPS_ON, HIGH);
    delay(1000); // Wait for GPS to power up
    DEBUG_PRINTLN("[GPS] Powering on...");
    // Initialize the serial port for GPS communication
    //gpsSerial.begin(9600);

    // Send NMEA configuration strings to the GPS
    //DEBUG_PRINTLN("[GPS] Configuring NMEA sentences...");
    //gpsSerial.println(NMEA_CONFIG_STRING);
    //delay(100);
    //gpsSerial.println(NMEA_UPDATE_RATE);
    //delay(100);
    //gpsSerial.println(GPS_BAUD_CONFIG_STRING);

    //gpsSerial.flush(); // Ensure all data is sent
    //gpsSerial.end(); // End the serial connection to reset baud rate

    //DEBUG_PRINTLN("[GPS] Reinitializing with new baud rate...");
    //gpsSerial.begin(GPS_BAUD); // Reinitialize with the new baud rate
    Serial2.setRx(GPS_SERIAL_RX);
    Serial2.setTx(GPS_SERIAL_TX);
    Serial2.begin(9600);
    delay(1000); // Wait for GPS to initialize
    
    /** Skip this for now
     *  Currently it's unclear whether these settings are preserved across resets
     *  and they aren't documented. Equally they aren't necessary.
     **/
    //DEBUG_PRINTLN("[GPS] Configuring NMEA sentences...");
    //Serial2.println(NMEA_CONFIG_STRING);
    //delay(100);
    //Serial2.println(NMEA_UPDATE_RATE);
    //delay(100);
    //Serial2.println(GPS_BAUD_CONFIG_STRING);
    //delay(100);
    //Serial2.flush(); // Ensure all data is sent
    //Serial2.end(); // End the serial connection to reset baud rate
    //Serial2.begin(GPS_BAUD); // Reinitialize with the new baud rate

    DEBUG_PRINTLN("[GPS] Initialized.");
}

void GPS::update()
{
    // Read data from the GPS serial port
    while (Serial2.available())
    {
        //DEBUG_PRINTLN("GPS Serial Data Available");
        char c = Serial2.read();
        //DEBUG_PRINT(c);
        //Serial.print(c);
        gps.encode(c);
        updated = true; // Mark as updated whenever we receive data
    }
    // Update cached data if a valid fix is available
    if (gps.location.isValid() && (gps.location.age() < GPS_FIX_AGE || updated == true))
    {
        // DEBUG_PRINTLN("[GPS] Valid fix received.");
        latitude = gps.location.lat();
        longitude = gps.location.lng();
        altitude = gps.altitude.meters();
        satellites = gps.satellites.value();
        speed = gps.speed.knots();
        time = gps.time.value();
        hour = gps.time.hour();
        minute = gps.time.minute();
        second = gps.time.second();

        // Also update the locator
        update_mh_6(latitude, longitude);
    }
    else
    {
        // DEBUG_PRINTLN("[GPS] No valid fix.");
        latitude = 0.0;
        longitude = 0.0;
        altitude = 0.0;
        satellites = gps.satellites.value(); // satellites will still be valid even if no fix
        speed = 0.0;
        time = gps.time.value(); // even if no fix, the time will still be updated
        hour = gps.time.hour();
        minute = gps.time.minute();
        second = gps.time.second();
    }
    age = gps.time.age();
    //DEBUG_PRINTLN(time);
    //DEBUG_PRINTLN(age);
}

// The following were taken from https://github.com/knormoyle/rp2040_si5351_wspr/blob/main/tracker/mh_functions.cpp

char GPS::letterize(int x) {
    // KC3LBR 07/23/24 alternate/redundant fix
    // (the increased resolution may preclude a need)
    // this clamps the returned characters at 'X' or lower.
    // Original code sometimes returned a invalid Y for 5th or 6 char,
    // because of no bounds check.
    if (x < 24) return (char) x + 65;
    else return (char) 23 + 65;
}

// call with double for more precision
// force size to be 6
// always return char[7] (null term)
// will always return upper case

// tinyGPS gives you doubles for lat long, so use doubles here
// only place we return a pointer to a static char array ! (locator)
void GPS::update_mh_6(double lat, double lon) {
    double LON_F[] = {20, 2.0, 0.0833330, 0.008333, 0.0003472083333333333};
    double LAT_F[] = {10, 1.0, 0.0416665, 0.004166, 0.0001735833333333333};
    int i;

    lon += 180;
    lat += 90;
    int size = 6;
    for (i = 0; i < size/2; i++) {
        if (i % 2 == 1) {
            locator[i*2]   = (char) (lon/LON_F[i] + '0');
            locator[i*2+1] = (char) (lat/LAT_F[i] + '0');
        } else {
            locator[i*2]   = letterize((int) (lon/LON_F[i]));
            locator[i*2+1] = letterize((int) (lat/LAT_F[i]));
        }
        lon = fmod(lon, LON_F[i]);
        lat = fmod(lat, LAT_F[i]);
    }
    locator[6] = 0;  // null term
}