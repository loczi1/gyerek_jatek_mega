#include "LedStripTester.h"
#include <stdio.h>

namespace debug {

void LedStripTester::start(uint16_t length, uint32_t now) {
    if (length == 0) length = 1;
    if (length > board::LedStripCapacity) length = board::LedStripCapacity;
    length_ = static_cast<uint8_t>(length);
    position_ = 0;
    lastStepAt_ = now;
    active_ = true;
}

bool LedStripTester::step(uint32_t now) {
    if (!active_) return false;
    position_ = static_cast<uint8_t>((position_ + 1) % length_);
    lastStepAt_ = now;
    return true;
}

bool LedStripTester::hasTimedOut(uint32_t now) const {
    return active_ && static_cast<uint32_t>(now - lastStepAt_) >= idleTimeoutMs_;
}

void LedStripTester::formatPosition(char* buffer, size_t capacity) const {
    if (!buffer || capacity == 0) return;
    if (!active_) {
        buffer[0] = '\0';
        return;
    }
    snprintf(buffer, capacity, "LED %u/%u", static_cast<unsigned int>(position_ + 1),
        static_cast<unsigned int>(length_));
}

} // namespace debug