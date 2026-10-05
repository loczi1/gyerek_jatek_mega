#ifndef GYEREK_JATEK_DEBOUNCED_BUTTON_H
#define GYEREK_JATEK_DEBOUNCED_BUTTON_H

#include <stdint.h>

namespace game {

// The caller supplies a logical pressed value, independent of pin polarity.
class DebouncedButton {
public:
    explicit DebouncedButton(uint32_t debounceMs = 30)
        : debounceMs_(debounceMs) {}

    void reset(uint32_t now, bool pressed) {
        rawPressed_ = pressed;
        stablePressed_ = pressed;
        changedAt_ = now;
    }

    // Returns exactly one event per debounced press. Holding the button never
    // repeats, and a button held during reset must be released before use.
    bool update(uint32_t now, bool pressed) {
        if (pressed != rawPressed_) {
            rawPressed_ = pressed;
            changedAt_ = now;
        }

        if (stablePressed_ != rawPressed_ &&
            static_cast<uint32_t>(now - changedAt_) >= debounceMs_) {
            stablePressed_ = rawPressed_;
            return stablePressed_;
        }
        return false;
    }

private:
    uint32_t debounceMs_;
    uint32_t changedAt_ = 0;
    bool rawPressed_ = false;
    bool stablePressed_ = false;
};

} // namespace game

#endif