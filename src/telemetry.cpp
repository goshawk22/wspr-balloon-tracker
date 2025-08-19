#include "telemetry.h"

// Add static instance pointer
Telemetry* Telemetry::instance = nullptr;

Telemetry::Telemetry(Si5351& si5351) : si5351(si5351), transmitting(false), current_symbol(0)
{
    instance = this; // Set static instance for callback
}

void Telemetry::init()
{
    DEBUG_PRINTLN("[Telemetry] Starting Telemetry...");
    DEBUG_PRINTLN("[Telemetry] Powering on Si5351...");
    pinMode(VFO_VCC_ON, OUTPUT);
    digitalWrite(VFO_VCC_ON, LOW); // Enable VFO power
    delay(1000);
    DEBUG_PRINTLN("[Telemetry] SI5351 Powered on...");
    // delay(500);
    // Set up si5351
    Wire.setSDA(PB7);
    Wire.setSCL(PB6);
    // Wire.begin();
    delay(1000); // Allow Si5351 to power up
    si5351.init(SI5351_CRYSTAL_LOAD_0PF, 26000000UL, 0);
    // Set CLK0 output
    si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_8MA);
    si5351.output_enable(SI5351_CLK0, 0);

    DEBUG_PRINTLN("[Telemetry] SI5351 Initialized...");
    delay(1000);
    // Initialize hardware timer
    timer = new HardwareTimer(TIM1); // Use TIM1 or another available timer
    timer->setOverflow(WSPR_DELAY, MICROSEC_FORMAT); // Set period to WSPR_DELAY us
    timer->attachInterrupt(timerCallback);

    DEBUG_PRINTLN("[Telemetry] Hardware Timer Initialized...");
    delay(1000);
    // Get channel details
    cd = WsprChannelMap::GetChannelDetails(BAND, CHANNEL);
    DEBUG_PRINTLN("[Telemetry] Channel Configured");
    delay(1000);
    DEBUG_PRINTLN("[Telemetry] Telemetry Initialized");
}

void Telemetry::sendType1(char call[], char loc[], uint8_t dbm) {
    DEBUG_PRINTLN("[Telemetry] Sending Type 1 WSPR frame...");
    DEBUG_PRINTLN(call);
    DEBUG_PRINTLN(loc);
    // Ensure loc is only 6 characters (truncate if longer)
    char loc6[7];
    strncpy(loc6, loc, 6);
    loc6[6] = '\0';

    set_tx_buffer(call, loc6, dbm);
    tx(cd.freq);
}

void Telemetry::sendBasic(char loc[], int32_t altitudeMeters, int8_t temperatureCelsius, double voltageVolts, uint8_t speedKnots) {
    DEBUG_PRINTLN("[Telemetry] Sending Basic Telemetry frame...");
    DEBUG_PRINTLN(loc);

    // Create the message encoder
    WsprMessageTelemetryBasic msg;
    char id56[3];
    id56[0] = loc[4];
    id56[1] = loc[5];
    id56[2] = '\0';
    DEBUG_PRINTLN(id56);

    // Set the telemetry fields
    msg.SetGrid56(id56);
    msg.SetAltitudeMeters(altitudeMeters);
    msg.SetTemperatureCelsius(temperatureCelsius);
    msg.SetVoltageVolts(voltageVolts + 2.0); // offset so it fits the range
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

    // Add debug output for the raw encoded buffer
    set_tx_buffer(callsign, grid4, powerDbm);

    tx(cd.freq);
}

void Telemetry::sendExtended(char loc[], float pressure, uint8_t satellites) {
    DEBUG_PRINTLN("[Telemetry] Sending Extended Telemetry frame...");
    DEBUG_PRINTLN(loc);

    WsprMessageTelemetryExtendedUserDefined<6> msg;
    msg.DefineField("Pressure", 0, 110000, 100);
    msg.DefineField("Loc", 0, 99, 1);
    msg.DefineField("Uptime", 0, 1000, 10);
    msg.DefineField("Sats", 0, 40, 1);

    // Convert last 2 characters (positions 6 and 7) to integer
    // For 8-character grid: AA##aa##, positions 6-7 are the last ##
    int loc_int = (loc[6] - '0') * 10 + (loc[7] - '0');

    // set values
    msg.Set("Pressure", pressure * 100); // Convert to Pa
    msg.Set("Loc", loc_int); // Use the converted integer
    msg.Set("Uptime", (millis() / 1000) / 60);
    msg.Set("Sats", satellites);

    // encode
    msg.SetId13(cd.id13);
    msg.SetHdrSlot(2); // +4 minutes
    msg.Encode();

    // Extract the WSPR Type1 Message fields from the encoder
    const char *callsign = msg.GetCallsign();
    const char *grid4    = msg.GetGrid4();
    uint8_t     powerDbm = msg.GetPowerDbm();

    DEBUG_PRINTLN("Callsign: " + String(callsign));
    DEBUG_PRINTLN("Grid4: " + String(grid4));
    DEBUG_PRINTLN("Power (dBm): " + String(powerDbm));

    // Add debug output for the raw encoded buffer
    set_tx_buffer(callsign, grid4, powerDbm);

    sentExtended = true; // Set flag to indicate extended telemetry has been sent

    tx(cd.freq);
}

void Telemetry::tx(unsigned long freq)
{
    if (transmitting) {
        // We shouldn't get here anyway but just in case
        DEBUG_PRINTLN("[Telemetry] Already transmitting, skipping...");
        return;
    }

    DEBUG_PRINTLN("[Telemetry] Starting WSPR TX!");

    // Initialize transmission state
    transmitting = true;
    current_symbol = 0;
    base_freq = freq;
    
    // Set first symbol and enable output
    si5351.set_freq((base_freq * 100) + (tx_buffer[0] * WSPR_TONE_SPACING), SI5351_CLK0);
    si5351.output_enable(SI5351_CLK0, 1);
    
    // Start the timer
    timer->resume();
}

// Static timer callback function
void Telemetry::timerCallback()
{
    if (!instance || !instance->transmitting) {
        return;
    }
    
    instance->current_symbol++;
    
    if (instance->current_symbol >= WSPR_SYMBOL_COUNT) {
        // Transmission complete
        instance->si5351.output_enable(SI5351_CLK0, 0);
        instance->timer->pause();
        instance->transmitting = false;
        DEBUG_PRINTLN("[Telemetry] DONE!");
    } else {
        // Set next symbol frequency
        instance->si5351.set_freq(
            (instance->base_freq * 100) + (instance->tx_buffer[instance->current_symbol] * WSPR_TONE_SPACING), 
            SI5351_CLK0
        );
    }
}

void Telemetry::set_tx_buffer(const char* call, const char* loc, uint8_t dbm) {
    jtencode.wspr_encode(call, loc, dbm, tx_buffer);
}