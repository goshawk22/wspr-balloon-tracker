#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <WsprEncoded.h>
#include <si5351.h>
#include <JTEncode.h>
#include <rs_common.h>
#include <int.h>
#include <string.h>
#include <Wire.h>
#include "config.h"
#include "console.h"

class Telemetry
{
public:
    Telemetry(Si5351 &si5351);
    void init();
    void sendType1(char call[], char loc[], uint8_t dbm);
    void sendBasic(char loc[], int32_t altitudeMeters, int8_t temperatureCelsius, double voltageVolts, uint8_t speedKnots);
    void sendExtended(char loc[], float pressure, uint8_t satellites);
    uint32_t getFrequency() const
    {
        return cd.freq;
    }
    uint8_t getMinute() const
    {
        return cd.min;
    }
    bool isTransmitting() const {
        return transmitting;
    }
private:
    void tx(unsigned long freq);
    void set_tx_buffer(const char* call, const char* loc, uint8_t dbm);
    Si5351 si5351;
    JTEncode jtencode;
    uint8_t tx_buffer[255];
    WsprChannelMap::ChannelDetails cd;

    HardwareTimer *timer;
    volatile bool transmitting;
    volatile uint8_t current_symbol;
    volatile unsigned long base_freq;

    // Timer callback function (must be static)
    static void timerCallback();
    static Telemetry* instance; // For callback access
};

#endif