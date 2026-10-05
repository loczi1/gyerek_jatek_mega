#ifndef GYEREK_JATEK_BOARD_CONFIG_H
#define GYEREK_JATEK_BOARD_CONFIG_H

#include <Arduino.h>

namespace board {

constexpr uint8_t LedDataPin = 2;
constexpr uint8_t BuzzerPin = 5;
constexpr uint8_t GameButtonPin = 46;
constexpr uint8_t FirstDiagnosticPin = 46;
constexpr uint8_t LastDiagnosticPin = 52;
constexpr uint8_t AnalogInputPin = A0;
constexpr uint8_t EntropyInputPin = A1;
constexpr uint8_t StatusLedPin = LED_BUILTIN;
constexpr unsigned long SerialBaud = 9600;
constexpr uint32_t AppTickMs = 5;
constexpr uint32_t AudioTickMs = 5;
constexpr uint32_t DiagnosticRefreshMs = 250;

constexpr uint8_t IdleBrightness = 10;
constexpr uint8_t GameBrightness = 80;
constexpr uint8_t FeedbackBrightness = 255;
// Conservative software estimate, not a substitute for a correctly rated,
// externally powered 5 V LED supply and a common ground with the Mega.
constexpr uint16_t MaximumLedMilliamps = 1000;

} // namespace board

#endif