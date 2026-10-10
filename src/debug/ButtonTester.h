#ifndef GYEREK_JATEK_BUTTON_TESTER_H
#define GYEREK_JATEK_BUTTON_TESTER_H

#include <stddef.h>
#include <stdint.h>
#include "PinSet.h"

namespace debug {

constexpr uint32_t ButtonDebounceMs = 30;
constexpr uint8_t ButtonSummaryRows = 4;
// UART D0/1, LED D2, buzzer D5, status D13, I2C D20/21 and menu D53.
constexpr uint8_t ButtonCandidateCount = board::DigitalPinCount - 8;
constexpr uint8_t ButtonDebugRowCount = ButtonSummaryRows + ButtonCandidateCount;

enum class ButtonPinMode : uint8_t { FloatingInput, PullupInput, Output };

bool canTestButton(uint8_t pin);
uint8_t buttonCandidatePin(uint8_t index);

class ButtonTester {
public:
    using ReadMode = ButtonPinMode (*)(uint8_t pin);
    using SetPullup = void (*)(uint8_t pin, bool enabled);
    using ReadPressed = bool (*)(uint8_t pin);

    ButtonTester(ReadMode readMode = nullptr, SetPullup setPullup = nullptr,
                 ReadPressed readPressed = nullptr)
        : readMode_(readMode), setPullup_(setPullup), readPressed_(readPressed) {}

    void setEnabled(bool enabled, uint32_t now);
    bool update(uint32_t now);
    bool isEnabled() const { return enabled_; }
    bool isPressed(uint8_t pin) const { return stableLow_.contains(pin); }
    uint8_t pressedCount() const { return stableLow_.count(); }
    uint8_t testedCount() const { return activePins_.count(); }
    static void formatName(uint8_t row, char* buffer, size_t capacity);
    void formatValue(uint8_t row, char* buffer, size_t capacity) const;

private:
    static void formatPins(const PinSet& pins, char* buffer, size_t capacity);

    ReadMode readMode_;
    SetPullup setPullup_;
    ReadPressed readPressed_;
    PinSet activePins_;
    PinSet changedPullups_;
    PinSet outputPins_;
    PinSet rawLow_;
    PinSet stableLow_;
    PinSet lastPress_;
    uint32_t changedAt_[board::DigitalPinCount] = {};
    bool enabled_ = false;
};

} // namespace debug

#endif