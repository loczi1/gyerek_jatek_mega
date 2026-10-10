#include "MenuInput.h"
#include "JoystickRepeat.h"
#include "config/BoardConfig.h"
#include "config/NavigationConfig.h"
#include <tcMenu.h>
#include <SwitchInput.h>

namespace menuInput {
namespace {

// tcMenu owns the absolute encoder range/current position, including list back
// index 0. Do not maintain a second cursor or pass a relative delta to tcMenu.
RotaryEncoder encoder([](int value) { menuMgr.valueChanged(value); });
ui::JoystickRepeat repeat;
MenuItem* previousMenu = nullptr;
MenuItem* previousEditor = nullptr;
uint32_t lastSampleAt = 0;
bool initialized = false;

void selectMenu(pinid_t, bool held) {
    repeat.reset(millis());
    menuMgr.onMenuSelect(held);
}

} // namespace

void begin() {
    if (initialized) return;
    pinMode(board::MenuJoystickPin, INPUT);
    switches.init(internalDigitalIo(), SWITCHES_POLL_EVERYTHING, true);
    switches.addSwitch(board::MenuButtonPin, nullptr);
    switches.onRelease(board::MenuButtonPin, selectMenu);
    switches.setEncoder(&encoder);
    repeat.reset(millis());
    lastSampleAt = millis();
    initialized = true;
}

void update(uint32_t now) {
    if (!initialized) return;

    MenuItem* currentMenu = menuMgr.getCurrentMenu();
    MenuItem* currentEditor = menuMgr.getCurrentEditor();
    if (currentMenu != previousMenu || currentEditor != previousEditor) {
        previousMenu = currentMenu;
        previousEditor = currentEditor;
        repeat.reset(now); // No carried hold from a list, Back, or auto-return.
    }

    if (digitalRead(board::MenuButtonPin) == LOW) {
        repeat.reset(now); // Do not scroll while the select button is pressed.
        lastSampleAt = now;
        return;
    }
    if (static_cast<uint32_t>(now - lastSampleAt) < board::MenuJoystickPollMs) return;
    lastSampleAt = now;

    // Debug scans other ADC channels. Discard a conversion after switching
    // back to A9 so navigation never uses a mux-settling transient.
    analogRead(board::MenuJoystickPin);
    const uint16_t value = static_cast<uint16_t>(analogRead(board::MenuJoystickPin));
    int8_t step = repeat.update(now, value);
    if (step == 0) return;
    const EncoderUserIntention intent = encoder.getUserIntention();
    if (intent == CHANGE_VALUE || intent == DIRECTION_ONLY) step = -step;
    encoder.increment(step); // Exactly one, bounded by tcMenu's current range.
}

} // namespace menuInput