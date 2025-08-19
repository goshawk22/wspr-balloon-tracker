#define PC_SERIAL_TX PA14
#define PC_SERIAL_RX PA15 // not used
#define PC_SERIAL_BAUD 115200

#define GPS_SERIAL_TX PA2
#define GPS_SERIAL_RX PA3
#define GPS_VCC_ON PA6
#define GPS_ON PA5
#define GPS_SERIAL_BAUD 9600
#define GPS_PPS_PIN PB0_ALT1

#define VFO_SDA PB7
#define VFO_SCL PB6
#define VFO_VCC_ON PB5

#define TCXO_FREQ 26000000UL

#define WSPR_TONE_SPACING 146          // ~1.46 Hz
#define WSPR_DELAY 683000          // Delay value for WSPR in us
#define WSPR_DEFAULT_FREQ 14097200UL

#define BAND "10m"
#define CHANNEL 226

#define CALLSIGN "M7GAQ"

#define ADC_PIN PB1
#define VOLTAGE_DIVIDER_MULTIPLIER 2.0

#define THRESHOLD_VOLTAGE 2.2 // Minimum voltage to start