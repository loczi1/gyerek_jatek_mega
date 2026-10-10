#ifndef GYEREK_JATEK_PIN_CONFIG_H
#define GYEREK_JATEK_PIN_CONFIG_H

#include <stdint.h>

// Portable Mega pin policy, shared by hardware setup and native diagnostics.
namespace board {

constexpr uint8_t LedDataPin = 2;
constexpr uint8_t BuzzerPin = 5;
constexpr uint8_t GameButtonPin = 46;
constexpr uint8_t MenuButtonPin = 53;
constexpr uint8_t FirstDiagnosticPin = 46;
constexpr uint8_t LastDiagnosticPin = 52;
constexpr uint8_t StatusLedPin = 13;
constexpr uint8_t DigitalPinCount = 54;
constexpr uint8_t AnalogChannelCount = 16;
constexpr uint8_t MenuJoystickChannel = 9;
constexpr uint8_t LcdI2cAddress = 0x27;

} // namespace board

#endif