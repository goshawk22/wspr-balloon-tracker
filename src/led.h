#ifndef LED_H
#define LED_H

#include "config.h"
#include "console.h"
#include <HardwareTimer.h>
#include <Arduino.h>

void setupLED();
// Blink the LED quickly a number of times.
// - `count`: number of on/off cycles
// - `on_ms`: milliseconds the LED stays on (default 25 ms)
// Starts a non-blocking fast blink sequence. Returns immediately.
void blink(uint8_t count, uint32_t on_ms = 25);

#endif