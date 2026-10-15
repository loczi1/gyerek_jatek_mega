#ifndef GYEREK_JATEK_DIGITAL_OUTPUT_TESTER_H
#define GYEREK_JATEK_DIGITAL_OUTPUT_TESTER_H

#include <stddef.h>
#include <stdint.h>
#include "ButtonTester.h"

namespace debug {

bool canTestDigitalOutput(uint8_t pin);
uint8_t digitalOutputCandidateCount();
uint8_t digitalOutputCandidatePin(uint8_t index);
void formatDigitalOutputName(uint8_t pin, char* buffer, size_t capacity);
bool digitalOutputInactiveHigh(uint8_t pin);

// Scoped ownership of otherwise unused lamp pins while Gombok is open.
class ButtonLampGuard {
public:
    using RestorePin = void (*)(uint8_t pin, bool pullup);
    ButtonLampGuard(ButtonTester::ReadMode readMode,
                    void (*writePin)(uint8_t, bool), RestorePin restorePin)
        : readMode_(readMode), writePin_(writePin), restorePin_(restorePin) {}
    void setEnabled(bool enabled);

private:
    ButtonTester::ReadMode readMode_;
    void (*writePin_)(uint8_t, bool);
    RestorePin restorePin_;
    PinSet owned_;
    PinSet pullups_;
    bool enabled_ = false;
};

class DigitalOutputTester {
public:
    using WritePin = void (*)(uint8_t pin, bool high);
    using ResetPin = void (*)(uint8_t pin);

    DigitalOutputTester(WritePin writePin = nullptr, ResetPin resetPin = nullptr)
        : writePin_(writePin), resetPin_(resetPin) {}

    void setEnabled(bool enabled);
    bool isEnabled() const { return enabled_; }
    uint8_t selectedPin() const { return selectedPin_; }
    bool selectNext();
    bool selectPrevious();
    bool driveSelected(bool high);
    void formatStatus(char* buffer, size_t capacity) const;

private:
    bool selectOffset(int8_t direction);

    WritePin writePin_;
    ResetPin resetPin_;
    uint8_t selectedIndex_ = 0;
    uint8_t selectedPin_ = 0xff;
    bool enabled_ = false;
    bool driven_ = false;
    bool high_ = false;
};

} // namespace debug

#endif