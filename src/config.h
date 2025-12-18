#define PC_SERIAL_TX PA14
#define PC_SERIAL_RX PA15 // not used
#define PC_SERIAL_BAUD 115200

#define GPS_SERIAL_TX PA2
#define GPS_SERIAL_RX PA3
#define GPS_VCC_ON PA7
#define GPS_ON PA5
#define GPS_RST PA6
#define GPS_SERIAL_BAUD 9600
#define GPS_PPS_PIN PB0_ALT1

#define VFO_SDA PB7
#define VFO_SCL PB6
#define VFO_VCC_ON PB3

#define SENSOR_SDA PA12
#define SENSOR_SCL PA11
#define SENSOR_VCC PC6

// misc definitions
#define ADC_PIN PA0
#define LED PA1
#define CHG_EN PA8

#define TCXO_FREQ 26000000UL

#define WSPR_TONE_SPACING 146          // ~1.46 Hz
#define WSPR_DELAY 683000          // Delay value for WSPR in us

#define BAND "17m"
#define CHANNEL 265

#define CALLSIGN "M7GAQ"

#define VIN_ADC_PIN PB1
#define VOLTAGE_DIVIDER_MULTIPLIER 2.0

//#define HAS_PRESSURE_SENSOR
