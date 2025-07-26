#include "telemetry.h"

Telemetry::Telemetry(Si5351& si5351) : si5351(si5351)
{
}

void Telemetry::init()
{
    DEBUG_PRINTLN("Starting Telemetry...");
    // Set up si5351
    Wire.setSDA(PB7);
    Wire.setSCL(PB6);
    Wire.begin();
    pinMode(VFO_VCC_ON, INPUT_PULLDOWN);
    delay(500);
    si5351.init(SI5351_CRYSTAL_LOAD_0PF, 26000000UL, 0);
    // Set CLK0 output
    si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_8MA); // Set for max power if desired
    si5351.output_enable(SI5351_CLK0, 0); // Disable the clock initially

    // Get channel details
    cd = WsprChannelMap::GetChannelDetails(BAND, CHANNEL);
}

void Telemetry::sendType1(char call[], char loc[], uint8_t dbm) {
    DEBUG_PRINTLN("Sending Type 1 WSPR frame...");
    DEBUG_PRINTLN(call);
    DEBUG_PRINTLN(loc);
    set_tx_buffer(call, loc, dbm);
    tx(cd.freq);
}

void Telemetry::sendBasic(char loc[], int32_t altitudeMeters, int8_t temperatureCelsius, double voltageVolts, uint8_t speedKnots) {
    DEBUG_PRINTLN("Sending Basic Telemetry frame...");
    DEBUG_PRINTLN(loc);

    // Create the message encoder
    WsprMessageTelemetryBasic msg;
    char id56[3];
    id56[0] = loc[4];
    id56[1] = loc[5];
    id56[2] = '\0';
    DEBUG_PRINTLN(id56);
    DEBUG_PRINTLN("String Length: " + String(strlen(id56)));

    // Set the telemetry fields
    msg.SetGrid56(id56);
    msg.SetAltitudeMeters(altitudeMeters);
    msg.SetTemperatureCelsius(temperatureCelsius);
    msg.SetVoltageVolts(voltageVolts);
    msg.SetSpeedKnots(speedKnots);
    msg.SetGpsIsValid(true); // Assume GPS is valid as we use it to tell time

    // Set Encoding parameter
    msg.SetId13(cd.id13);

    // Report the parameters passed, and if they got automatically clamped
    DEBUG_PRINTLN("Encoded WSPR BasicTelemetry Type1 Message for:");
    DEBUG_PRINT("id13      : input as  : "); DEBUG_PRINTLN(cd.id13);
    DEBUG_PRINT("          : clamped to: "); DEBUG_PRINTLN(msg.GetId13());
    DEBUG_PRINT("grid56    : input as  : "); DEBUG_PRINTLN(id56);
    DEBUG_PRINT("          : clamped to: "); DEBUG_PRINTLN(msg.GetGrid56());
    DEBUG_PRINT("altM      : input as  : "); DEBUG_PRINTLN(altitudeMeters);
    DEBUG_PRINT("          : clamped to: "); DEBUG_PRINTLN(msg.GetAltitudeMeters());
    DEBUG_PRINT("tempC     : input as  : "); DEBUG_PRINTLN(temperatureCelsius);
    DEBUG_PRINT("          : clamped to: "); DEBUG_PRINTLN(msg.GetTemperatureCelsius());
    DEBUG_PRINT("voltage   : input as  : "); DEBUG_PRINTLN(voltageVolts);
    DEBUG_PRINT("          : clamped to: "); DEBUG_PRINTLN(msg.GetVoltageVolts());
    DEBUG_PRINT("speedKnots: input as  : "); DEBUG_PRINTLN(speedKnots);
    DEBUG_PRINT("          : clamped to: "); DEBUG_PRINTLN(msg.GetSpeedKnots());
    DEBUG_PRINT("gpsValid  : input as  : "); DEBUG_PRINTLN(true);
    DEBUG_PRINT("          : clamped to: "); DEBUG_PRINTLN(msg.GetGpsIsValid());
    DEBUG_PRINTLN();


    // Do encoding
    msg.Encode();

    // Extract the WSPR Type1 Message fields from the encoder
    const char *callsign = msg.GetCallsign();
    const char *grid4    = msg.GetGrid4();
    uint8_t     powerDbm = msg.GetPowerDbm();

    DEBUG_PRINTLN("Callsign: " + String(callsign));
    DEBUG_PRINTLN("Grid4: " + String(grid4));
    DEBUG_PRINTLN("Power (dBm): " + String(powerDbm));

    set_tx_buffer(callsign, grid4, powerDbm);
    tx(cd.freq);
}

void Telemetry::tx(unsigned long freq)
{
    DEBUG_PRINTLN("Starting WSPR TX!");
    uint8_t i;

    // Reset the tone to the base frequency and turn on the output
    si5351.output_enable(SI5351_CLK0, 1);

    for (i = 0; i < WSPR_SYMBOL_COUNT; i++)
    {
        si5351.set_freq((freq * 100) + (tx_buffer[i] * WSPR_TONE_SPACING), SI5351_CLK0);
        delay(WSPR_DELAY);
    }

    // Turn off the output
    si5351.output_enable(SI5351_CLK0, 0);
    DEBUG_PRINTLN("DONE!");
}

void Telemetry::set_tx_buffer(const char* call, const char* loc, uint8_t dbm) {
    jtencode.wspr_encode(call, loc, dbm, tx_buffer);
}