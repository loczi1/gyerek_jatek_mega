#include "hardware/GameLamps.h"
#include "config/BoardConfig.h"
#include "debug/DigitalOutputTester.h"
#include "game/PairedLampOutputs.h"

namespace gameLamps {
namespace {
debug::ButtonPinMode readMode(uint8_t pin) {
    const uint8_t port = digitalPinToPort(pin);
    const uint8_t bit = digitalPinToBitMask(pin);
    if (*portModeRegister(port) & bit) return debug::ButtonPinMode::Output;
    return (*portOutputRegister(port) & bit) ? debug::ButtonPinMode::PullupInput
                                           : debug::ButtonPinMode::FloatingInput;
}

bool readHigh(uint8_t pin) {
    return (*portOutputRegister(digitalPinToPort(pin)) & digitalPinToBitMask(pin)) != 0;
}

void write(uint8_t pin, bool high) {
    if (!debug::canTestDigitalOutput(pin)) return;
    digitalWrite(pin, high ? HIGH : LOW);
    pinMode(pin, OUTPUT); // Latch before enabling the transistor output.
}

void restore(uint8_t pin, bool pullup) {
    if (!debug::canTestDigitalOutput(pin)) return;
    // PairedLampOutputs already drove the lamp inactive before releasing it.
    pinMode(pin, pullup ? INPUT_PULLUP : INPUT);
}

game::PairedLampOutputs lamps(readMode, readHigh, write, restore);
}

void setEnabled(bool enabled) { lamps.setEnabled(enabled); }
void draw(uint32_t inputMask, const uint8_t* inputs, uint8_t count) {
    lamps.draw(inputMask, inputs, count);
}
} // namespace gameLamps