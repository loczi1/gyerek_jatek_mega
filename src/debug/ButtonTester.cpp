#include "ButtonTester.h"
#include "InputMonitor.h"
#include <stdio.h>

namespace debug {

bool canTestButton(uint8_t pin) {
    if (pin >= board::FirstAnalogGpioPin && pin < board::GpioPinCount)
        return pin != board::FirstAnalogGpioPin + board::MenuJoystickChannel;
    return canReadDigital(pin) && pin != board::MenuButtonPin;
}

uint8_t buttonCandidatePin(uint8_t index) {
    uint8_t current = 0;
    for (uint8_t pin = 0; pin < board::GpioPinCount; ++pin) {
        if (canTestButton(pin) && current++ == index) return pin;
    }
    return 0xff;
}

void ButtonTester::setEnabled(bool enabled, uint32_t now) {
    if (enabled == enabled_) return;
    if (!enabled) {
        // Restore only pull-ups this test added, and never touch a pin that
        // another owner has since made an output. Existing pull-ups stay on.
        for (uint8_t pin = 0; pin < board::GpioPinCount; ++pin) {
            if (changedPullups_.contains(pin) && readMode_ && setPullup_ &&
                readMode_(pin) == ButtonPinMode::PullupInput)
                setPullup_(pin, false);
        }
        activePins_.clear();
        changedPullups_.clear();
        stableLow_.clear();
        enabled_ = false;
        return;
    }

    activePins_.clear();
    changedPullups_.clear();
    outputPins_.clear();
    rawLow_.clear();
    stableLow_.clear();
    lastPress_.clear();
    enabled_ = true;
    if (!readMode_ || !readPressed_) return;

    for (uint8_t pin = 0; pin < board::GpioPinCount; ++pin) {
        if (!canTestButton(pin)) continue;
        const ButtonPinMode mode = readMode_(pin);
        if (mode == ButtonPinMode::Output) {
            outputPins_.set(pin);
            continue;
        }
        if (mode == ButtonPinMode::FloatingInput) {
            if (!setPullup_) continue;
            setPullup_(pin, true);
            changedPullups_.set(pin);
        }
        activePins_.set(pin);
        changedAt_[pin] = now;
    }
}

bool ButtonTester::update(uint32_t now) {
    if (!enabled_ || !readMode_ || !readPressed_) return false;
    bool changed = false;
    bool newPress = false;
    for (uint8_t pin = 0; pin < board::GpioPinCount; ++pin) {
        if (!activePins_.contains(pin)) continue;
        if (readMode_(pin) == ButtonPinMode::Output) {
            activePins_.set(pin, false);
            outputPins_.set(pin);
            stableLow_.set(pin, false);
            rawLow_.set(pin, false);
            changed = true;
            continue;
        }

        const bool low = readPressed_(pin);
        if (low != rawLow_.contains(pin)) {
            rawLow_.set(pin, low);
            changedAt_[pin] = now;
        } else if (low != stableLow_.contains(pin) &&
                   static_cast<uint32_t>(now - changedAt_[pin]) >= ButtonDebounceMs) {
            stableLow_.set(pin, low);
            changed = true;
            newPress = newPress || low;
        }
    }
    // Retain a short press until the next press or test entry, so a slower LCD
    // redraw cannot hide it. The live row still reports actual pressed state.
    if (newPress) lastPress_ = stableLow_;
    return changed;
}

void ButtonTester::formatPins(const PinSet& pins, char* buffer, size_t capacity) {
    if (!buffer || capacity == 0) return;
    const uint8_t total = pins.count();
    if (total == 0) {
        snprintf(buffer, capacity, "NINCS");
        return;
    }
    // Three IDs and an overflow count always fit the LCD's 15 value columns.
    // All remaining pins are also individually visible in fixed list rows.
    char text[16] = {};
    size_t used = 0;
    uint8_t listed = 0;
    for (uint8_t pin = 0; pin < board::GpioPinCount && listed < 3; ++pin) {
        if (!pins.contains(pin)) continue;
        const bool analog = pin >= board::FirstAnalogGpioPin;
        const unsigned int number = analog ? pin - board::FirstAnalogGpioPin : pin;
        const int count = snprintf(text + used, sizeof(text) - used, "%s%c%u",
            listed == 0 ? "" : " ", analog ? 'A' : 'D', number);
        if (count < 0) break;
        used += static_cast<size_t>(count);
        ++listed;
    }
    if (listed < total)
        snprintf(text + used, sizeof(text) - used, " +%u", static_cast<unsigned int>(total - listed));
    snprintf(buffer, capacity, "%s", text);
}

void ButtonTester::formatName(uint8_t row, char* buffer, size_t capacity) {
    if (!buffer || capacity == 0) return;
    if (row < ButtonSummaryRows) {
        const char* text = row == 0 ? "Utolso" : (row == 1 ? "Lenyomva" :
            (row == 2 ? "Darab" : "Tesztelt"));
        snprintf(buffer, capacity, "%s", text);
    } else {
        const uint8_t pin = buttonCandidatePin(row - ButtonSummaryRows);
        if (pin >= board::FirstAnalogGpioPin && pin < board::GpioPinCount)
            InputMonitor::formatAnalogName(pin - board::FirstAnalogGpioPin, buffer, capacity);
        else
            InputMonitor::formatDigitalName(pin, buffer, capacity);
    }
}

void ButtonTester::formatValue(uint8_t row, char* buffer, size_t capacity) const {
    if (!buffer || capacity == 0) return;
    if (row == 0) formatPins(lastPress_, buffer, capacity);
    else if (row == 1) formatPins(stableLow_, buffer, capacity);
    else if (row == 2 || row == 3)
        snprintf(buffer, capacity, "%u", static_cast<unsigned int>(
            row == 2 ? pressedCount() : testedCount()));
    else {
        const uint8_t pin = buttonCandidatePin(row - ButtonSummaryRows);
        const char* text = pin >= board::GpioPinCount ? "" :
            (outputPins_.contains(pin) ? "OUT" : (activePins_.contains(pin)
                ? (stableLow_.contains(pin) ? "LOW" : "HIGH") : "---"));
        snprintf(buffer, capacity, "%s", text);
    }
}

} // namespace debug