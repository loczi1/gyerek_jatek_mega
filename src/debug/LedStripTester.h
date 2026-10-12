#ifndef GYEREK_JATEK_LED_STRIP_TESTER_H
#define GYEREK_JATEK_LED_STRIP_TESTER_H

#include <stddef.h>
#include <stdint.h>
#include "config/LedConfig.h"

namespace debug {

// Portable, manually stepped LED-test state. Positions are zero-based inside
// the renderer, but the user sees LEDs 1..length on the LCD.
class LedStripTester {
public:
    explicit LedStripTester(uint32_t idleTimeoutMs) : idleTimeoutMs_(idleTimeoutMs) {}

    void start(uint16_t length, uint32_t now);
    void stop() { active_ = false; }
    bool step(uint32_t now);
    bool hasTimedOut(uint32_t now) const;
    bool isActive() const { return active_; }
    uint8_t length() const { return length_; }
    uint8_t position() const { return position_; }
    bool isLit(uint16_t index) const { return active_ && index == position_; }
    void formatPosition(char* buffer, size_t capacity) const;

private:
    uint32_t idleTimeoutMs_;
    uint32_t lastStepAt_ = 0;
    uint8_t length_ = board::DefaultLedTestCount;
    uint8_t position_ = 0;
    bool active_ = false;
};

} // namespace debug

#endif