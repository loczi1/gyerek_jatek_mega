#include "game/PairedLampOutputs.h"
#include "config/PanelConfig.h"
#include "debug/DigitalOutputTester.h"

namespace game {

bool PairedLampOutputs::stillOwned(uint8_t pin) {
    if (!owned_.contains(pin)) return false;
    if (mode_(pin) == debug::ButtonPinMode::Output &&
        high_(pin) == lastHigh_.contains(pin)) return true;
    owned_.set(pin, false); // Another owner changed mode or level; leave it alone.
    return false;
}

void PairedLampOutputs::setEnabled(bool enabled) {
    if (enabled_ == enabled) return;
    enabled_ = enabled;
    for (uint8_t pin = 0; pin < board::DigitalPinCount; ++pin) {
        if (enabled) {
            if (!debug::canTestDigitalOutput(pin)) continue;
            const auto mode = mode_(pin);
            if (mode == debug::ButtonPinMode::Output) continue;
            owned_.set(pin);
            pullups_.set(pin, mode == debug::ButtonPinMode::PullupInput);
            const bool inactive = debug::digitalOutputInactiveHigh(pin);
            lastHigh_.set(pin, inactive);
            write_(pin, inactive);
        } else if (stillOwned(pin)) {
            write_(pin, debug::digitalOutputInactiveHigh(pin));
            restore_(pin, pullups_.contains(pin));
        }
    }
    if (!enabled) {
        owned_.clear();
        pullups_.clear();
        lastHigh_.clear();
    }
}

void PairedLampOutputs::draw(uint32_t inputMask, const uint8_t* inputs, uint8_t count) {
    if (!enabled_) return;
    debug::PinSet active;
    if (inputs) {
        for (uint8_t i = 0; i < count && i < PanelInputCount; ++i) {
            if (inputMask & (uint32_t(1) << i)) {
                const uint8_t pin = board::pairedGpioLamp(inputs[i]);
                if (pin < board::DigitalPinCount) active.set(pin);
            }
        }
    }
    for (uint8_t pin = 0; pin < board::DigitalPinCount; ++pin) {
        if (!stillOwned(pin)) continue;
        const bool high = active.contains(pin) != debug::digitalOutputInactiveHigh(pin);
        if (high != lastHigh_.contains(pin)) {
            lastHigh_.set(pin, high);
            write_(pin, high);
        }
    }
}

} // namespace game