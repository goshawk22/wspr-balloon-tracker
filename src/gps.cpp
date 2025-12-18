#include "gps.h"

GPS *GPS::instance = nullptr;

GPS::GPS()
{
    instance = this;
}

void GPS::begin()
{
    DEBUG_PRINTLN("[GPS] Powering on...");
    pinMode(GPS_VCC_ON, OUTPUT);
    digitalWrite(GPS_VCC_ON, LOW);
    delay(500); // Wait for GPS to stabilize
    pinMode(GPS_ON, OUTPUT);
    digitalWrite(GPS_ON, HIGH);
    delay(500); // Allow GPS to stabilize

    fix_start_time = millis();

    // Initialize the GPS serial port
    // clock must be in the range [3x baud rate..4096 x baud rate], so max clock is 9600 * 4096 = 39321600
    SerialLP1.setRx(GPS_SERIAL_RX);
    SerialLP1.setTx(GPS_SERIAL_TX);
    SerialLP1.begin(GPS_SERIAL_BAUD); // Set the baud rate for GPS
    delay(500); // Wait for GPS to initialize

    DEBUG_PRINTLN("[GPS] Setting up PPS interrupt...");
    setupPPS(); // Setup PPS pin and interrupt

    DEBUG_PRINTLN("[GPS] Initializing RTC...");
    STM32RTC &rtc = STM32RTC::getInstance();
    rtc.setClockSource(STM32RTC::HSE_CLOCK);
    rtc.begin();

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
    if (enabled) {
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
            // update the locator (now 10-figure Maidenhead)
            update_mh_10(gps.location.lat(), gps.location.lng());

            // Update fix time
            if (!updated) {
                fix_time = millis() - fix_start_time;
                blink(2, 250); // we got a fix
            }

            updated = true; // Mark as updated whenever we receive new GPS data
        }
    }
}

// The following were taken from https://github.com/knormoyle/rp2040_si5351_wspr/blob/main/tracker/mh_functions.cpp
// and adapted for 10 character Maidenhead locator format
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
// AA00BB1122
void GPS::update_mh_10(double lat, double lon)
{
    // Factors per Maidenhead precision step
    // Index 0: field (letters), 1: square (digits), 2: subsquare (letters),
    // 3: extended square (digits), 4: extended subsquare (digits for 9th/10th)
    double LON_F[] = {20, 2.0, 0.0833330, 0.008333, 0.0008333};
    double LAT_F[] = {10, 1.0, 0.0416665, 0.004166, 0.0004166};

    lon += 180;
    lat += 90;

    // Produce 10-character locator (positions 9 and 10 are digits 0-9)
    const int size = 10;
    for (int i = 0; i < size / 2; i++)
    {
        bool useDigits = (i % 2 == 1) || (i == 4); // digits for 3rd, 4th and 5th pairs
        if (useDigits)
        {
            locator[i * 2]     = (char)((int)(lon / LON_F[i]) + '0');
            locator[i * 2 + 1] = (char)((int)(lat / LAT_F[i]) + '0');
        }
        else
        {
            locator[i * 2]     = letterize((int)(lon / LON_F[i]));
            locator[i * 2 + 1] = letterize((int)(lat / LAT_F[i]));
        }
        lon = fmod(lon, LON_F[i]);
        lat = fmod(lat, LAT_F[i]);
    }
    locator[size] = 0; // null terminator
}

