#ifndef GYEREK_JATEK_JOYSTICK_REPEAT_H
#define GYEREK_JATEK_JOYSTICK_REPEAT_H

#include <stdint.h>

namespace ui {

// Portable, debounced single-step navigation. Never accelerates or catches up
// missed repeats. Reset on a view/selection change to require neutral first.
class JoystickRepeat {
public:
    void reset(uint32_t now);
    // Positive ADC deflection preserves the original "up" mapping (-1).
    // Returns only -1, 0 or +1, never a jump proportional to force/hold length.
    int8_t update(uint32_t now, uint16_t value);

private:
    int8_t direction(uint16_t value) const;

    uint32_t candidateSince_ = 0;
    uint32_t lastStepAt_ = 0;
    int8_t candidate_ = 0;
    int8_t stable_ = 0;
    bool needsNeutral_ = true;
    bool neutralSeen_ = false;
    bool repeating_ = false;
};

} // namespace ui

#endif