#include "InputMonitor.h"
#include <stdio.h>

namespace debug {

DigitalRole digitalRole(uint8_t pin) {
    if (pin >= board::DigitalPinCount) return DigitalRole::Invalid;
    if (pin == 0 || pin == 1) return DigitalRole::Serial;
    if (pin == 20 || pin == 21) return DigitalRole::I2c;
    if (pin == board::LedDataPin) return DigitalRole::LedData;
    if (pin == board::BuzzerPin) return DigitalRole::Buzzer;
    if (pin == board::StatusLedPin) return DigitalRole::StatusLed;
    if (pin == board::GameButtonPin) return DigitalRole::GameButton;
    if (pin == board::MenuButtonPin) return DigitalRole::MenuButton;
    return DigitalRole::Input;
}

bool canReadDigital(uint8_t pin) {
    const DigitalRole role = digitalRole(pin);
    return role == DigitalRole::Input || role == DigitalRole::GameButton ||
           role == DigitalRole::MenuButton;
}

InputMonitor::InputMonitor(DigitalReader readDigital, AnalogReader readAnalog)
    : readDigital_(readDigital), readAnalog_(readAnalog) {
    resetSamples();
}

void InputMonitor::resetSamples() {
    for (uint8_t pin = 0; pin < board::DigitalPinCount; ++pin)
        digitalValues_[pin] = canReadDigital(pin) ? DigitalState::Pending : DigitalState::Reserved;
    for (uint8_t channel = 0; channel < board::AnalogChannelCount; ++channel)
        analogValues_[channel] = AnalogPending;
    nextDigital_ = 0;
    nextAnalog_ = 0;
}

void InputMonitor::setEnabled(bool enabled) {
    if (enabled == enabled_) return;
    enabled_ = enabled;
    if (enabled_) resetSamples();
}

void InputMonitor::update() {
    if (!enabled_) return;
    for (uint8_t count = 0; count < DigitalSamplesPerTick; ++count) {
        if (canReadDigital(nextDigital_) && readDigital_)
            digitalValues_[nextDigital_] = readDigital_(nextDigital_);
        nextDigital_ = static_cast<uint8_t>((nextDigital_ + 1) % board::DigitalPinCount);
    }
    if (readAnalog_) analogValues_[nextAnalog_] = readAnalog_(nextAnalog_);
    nextAnalog_ = static_cast<uint8_t>((nextAnalog_ + 1) % board::AnalogChannelCount);
}

DigitalState InputMonitor::digitalValue(uint8_t pin) const {
    return pin < board::DigitalPinCount ? digitalValues_[pin] : DigitalState::Reserved;
}

uint16_t InputMonitor::analogValue(uint8_t channel) const {
    return channel < board::AnalogChannelCount ? analogValues_[channel] : AnalogPending;
}

void InputMonitor::formatDigitalName(uint8_t pin, char* buffer, size_t capacity) {
    if (!buffer || capacity == 0) return;
    if (pin >= board::DigitalPinCount) {
        buffer[0] = '\0';
        return;
    }
    const char* suffix = pin == board::GameButtonPin ? " Jatek" :
        (pin == board::MenuButtonPin ? " Menu" : "");
    snprintf(buffer, capacity, "D%u%s", static_cast<unsigned int>(pin), suffix);
}

void InputMonitor::formatAnalogName(uint8_t channel, char* buffer, size_t capacity) {
    if (!buffer || capacity == 0) return;
    if (channel >= board::AnalogChannelCount) {
        buffer[0] = '\0';
        return;
    }
    snprintf(buffer, capacity, "A%u%s", static_cast<unsigned int>(channel),
        channel == board::MenuJoystickChannel ? " Joy" : "");
}

void InputMonitor::formatDigitalValue(uint8_t pin, char* buffer, size_t capacity) const {
    if (!buffer || capacity == 0) return;
    const char* text = "---";
    switch (digitalRole(pin)) {
        case DigitalRole::Serial: text = "UART"; break;
        case DigitalRole::LedData: text = "LED"; break;
        case DigitalRole::Buzzer: text = "HANG"; break;
        case DigitalRole::StatusLed: text = "OUT"; break;
        case DigitalRole::I2c: text = "I2C"; break;
        case DigitalRole::Invalid: text = ""; break;
        default:
            switch (digitalValue(pin)) {
                case DigitalState::Low: text = "LOW"; break;
                case DigitalState::High: text = "HIGH"; break;
                case DigitalState::Output: text = "OUT"; break;
                default: break;
            }
            break;
    }
    snprintf(buffer, capacity, "%s", text);
}

void InputMonitor::formatAnalogValue(uint8_t channel, char* buffer, size_t capacity) const {
    if (!buffer || capacity == 0) return;
    if (channel >= board::AnalogChannelCount) {
        buffer[0] = '\0';
        return;
    }
    const uint16_t value = analogValue(channel);
    if (value <= AnalogMaximum) {
        snprintf(buffer, capacity, "%u", static_cast<unsigned int>(value));
    } else {
        snprintf(buffer, capacity, "%s", value == AnalogOutput ? "OUT" :
            (value == AnalogPending ? "---" : "ERR"));
    }
}

} // namespace debug