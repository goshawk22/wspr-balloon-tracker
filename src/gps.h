#ifndef GPS_H
#define GPS_H

#include "config.h"
#include "console.h"
#include <TinyGPS++.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <HardwareTimer.h>
#include <STM32RTC.h>

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
    void setupPPS();
    static void ppsInterrupt();
    void syncRTC();
    void enable();
    void disable();

    // Accessors
    bool isValidFix()
    {
        return gps.location.isValid() && gps.location.age() < GPS_FIX_AGE;
    }
    double getLatitude()
    {
        return gps.location.lat();
    }
    double getLongitude()
    {
        return gps.location.lng();
    }
    double getSpeed()
    {
        return gps.speed.knots();
    }
    double getAltitude()
    {
        return gps.altitude.meters();
    }
    uint8_t getSatellites()
    {
        return gps.satellites.value();
    }
    uint32_t getTime()
    {
        return gps.time.value();
    }
    uint8_t getHour()
    {
        return gps.time.hour();
    }
    uint8_t getMinute()
    {
        return gps.time.minute();
    }
    uint8_t getSec()
    {
        return gps.time.second();
    }
    void get_m8(char *loc)
    {
        strcpy(loc, locator);
        // strcpy(loc, "IO90WX");
    }
    uint8_t getAge()
    {
        return gps.time.age();
    }
    void setUpdated(bool value)
    {
        updated = value;
    }
    bool isUpdated() const
    {
        return updated;
    }
    bool isRTCSynced() const
    {
        return rtc_synced;
    }

private:
    TinyGPSPlus gps;

    char letterize(int x);
    void update_mh_8(double lat, double lon);

    // Cached data
    double latitude;
    double longitude;
    double altitude;
    char locator[8];
    uint8_t satellites;
    int speed;
    uint32_t time;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t age;
    bool updated;
    bool enabled = true;

    // PPS interrupt handling
    static GPS *instance;
    HardwareTimer *pps_timer = nullptr;
    uint32_t last_sync_time = 0; // Time since last PPS sync in milliseconds
    bool rtc_synced = false; // Flag to indicate if RTC has been synced with GPS
};

#endif