void GPS::setupPPS()
{
    uint32_t channel;
    // Automatically retrieve TIM instance and channel associated to pin
    // This is used to be compatible with all STM32 series automatically.
    TIM_TypeDef *Instance = (TIM_TypeDef *)pinmap_peripheral(digitalPinToPinName(GPS_PPS_PIN), PinMap_PWM);
    channel = STM_PIN_CHANNEL(pinmap_function(digitalPinToPinName(GPS_PPS_PIN), PinMap_PWM));

    // Setup a high-resolution timer for PPS timing
    pps_timer = new HardwareTimer(Instance);

    // With a PrescalerFactor = 1, the minimum frequency value to measure is : TIM counter clock / CCR MAX
    //  = (SystemCoreClock) / 65535
    // Example on Nucleo_L476RG with systemClock at 80MHz, the minimum frequency is around 1,2 khz
    // To reduce minimum frequency, it is possible to increase prescaler. But this is at a cost of precision.
    // The maximum frequency depends on processing of the interruption and thus depend on board used
    // Example on Nucleo_L476RG with systemClock at 80MHz the interruption processing is around 4,5 microseconds and thus Max frequency is around 220kHz
    uint32_t PrescalerFactor = 1;
    pps_timer->setPrescaleFactor(PrescalerFactor);

    // Configure rising edge detection to measure frequency
    pps_timer->setMode(channel, TIMER_INPUT_CAPTURE_RISING, GPS_PPS_PIN);
    // No need to set overflow for input capture mode; just attach the interrupt
    pps_timer->attachInterrupt(channel, ppsInterrupt);
    pps_timer->resume();

    // Attach interrupt to PPS pin - trigger on rising edge
    // attachInterrupt(digitalPinToInterrupt(GPS_PPS_PIN), ppsInterrupt, RISING);

    // pps_timer->resume();

    DEBUG_PRINTLN("[GPS] PPS synchronization enabled");
}

void GPS::ppsInterrupt()
{
    DEBUG_PRINTLN("[GPS] PPS interrupt received");
    if (instance)
    {
        instance->syncRTC();
        // GPS time here is still old, i.e. we haven't got the new data over serial yet.
        DEBUG_PRINTF("GPS Time: %02d:%02d:%02d.%02d\n",
                     instance->gps.time.hour(), instance->gps.time.minute(),
                     instance->gps.time.second(), instance->gps.time.centisecond());
        DEBUG_PRINTF("RTC Time: %02d:%02d:%02d.%02d\n",
                     STM32RTC::getInstance().getHours(),
                     STM32RTC::getInstance().getMinutes(),
                     STM32RTC::getInstance().getSeconds(),
                     STM32RTC::getInstance().getSubSeconds());
    }
}

void GPS::syncRTC() {
    if ((millis() - last_sync_time > 30000 || !rtc_synced) && gps.time.isValid() && gps.date.isValid()) {
        // Get GPS time
        int year = gps.date.year();
        int month = gps.date.month();
        int day = gps.date.day();
        int hour = gps.time.hour();
        int minute = gps.time.minute();
        int second = gps.time.second() + 1;

        // Normalize time to avoid rollover
        // We don't really care about hours, days etc
        if (second >= 60) {
            second -= 60;
            minute += 1;
        }
        if (minute >= 60) {
            minute -= 60;
        }
        
        // Set STM32 RTC
        STM32RTC& rtc = STM32RTC::getInstance();
        rtc.setTime(hour, minute, second);
        rtc.setDate(day, month, year);
        rtc.setSubSeconds(0); // Set sub-seconds to 0 for simplicity
        
        last_sync_time = millis(); // Reset sync timer
        DEBUG_PRINTLN("[GPS] RTC synchronized with GPS time");

        rtc_synced = true; // Set flag to indicate RTC has been synced
    }
}

void GPS::enable()
{
    if (enabled) {
        DEBUG_PRINTLN("[GPS] Already enabled.");
        return;
    }
    digitalWrite(GPS_ON, HIGH);
    delay(1000); // Allow GPS to power up
    fix_start_time = millis();
    updated = false; // Data is now stale
    pps_timer->resume(); // Resume PPS timer
    enabled = true;
    DEBUG_PRINTLN("[GPS] Enabled.");
}

void GPS::disable()
{
    if (!enabled) {
        DEBUG_PRINTLN("[GPS] Already disabled.");
        return;
    }
    pps_timer->pause(); // Pause PPS timer
    digitalWrite(GPS_ON, LOW);
    enabled = false;
    DEBUG_PRINTLN("[GPS] Disabled.");
}