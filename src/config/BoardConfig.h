#ifndef GYEREK_JATEK_BOARD_CONFIG_H
#define GYEREK_JATEK_BOARD_CONFIG_H

#include <Arduino.h>
#include "PinConfig.h"

namespace board {

constexpr uint8_t AnalogInputPin = A0;
constexpr uint8_t EntropyInputPin = A1;
constexpr uint8_t MenuJoystickPin = A0 + MenuJoystickChannel;
constexpr unsigned long SerialBaud = 9600;
constexpr uint32_t AppTickMs = 5;
constexpr uint32_t AudioTickMs = 5;
constexpr uint32_t DiagnosticRefreshMs = 250;
// Return from Debug/nested menus after two minutes without menu interaction.
constexpr uint16_t MenuIdleTimeoutSeconds = 120;
constexpr uint32_t I2cTimeoutMicros = 25000;

static_assert(NUM_DIGITAL_PINS == DigitalPinCount + AnalogChannelCount,
              "Debug pin policy requires an Arduino Mega 2560");
static_assert(NUM_ANALOG_INPUTS == AnalogChannelCount, "Expected 16 analog channels");
static_assert(A0 == FirstAnalogGpioPin, "Unexpected Mega analog GPIO mapping");
static_assert(LED_BUILTIN == StatusLedPin, "Unexpected status LED pin");

constexpr uint8_t IdleBrightness = 10;
constexpr uint8_t GameBrightness = 80;
constexpr uint8_t FeedbackBrightness = 255;
// Conservative software estimate, not a substitute for a correctly rated,
// externally powered 5 V LED supply and a common ground with the Mega.
constexpr uint16_t MaximumLedMilliamps = 1000;

} // namespace board

#endif