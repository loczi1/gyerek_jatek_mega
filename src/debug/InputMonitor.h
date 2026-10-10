#ifndef GYEREK_JATEK_INPUT_MONITOR_H
#define GYEREK_JATEK_INPUT_MONITOR_H

#include <stddef.h>
#include <stdint.h>
#include "config/PinConfig.h"

namespace debug {

enum class DigitalRole : uint8_t {
    Input, GameButton, MenuButton, Serial, LedData, Buzzer, StatusLed, I2c, Invalid
};
enum class DigitalState : uint8_t { Pending, Low, High, Output, Reserved };

constexpr uint16_t AnalogPending = 0xffff;
constexpr uint16_t AnalogOutput = 0xfffe;
constexpr uint16_t AnalogMaximum = 1023;
constexpr uint8_t DigitalSamplesPerTick = 4;

DigitalRole digitalRole(uint8_t pin);
bool canReadDigital(uint8_t pin);

// Read-only sampler with bounded work: four digital pins + one ADC channel per
// tick. Injected readers make the safety policy and formatting testable.
class InputMonitor {
public:
    using DigitalReader = DigitalState (*)(uint8_t pin);
    using AnalogReader = uint16_t (*)(uint8_t channel);

    InputMonitor(DigitalReader readDigital = nullptr, AnalogReader readAnalog = nullptr);
    void setEnabled(bool enabled);
    void update();
    bool isEnabled() const { return enabled_; }
    DigitalState digitalValue(uint8_t pin) const;
    uint16_t analogValue(uint8_t channel) const;

    static void formatDigitalName(uint8_t pin, char* buffer, size_t capacity);
    static void formatAnalogName(uint8_t channel, char* buffer, size_t capacity);
    void formatDigitalValue(uint8_t pin, char* buffer, size_t capacity) const;
    void formatAnalogValue(uint8_t channel, char* buffer, size_t capacity) const;

private:
    void resetSamples();

    DigitalReader readDigital_;
    AnalogReader readAnalog_;
    DigitalState digitalValues_[board::DigitalPinCount];
    uint16_t analogValues_[board::AnalogChannelCount];
    uint8_t nextDigital_ = 0;
    uint8_t nextAnalog_ = 0;
    bool enabled_ = false;
};

} // namespace debug

#endif