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
    si5351.set_clock_pwr(SI5351_CLK0, 0);
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

void Telemetry::sendExtended(char loc[], uint8_t satellites, uint32_t fix_time) {
    DEBUG_PRINTLN("[Telemetry] Sending Extended Telemetry frame...");
    DEBUG_PRINTLN(loc);

    WsprMessageTelemetryExtendedUserDefined<6> msg;
    msg.DefineField("Loc", 0, 99, 1);
    msg.DefineField("Loc2", 0, 99, 1);
    msg.DefineField("Uptime", 0, 120, 10);
    msg.DefineField("Fixtime", 0, 24, 1);
    msg.DefineField("Sats", 0, 40, 1);

    // Convert last 2 characters (positions 6 and 7) to integer
    // For 8-character grid: AA##aa##, positions 6-7 are the last ##
    int loc_int = (loc[6] - '0') * 10 + (loc[7] - '0');

    int loc2_int = (loc[8] - '0') * 10 + (loc[9] - '0');

    // nonlinear encodings for msg
    int enc_fixtime = constrain((floor((1/(50+(50*(pow(2.71, ((-0.4*(fix_time/1000))+3)))))) + (fix_time/40000000))), 0, 24);

    // set values
    msg.Set("Loc", loc_int);
    msg.Set("Loc2", loc2_int);
    msg.Set("Uptime", (millis() / 1000) / 60);
    msg.Set("Fixtime", (enc_fixtime));
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

void Telemetry::sendSlot1_new(char loc[], double voltageVolts) {
    DEBUG_PRINTLN("[Telemetry] Sending Slot 1 frame...");
    DEBUG_PRINTLN(loc);

    // Debug output (one-line concatenation)
    DEBUG_PRINT("[Telemetry] Maidenhead 10 chars: ");
    for (int i = 0; i < 10; ++i) {
        DEBUG_PRINT(String(loc[i]));
    }
    DEBUG_PRINTLN("");

    // Convert each character to its integer value according to Maidenhead rules:
    // pos 0-1: letters A-R -> 0..17 (A=0)
    // pos 2-3: digits 0-9 -> 0..9
    // pos 4-5: letters a-x/A-X -> 0..23 (a=0)
    // pos 6-7: digits 0-9 -> 0..9
    // pos 8-9: letters a-x/A-X -> 0..23 (a=0)
    int v[6];

    // positions 4 and 5 (subsquare letters a-x)
    v[0] = tolower((int)loc[4]) - 'a';
    if (v[0] < 0) v[0] = 0; 
    if (v[0] > 23) v[0] = 23;
    v[1] = tolower((int)loc[5]) - 'a';
    if (v[1] < 0) v[1] = 0;
    if (v[1] > 23) v[1] = 23;

    // positions 6 and 7 (extended digits)
    v[2] = loc[6] - '0';
    if (v[2] < 0) v[2] = 0;
    if (v[2] > 9) v[2] = 9;
    v[3] = loc[7] - '0';
    if (v[3] < 0) v[3] = 0;
    if (v[3] > 9) v[3] = 9;

    // positions 8 and 9 (extended subsquare letters a-x)
    v[4] = tolower((int)loc[8]) - 'a';
    if (v[4] < 0) v[4] = 0;
    if (v[4] > 23) v[4] = 23;
    v[5] = tolower((int)loc[9]) - 'a';
    if (v[5] < 0) v[5] = 0;
    if (v[5] > 23) v[5] = 23;

    // Debug print numeric values
    DEBUG_PRINTLN("[Telemetry] Maidenhead numeric values:");
    for (int i = 0; i < 6; ++i) {
        DEBUG_PRINT("v"); DEBUG_PRINT(i); DEBUG_PRINT(": "); DEBUG_PRINTLN(v[i]);
    }

    WsprMessageTelemetryExtendedUserDefined<6> msg;
    msg.DefineField("grid5", 0, 23, 1);
    msg.DefineField("grid6", 0, 23, 1);
    msg.DefineField("grid7", 0, 9, 1);
    msg.DefineField("grid8", 0, 9, 1);
    msg.DefineField("grid9", 0, 23, 1);
    msg.DefineField("grid10", 0, 23, 1);

    msg.DefineField("Voltage", 2, 3.7, 0.1);

    msg.Set("grid5", v[0]);
    msg.Set("grid6", v[1]);
    msg.Set("grid7", v[2]);
    msg.Set("grid8", v[3]);
    msg.Set("grid9", v[4]);
    msg.Set("grid10", v[5]);

    // Set Voltage using the provided parameter
    msg.Set("Voltage", voltageVolts);

    // encode
    msg.SetId13(cd.id13);
    msg.SetHdrSlot(1); // +2 minutes
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

    //sentExtended = true; // Set flag to indicate extended telemetry has been sent

    tx(cd.freq);
}

void Telemetry::sendSlot2_new(int32_t altitudeMeters, uint8_t speedKPH, int8_t temperatureCelsius, uint8_t satellites) {
    DEBUG_PRINTLN("[Telemetry] Sending Slot 2 Telemetry frame...");

    WsprMessageTelemetryExtendedUserDefined<6> msg;
    msg.DefineField("Altitude", 0, 16000, 10);
    msg.DefineField("Speed", 0, 300, 2);
    msg.DefineField("Temperature", -70, 50, 1);
    msg.DefineField("Sats", 0, 16, 4);
    msg.DefineField("Uptime", 0, 60, 20);

    // set values
    msg.Set("Altitude", altitudeMeters); // m
    msg.Set("Speed", speedKPH); // kph
    msg.Set("Temperature", temperatureCelsius); // degC
    msg.Set("Sats", satellites);
    msg.Set("Uptime", (millis() / 1000) / 60);

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

    blink(3, 250); // we are about to transmit

    // Initialize transmission state
    transmitting = true;
    current_symbol = 0;
    base_freq = freq;
    
    // Set first symbol and enable output
    si5351.set_freq((base_freq * 100) + (tx_buffer[0] * WSPR_TONE_SPACING), SI5351_CLK0);
    si5351.set_clock_pwr(SI5351_CLK0, 1);
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
        instance->si5351.output_enable(SI5351_CLK1, 0);
        instance->si5351.set_clock_pwr(SI5351_CLK0, 0);
        instance->si5351.set_clock_pwr(SI5351_CLK1, 0);
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