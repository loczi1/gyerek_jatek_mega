#ifndef GYEREK_JATEK_PIN_SET_H
#define GYEREK_JATEK_PIN_SET_H

#include "config/PinConfig.h"

namespace debug {

// Compact, bounded set for the Mega's D0-D53 bank. No heap or 64-bit shifts.
class PinSet {
public:
    void clear() {
        for (uint8_t& byte : bits_) byte = 0;
    }

    bool contains(uint8_t pin) const {
        return pin < board::DigitalPinCount &&
            (bits_[pin / 8] & static_cast<uint8_t>(1U << (pin % 8))) != 0;
    }

    void set(uint8_t pin, bool included = true) {
        if (pin >= board::DigitalPinCount) return;
        const uint8_t mask = static_cast<uint8_t>(1U << (pin % 8));
        if (included) bits_[pin / 8] |= mask;
        else bits_[pin / 8] &= static_cast<uint8_t>(~mask);
    }

    uint8_t count() const {
        uint8_t result = 0;
        for (uint8_t pin = 0; pin < board::DigitalPinCount; ++pin)
            if (contains(pin)) ++result;
        return result;
    }

private:
    uint8_t bits_[(board::DigitalPinCount + 7) / 8] = {};
};

} // namespace debug

#endif