#include "Diagnostics.h"
#include "config/BoardConfig.h"
#include <IoAbstraction.h>
#include <AnalogDeviceAbstraction.h>
#include <stdio.h>

namespace diagnostics {

void begin() {
    // Only pins actually used as button inputs get pull-ups. Leave UART, I2C,
    // LED data and all unrelated pins to their own peripherals.
    for (uint8_t pin = board::FirstDiagnosticPin; pin <= board::LastDiagnosticPin; ++pin)
        pinMode(pin, INPUT_PULLUP);
    pinMode(board::BuzzerPin, OUTPUT);
    digitalWrite(board::BuzzerPin, LOW);
    pinMode(board::StatusLedPin, OUTPUT);
    setOutput(false);
    internalAnalogDevice().initPin(board::AnalogInputPin, DIR_IN);
}

uint8_t pressedMask() {
    uint8_t mask = 0;
    for (uint8_t pin = board::FirstDiagnosticPin; pin <= board::LastDiagnosticPin; ++pin) {
        if (digitalRead(pin) == LOW)
            mask |= static_cast<uint8_t>(1U << (pin - board::FirstDiagnosticPin));
    }
    return mask;
}

void formatDigitalInputs(uint8_t mask, char* buffer, size_t capacity) {
    if (capacity == 0) return;
    if (mask == 0) {
        snprintf(buffer, capacity, "NINCS");
    } else if ((mask & static_cast<uint8_t>(mask - 1)) != 0) {
        snprintf(buffer, capacity, "DUPLA!");
    } else {
        uint8_t offset = 0;
        while ((mask & 1U) == 0) {
            mask >>= 1;
            ++offset;
        }
        snprintf(buffer, capacity, "%u", static_cast<unsigned int>(board::FirstDiagnosticPin + offset));
    }
}

void formatAnalogInput(char* buffer, size_t capacity) {
    if (capacity == 0) return;
    snprintf(buffer, capacity, "%u", static_cast<unsigned int>(
        internalAnalogDevice().getCurrentValue(board::AnalogInputPin)));
}

void setOutput(bool enabled) {
    digitalWrite(board::StatusLedPin, enabled ? HIGH : LOW);
}

} // namespace diagnostics