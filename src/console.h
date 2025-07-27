#ifndef CONSOLE_H
#define CONSOLE_H
#include "config.h"
#include <SoftwareSerial.h>

//extern SoftwareSerial SerialPC;

#define SerialPC Serial1

#ifdef DEBUG
#ifndef BMP_DEBUG
#define DEBUG_SERIAL SerialPC
#define DEBUG_PRINTLN(x) DEBUG_SERIAL.println(x)
#define DEBUG_PRINT(x) DEBUG_SERIAL.print(x)
#define DEBUG_PRINTF(...) DEBUG_SERIAL.printf(__VA_ARGS__)
#else
#define DEBUG_PRINTLN(x)
#define DEBUG_PRINT(x)
#define DEBUG_PRINTF(...)
#endif
#else
#define DEBUG_PRINTLN(x)
#define DEBUG_PRINT(x)
#define DEBUG_PRINTF(...)
#endif
#ifdef VERBOSE
#ifdef DEBUG
#define VERBOSE_PRINTLN(x) DEBUG_SERIAL.println(x)
#define VERBOSE_PRINT(x) DEBUG_SERIAL.print(x)
#endif
#else
#define VERBOSE_PRINTLN(x)
#define VERBOSE_PRINT(x)
#endif

#endif