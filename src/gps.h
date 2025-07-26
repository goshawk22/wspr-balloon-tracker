#ifndef GPS_H
#define GPS_H

#include "config.h"
#include "console.h"
#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#define GPS_FIX_AGE 5000 // Maximum age of GPS fix in milliseconds
#define NMEA_CONFIG_STRING "$PCAS03,1,0,0,1,0,0,0,0,0,,,0,0*02"
#define NMEA_UPDATE_RATE "$PCAS02,500*1A"
#define GPS_BAUD_CONFIG_STRING "$PCAS01,5*19"

class GPS
{
public:
    GPS();
    void begin();
    void update();

    // Accessors
    bool isValidFix() const
    {
        return gps.location.isValid() && gps.location.age() < GPS_FIX_AGE;
    }
    double getLatitude() const
    {
        return latitude;
    }
    double getLongitude() const
    {
        return longitude;
    }
    double getSpeed() const
    {
        return speed;
    }
    double getAltitude() const
    {
        return altitude;
    }
    uint8_t getSatellites() const
    {
        return satellites;
    }
    uint32_t getTime() const
    {
        return time;
    }
    uint8_t getHour() const
    {
        return hour;
    }
    uint8_t getMinute() const
    {
        return minute;
    }
    uint8_t getSec() const
    {
        return second;
    }
    void get_m6(char *loc) {
        //strcpy(loc, locator);
        strcpy(loc, "IO90WX");
    }
    uint8_t getAge() const {
        return age;
    }
    void setUpdated(bool value) {
        updated = value;
    }
    bool isUpdated() const {
        return updated;
    }

private:
    TinyGPSPlus gps;

    char letterize(int x);
    void update_mh_6(double lat, double lon);

    // Cached data
    double latitude;
    double longitude;
    double altitude;
    char locator[6];
    uint8_t satellites;
    int speed;
    uint32_t time;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t age;
    bool updated;
};

#endif