#include "Diagnostics.h"
#include "config/BoardConfig.h"
#include "debug/ButtonTester.h"
#include "debug/DigitalOutputTester.h"
#include "debug/InputMonitor.h"
#include <avr/interrupt.h>

namespace diagnostics {
namespace {

void setButtonLampsEnabled(bool enabled);

debug::DigitalState readDigital(uint8_t pin) {
    if (!debug::canReadDigital(pin)) return debug::DigitalState::Reserved;
    const uint8_t port = digitalPinToPort(pin);
    const uint8_t mask = digitalPinToBitMask(pin);
    if ((*portModeRegister(port) & mask) != 0) return debug::DigitalState::Output;
    // Arduino digitalRead() can disable PWM; read the input register instead.
    // Never change DDR, PORT, peripheral ownership or pull-ups while scanning.
    return (*portInputRegister(port) & mask) != 0
        ? debug::DigitalState::High : debug::DigitalState::Low;
}

uint16_t readAnalog(uint8_t channel) {
    if (channel >= board::AnalogChannelCount) return debug::AnalogPending;
    const uint8_t pin = A0 + channel;
    const uint8_t port = digitalPinToPort(pin);
    const uint8_t mask = digitalPinToBitMask(pin);
    if ((*portModeRegister(port) & mask) != 0) return debug::AnalogOutput;
    // Discard one conversion after changing channels to reduce ADC cross-talk.
    // The single retained conversion is still a raw 0..1023 input reading.
    analogRead(pin);
    return static_cast<uint16_t>(analogRead(pin));
}

debug::InputMonitor monitor(readDigital, readAnalog);

debug::ButtonPinMode readButtonMode(uint8_t pin) {
    const uint8_t port = digitalPinToPort(pin);
    const uint8_t mask = digitalPinToBitMask(pin);
    if ((*portModeRegister(port) & mask) != 0) return debug::ButtonPinMode::Output;
    return (*portOutputRegister(port) & mask) != 0
        ? debug::ButtonPinMode::PullupInput : debug::ButtonPinMode::FloatingInput;
}

void setButtonPullup(uint8_t pin, bool enabled) {
    if (!debug::canTestButton(pin)) return;
    const uint8_t port = digitalPinToPort(pin);
    const uint8_t mask = digitalPinToBitMask(pin);
    const uint8_t savedInterrupts = SREG;
    cli();
    // Change only the pull-up latch of an existing INPUT, never DDR/output.
    // Protect the read/modify/write from ISR activity on another port bit.
    if ((*portModeRegister(port) & mask) == 0) {
        if (enabled) *portOutputRegister(port) |= mask;
        else *portOutputRegister(port) &= static_cast<uint8_t>(~mask);
    }
    SREG = savedInterrupts;
}

bool readButtonPressed(uint8_t pin) {
    if (!debug::canTestButton(pin)) return false;
    const uint8_t port = digitalPinToPort(pin);
    const uint8_t mask = digitalPinToBitMask(pin);
    if ((*portModeRegister(port) & mask) != 0) return false;
    // Includes A0-A15 used as digital GPIO, excluding the analog joystick.
    return (*portInputRegister(port) & mask) == 0;
}

debug::ButtonTester buttonTester(readButtonMode, setButtonPullup, readButtonPressed);

} // namespace

void begin() {
    // Only pins actually used as button inputs get pull-ups. Leave UART, I2C,
    // LED data and all unrelated pins to their own peripherals.
    for (uint8_t pin = 0; pin < board::DigitalPinCount; ++pin) {
        if (debug::canTestButton(pin) &&
            (pin == board::GameButtonPin || board::isWorkbookDigitalInput(pin)))
            pinMode(pin, INPUT_PULLUP);
    }
    pinMode(board::BuzzerPin, OUTPUT);
    digitalWrite(board::BuzzerPin, LOW);
    pinMode(board::StatusLedPin, OUTPUT);
    digitalWrite(board::StatusLedPin, LOW);
    pinMode(board::AnalogInputPin, INPUT);
}

namespace {

void writeDigitalOutput(uint8_t pin, bool high) {
    if (!debug::canTestDigitalOutput(pin)) return;
    digitalWrite(pin, high ? HIGH : LOW);
    pinMode(pin, OUTPUT); // Preload latch before enabling the transistor output.
}

void resetDigitalOutput(uint8_t pin) {
    if (!debug::canTestDigitalOutput(pin)) return;
    digitalWrite(pin, debug::digitalOutputInactiveHigh(pin) ? HIGH : LOW);
    pinMode(pin, INPUT);
    digitalWrite(pin, LOW); // No pull-up left behind by the manual output test.
}

debug::DigitalOutputTester digitalOutputTester(writeDigitalOutput, resetDigitalOutput);

void restoreButtonLamp(uint8_t pin, bool pullup) {
    // If somebody changed the driven level, relinquish rather than overwrite it.
    const uint8_t port = digitalPinToPort(pin);
    const uint8_t mask = digitalPinToBitMask(pin);
    const bool high = (*portOutputRegister(port) & mask) != 0;
    if (high != debug::digitalOutputInactiveHigh(pin)) return;
    resetDigitalOutput(pin);
    setButtonPullup(pin, pullup);
}

debug::ButtonLampGuard buttonLamps(readButtonMode, writeDigitalOutput, restoreButtonLamp);

void setButtonLampsEnabled(bool enabled) {
    // Release the manual tester before taking ownership of free lamp pins.
    if (enabled) digitalOutputTester.setEnabled(false);
    buttonLamps.setEnabled(enabled);
}

} // namespace

void setEnabled(bool enabled) {
    monitor.setEnabled(enabled);
    if (!enabled) {
        buttonTester.setEnabled(false, millis());
        setButtonLampsEnabled(false);
        digitalOutputTester.setEnabled(false);
    }
}

void update() {
    monitor.update();
}

void setButtonTestEnabled(bool enabled, uint32_t now) {
    // Lamps become OUTPUT before button enumeration, so no lamp gets a pull-up.
    if (enabled) setButtonLampsEnabled(true);
    buttonTester.setEnabled(enabled, now);
    if (!enabled) setButtonLampsEnabled(false);
}

bool updateButtons(uint32_t now) {
    return buttonTester.update(now);
}

void setDigitalOutputTestEnabled(bool enabled) {
    digitalOutputTester.setEnabled(enabled);
}

void selectNextDigitalOutput() {
    digitalOutputTester.selectNext();
}

void selectPreviousDigitalOutput() {
    digitalOutputTester.selectPrevious();
}

void setSelectedDigitalOutput(bool high) {
    digitalOutputTester.driveSelected(high);
}

void formatDigitalOutputStatus(char* buffer, size_t capacity) {
    digitalOutputTester.formatStatus(buffer, capacity);
}

void formatButtonName(uint8_t row, char* buffer, size_t capacity) {
    debug::ButtonTester::formatName(row, buffer, capacity);
}

void formatButtonValue(uint8_t row, char* buffer, size_t capacity) {
    buttonTester.formatValue(row, buffer, capacity);
}

void formatDigitalName(uint8_t pin, char* buffer, size_t capacity) {
    debug::InputMonitor::formatDigitalName(pin, buffer, capacity);
}

void formatDigitalValue(uint8_t pin, char* buffer, size_t capacity) {
    monitor.formatDigitalValue(pin, buffer, capacity);
}

void formatAnalogName(uint8_t channel, char* buffer, size_t capacity) {
    debug::InputMonitor::formatAnalogName(channel, buffer, capacity);
}

void formatAnalogValue(uint8_t channel, char* buffer, size_t capacity) {
    monitor.formatAnalogValue(channel, buffer, capacity);
}

} // namespace diagnostics