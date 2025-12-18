#include "led.h"

// Keep a persistent handle to the HardwareTimer used for the LED so
// we can temporarily pause it while doing software-driven blinks.
static HardwareTimer *led_timer = nullptr;
static uint32_t led_channel = 0;

static volatile bool blinking = false;
static volatile uint8_t blink_remaining = 0;
// Default states
static uint32_t default_overflow_us = 2000000;
static uint32_t default_compare_us = 50;

// Restore the timer configuration used by setupLED
static void restore_timer_settings()
{
  if (!led_timer)
    return;
  led_timer->pause();
  led_timer->setOverflow(default_overflow_us, MICROSEC_FORMAT);
  led_timer->setCaptureCompare(led_channel, default_compare_us, MICROSEC_COMPARE_FORMAT);
  led_timer->resume();
}

void Update_IT_callback(void)
{
  if (blinking)
  {
    digitalWrite(LED, LOW);
    if (blink_remaining > 0)
    {
      blink_remaining--;
      if (blink_remaining == 0)
      {
        blinking = false;
        restore_timer_settings();
      }
    }
  }
  else
  {
    digitalWrite(LED, LOW);
  }
}

void Compare_IT_callback(void)
{
  digitalWrite(LED, HIGH);
}

void setupLED()
{
  uint32_t channel;
  // Automatically retrieve TIM instance and channel associated to pin
  // This is used to be compatible with all STM32 series automatically.
  TIM_TypeDef *Instance = (TIM_TypeDef *)pinmap_peripheral(digitalPinToPinName(LED), PinMap_PWM);
  channel = STM_PIN_CHANNEL(pinmap_function(digitalPinToPinName(LED), PinMap_PWM));

  // Setup a high-resolution timer for PPS timing
  led_timer = new HardwareTimer(Instance);
  led_channel = channel;

  // Configure rising edge detection to measure frequency
  led_timer->setMode(channel, TIMER_OUTPUT_COMPARE_PWM1, LED);
  led_timer->setOverflow(1000000, MICROSEC_FORMAT);
  led_timer->setCaptureCompare(channel, 50, MICROSEC_COMPARE_FORMAT);
  led_timer->attachInterrupt(Update_IT_callback);
  led_timer->attachInterrupt(channel, Compare_IT_callback);
  led_timer->resume();

  DEBUG_PRINTLN("[LED] Configured");
}

// Start a non-blocking fast blink sequence by reconfiguring the
// existing hardware timer to a short period. The timer interrupts
// already attached will toggle the LED; Update_IT_callback counts
// completed cycles and restores the original timer when done.
void blink(uint8_t count, uint32_t on_ms)
{
  if (count == 0)
    return;
  if (!led_timer)
    return;

  // Configure timer for blink period: on_ms high, on_ms low
  uint32_t on_us = (on_ms ? on_ms : 1) * 1000U;
  uint32_t period_us = on_us * 2;

  // Apply new timing
  led_timer->pause();
  led_timer->setOverflow(period_us, MICROSEC_FORMAT);
  led_timer->setCaptureCompare(led_channel, on_us, MICROSEC_COMPARE_FORMAT);
  blink_remaining = count;
  blinking = true;
  // Ensure the LED turns on at the start of the first period
  digitalWrite(LED, HIGH);
  led_timer->resume();
}
