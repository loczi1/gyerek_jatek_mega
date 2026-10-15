#include "DigitalOutputTester.h"
#include "InputMonitor.h"
#include "generated/pins.h"
#include <stdio.h>

namespace debug {
namespace {

bool hasWorkbookDigitalOutput(uint8_t pin) {
    if (pin >= board::DigitalPinCount) return false;
    for (uint8_t index = 0;
         index < static_cast<uint8_t>(generated::pins::ControlId::Count); ++index) {
        const generated::pins::PinDefinition definition =
            generated::pins::PinTable[index];
        if (definition.type == generated::pins::PinType::DigitalOut &&
            (definition.activeLevel == generated::pins::ActiveLevel::High ||
             definition.activeLevel == generated::pins::ActiveLevel::Low) &&
            definition.primaryPin == pin)
            return true;
    }
    return false;
}

} // namespace

bool canTestDigitalOutput(uint8_t pin) {
    return hasWorkbookDigitalOutput(pin) && digitalRole(pin) == DigitalRole::Input;
}

uint8_t digitalOutputCandidateCount() {
    uint8_t count = 0;
    for (uint8_t pin = 0; pin < board::DigitalPinCount; ++pin) {
        if (canTestDigitalOutput(pin)) ++count;
    }
    return count;
}

bool digitalOutputInactiveHigh(uint8_t pin) {
    for (const auto& definition : generated::pins::PinTable) {
        if (definition.type == generated::pins::PinType::DigitalOut &&
            definition.primaryPin == pin &&
            definition.activeLevel == generated::pins::ActiveLevel::Low) return true;
    }
    return false;
}

void ButtonLampGuard::setEnabled(bool enabled) {
    if (enabled_ == enabled) return;
    enabled_ = enabled;
    for (uint8_t pin = 0; pin < board::DigitalPinCount; ++pin) {
        if (enabled) {
            if (!canTestDigitalOutput(pin)) continue;
            const ButtonPinMode mode = readMode_(pin);
            if (mode == ButtonPinMode::Output) continue; // Another owner.
            owned_.set(pin);
            if (mode == ButtonPinMode::PullupInput) pullups_.set(pin);
            writePin_(pin, digitalOutputInactiveHigh(pin));
        } else if (owned_.contains(pin) && readMode_(pin) == ButtonPinMode::Output) {
            restorePin_(pin, pullups_.contains(pin));
        }
    }
    if (!enabled) {
        owned_.clear();
        pullups_.clear();
    }
}

uint8_t digitalOutputCandidatePin(uint8_t index) {
    uint8_t current = 0;
    for (uint8_t pin = 0; pin < board::DigitalPinCount; ++pin) {
        if (canTestDigitalOutput(pin) && current++ == index) return pin;
    }
    return 0xff;
}

void formatDigitalOutputName(uint8_t pin, char* buffer, size_t capacity) {
    InputMonitor::formatDigitalName(pin, buffer, capacity);
}

void DigitalOutputTester::setEnabled(bool enabled) {
    if (enabled == enabled_) return;
    enabled_ = enabled;
    if (!enabled_) {
        if (driven_ && resetPin_ && selectedPin_ != 0xff) resetPin_(selectedPin_);
        selectedIndex_ = 0;
        selectedPin_ = 0xff;
        driven_ = false;
        high_ = false;
        return;
    }

    selectedIndex_ = 0;
    selectedPin_ = digitalOutputCandidatePin(0);
    driven_ = false;
    high_ = false;
}

bool DigitalOutputTester::selectOffset(int8_t direction) {
    const uint8_t count = digitalOutputCandidateCount();
    if (!enabled_ || count == 0) return false;
    const uint8_t previousPin = selectedPin_;
    const bool previousDriven = driven_;
    const bool previousHigh = high_;
    selectedIndex_ = static_cast<uint8_t>((selectedIndex_ + count + direction) % count);
    selectedPin_ = digitalOutputCandidatePin(selectedIndex_);
    if (previousDriven && resetPin_ && previousPin != 0xff && previousPin != selectedPin_)
        resetPin_(previousPin);
    if (previousPin == selectedPin_) {
        driven_ = previousDriven;
        high_ = previousHigh;
    } else {
        driven_ = false;
        high_ = false;
    }
    return true;
}

bool DigitalOutputTester::selectNext() {
    return selectOffset(1);
}

bool DigitalOutputTester::selectPrevious() {
    return selectOffset(-1);
}

bool DigitalOutputTester::driveSelected(bool high) {
    if (!enabled_ || selectedPin_ == 0xff || !writePin_) return false;
    writePin_(selectedPin_, high);
    driven_ = true;
    high_ = high;
    return true;
}

void DigitalOutputTester::formatStatus(char* buffer, size_t capacity) const {
    if (!buffer || capacity == 0) return;
    if (!enabled_ || selectedPin_ == 0xff) {
        snprintf(buffer, capacity, "NINCS");
        return;
    }

    char name[8];
    formatDigitalOutputName(selectedPin_, name, sizeof(name));
    snprintf(buffer, capacity, "%s %s", name,
             driven_ ? (high_ ? "HIGH" : "LOW") : "---");
}

} // namespace debug