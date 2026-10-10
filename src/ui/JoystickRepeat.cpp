#include "JoystickRepeat.h"
#include "config/NavigationConfig.h"

namespace ui {

void JoystickRepeat::reset(uint32_t now) {
    candidateSince_ = now;
    lastStepAt_ = now;
    candidate_ = stable_ = 0;
    needsNeutral_ = true;
    neutralSeen_ = false;
    repeating_ = false;
}

int8_t JoystickRepeat::direction(uint16_t value) const {
    if (value >= board::MenuJoystickCentre + board::MenuJoystickEngageDistance) return -1;
    if (value <= board::MenuJoystickCentre - board::MenuJoystickEngageDistance) return 1;
    // Once engaged, retain that direction until the smaller release zone is
    // reached. Noise near the outer threshold cannot create extra presses.
    if (candidate_ < 0 && value > board::MenuJoystickCentre + board::MenuJoystickReleaseDistance)
        return -1;
    if (candidate_ > 0 && value < board::MenuJoystickCentre - board::MenuJoystickReleaseDistance)
        return 1;
    return 0;
}

int8_t JoystickRepeat::update(uint32_t now, uint16_t value) {
    if (value > 1023) {
        reset(now);
        return 0;
    }

    if (needsNeutral_) {
        const bool centred = value >= board::MenuJoystickCentre - board::MenuJoystickReleaseDistance &&
            value <= board::MenuJoystickCentre + board::MenuJoystickReleaseDistance;
        if (!centred) {
            neutralSeen_ = false;
        } else if (!neutralSeen_) {
            neutralSeen_ = true;
            candidateSince_ = now;
        } else if (static_cast<uint32_t>(now - candidateSince_) >= board::MenuJoystickDebounceMs) {
            needsNeutral_ = false;
        }
        return 0;
    }

    const int8_t next = direction(value);
    if (next != candidate_) {
        candidate_ = next;
        candidateSince_ = now;
        return 0; // Stop old-direction repeats as soon as the input changes.
    }

    if (candidate_ != stable_) {
        if (static_cast<uint32_t>(now - candidateSince_) < board::MenuJoystickDebounceMs)
            return 0;
        stable_ = candidate_;
        repeating_ = false;
        lastStepAt_ = now;
        return stable_; // One step on a debounced movement; zero on release.
    }

    if (stable_ == 0) return 0;
    const uint32_t interval = repeating_ ? board::MenuJoystickRepeatMs : board::MenuJoystickHoldMs;
    if (static_cast<uint32_t>(now - lastStepAt_) < interval) return 0;
    repeating_ = true;
    lastStepAt_ = now; // No backlog if LCD/I2C temporarily delays the loop.
    return stable_;
}

} // namespace ui