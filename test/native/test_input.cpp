#include "test_input.h"
#include "debug/DigitalOutputTester.h"
#include "debug/InputMonitor.h"
#include "ui/JoystickRepeat.h"
#include "config/NavigationConfig.h"
#include "debug/ButtonTester.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace {

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition); \
        exit(EXIT_FAILURE); \
    } \
} while (false)

debug::ButtonPinMode pinModes[board::GpioPinCount];
bool pressed[board::GpioPinCount];
unsigned int reads[board::GpioPinCount];
unsigned int enables[board::GpioPinCount];
unsigned int disables[board::GpioPinCount];
unsigned int outputWrites[board::DigitalPinCount];
unsigned int outputResets[board::DigitalPinCount];
bool outputHigh[board::DigitalPinCount];
constexpr uint8_t UnusedDigitalPin = 6;

bool hasInitialPullup(uint8_t pin) {
    return pin < board::DigitalPinCount && debug::canTestButton(pin) &&
        (pin == board::GameButtonPin || board::isWorkbookDigitalInput(pin));
}

uint8_t buttonRowForPin(uint8_t pin) {
    for (uint8_t index = 0; index < debug::ButtonCandidateCount; ++index) {
        if (debug::buttonCandidatePin(index) == pin)
            return debug::ButtonSummaryRows + index;
    }
    CHECK(false);
    return 255;
}

void checkOutputStatus(const debug::DigitalOutputTester& tester, uint8_t pin,
                       const char* state) {
    char text[16];
    char expected[16];
    tester.formatStatus(text, sizeof(text));
    snprintf(expected, sizeof(expected), "D%u %s", static_cast<unsigned int>(pin), state);
    CHECK(strcmp(text, expected) == 0);
}

debug::ButtonPinMode readMode(uint8_t pin) {
    CHECK(pin < board::GpioPinCount);
    CHECK(debug::canTestButton(pin));
    return pinModes[pin];
}

void setPullup(uint8_t pin, bool enabled) {
    CHECK(debug::canTestButton(pin));
    CHECK(pinModes[pin] != debug::ButtonPinMode::Output);
    if (enabled) ++enables[pin];
    else ++disables[pin];
    pinModes[pin] = enabled ? debug::ButtonPinMode::PullupInput : debug::ButtonPinMode::FloatingInput;
}

bool readPressed(uint8_t pin) {
    CHECK(debug::canTestButton(pin));
    CHECK(pinModes[pin] == debug::ButtonPinMode::PullupInput);
    ++reads[pin];
    return pressed[pin];
}

void writeOutput(uint8_t pin, bool high) {
    CHECK(debug::canTestDigitalOutput(pin));
    ++outputWrites[pin];
    outputHigh[pin] = high;
}

void resetOutput(uint8_t pin) {
    CHECK(debug::canTestDigitalOutput(pin));
    ++outputResets[pin];
    outputHigh[pin] = false;
}

void writeLamp(uint8_t pin, bool high) {
    writeOutput(pin, high);
    pinModes[pin] = debug::ButtonPinMode::Output;
}

void restoreLamp(uint8_t pin, bool pullup) {
    CHECK(pinModes[pin] == debug::ButtonPinMode::Output);
    resetOutput(pin);
    pinModes[pin] = pullup ? debug::ButtonPinMode::PullupInput
                          : debug::ButtonPinMode::FloatingInput;
}

void arm(ui::JoystickRepeat& repeat, uint32_t now = 0) {
    repeat.reset(now);
    CHECK(repeat.update(now, board::MenuJoystickCentre) == 0);
    CHECK(repeat.update(now + board::MenuJoystickDebounceMs, board::MenuJoystickCentre) == 0);
}

void testShortJoystickMovementIsOneStep() {
    ui::JoystickRepeat repeat;
    arm(repeat);
    CHECK(repeat.update(40, 100) == 0);
    CHECK(repeat.update(69, 100) == 0);
    CHECK(repeat.update(70, 100) == 1);
    CHECK(repeat.update(80, 512) == 0);
    CHECK(repeat.update(110, 512) == 0);
    CHECK(repeat.update(1000, 512) == 0);
    CHECK(repeat.update(1010, 950) == 0);
    CHECK(repeat.update(1040, 950) == -1);
}

void testJoystickHoldRepeatsOneAtFixedIntervals() {
    ui::JoystickRepeat repeat;
    arm(repeat);
    CHECK(repeat.update(40, 0) == 0);
    CHECK(repeat.update(70, 0) == 1);
    CHECK(repeat.update(569, 0) == 0);
    CHECK(repeat.update(570, 0) == 1);
    CHECK(repeat.update(869, 0) == 0);
    CHECK(repeat.update(870, 0) == 1);
    CHECK(repeat.update(1169, 0) == 0);
    CHECK(repeat.update(1170, 0) == 1);
}

void testJoystickForceDoesNotAccelerate() {
    ui::JoystickRepeat light;
    ui::JoystickRepeat full;
    arm(light);
    arm(full);
    unsigned int count = 0;
    for (uint32_t now = 40; now <= 4000; now += 10) {
        const int8_t a = light.update(now, 370);
        const int8_t b = full.update(now, 0);
        CHECK(a == b);
        CHECK(a == 0 || a == 1);
        if (a != 0) ++count;
    }
    CHECK(count == 13);
}

void testJoystickNoiseAndShortSpikesAreIgnored() {
    ui::JoystickRepeat repeat;
    arm(repeat);
    for (uint32_t now = 40; now < 1000; now += 10)
        CHECK(repeat.update(now, now % 20 == 0 ? 380 : 640) == 0);
    CHECK(repeat.update(1000, 0) == 0);
    CHECK(repeat.update(1010, 512) == 0);
    CHECK(repeat.update(1040, 512) == 0);
    CHECK(repeat.update(1500, 512) == 0);
}

void testJoystickHysteresisPreventsExtraSteps() {
    ui::JoystickRepeat repeat;
    arm(repeat);
    CHECK(repeat.update(40, 370) == 0);
    CHECK(repeat.update(70, 370) == 1);
    for (uint32_t now = 80; now < 500; now += 10)
        CHECK(repeat.update(now, now % 20 == 0 ? 400 : 425) == 0);
    CHECK(repeat.update(570, 410) == 1);
    CHECK(repeat.update(580, 512) == 0);
    CHECK(repeat.update(610, 512) == 0);
    CHECK(repeat.update(900, 425) == 0);
}

void testJoystickReleaseStopsRepeatImmediately() {
    ui::JoystickRepeat repeat;
    arm(repeat);
    repeat.update(40, 0);
    CHECK(repeat.update(70, 0) == 1);
    CHECK(repeat.update(570, 512) == 0); // At repeat deadline, but released.
    CHECK(repeat.update(600, 512) == 0);
    CHECK(repeat.update(1000, 512) == 0);
}

void testJoystickStallCannotReplayBacklog() {
    ui::JoystickRepeat repeat;
    arm(repeat);
    repeat.update(40, 0);
    CHECK(repeat.update(70, 0) == 1);
    CHECK(repeat.update(5000, 0) == 1);
    CHECK(repeat.update(5000, 0) == 0);
    CHECK(repeat.update(5299, 0) == 0);
    CHECK(repeat.update(5300, 0) == 1);
}

void testJoystickViewChangeRequiresStableNeutral() {
    ui::JoystickRepeat repeat;
    arm(repeat);
    repeat.update(40, 0);
    CHECK(repeat.update(70, 0) == 1);
    repeat.reset(100); // New menu or select button.
    CHECK(repeat.update(1000, 0) == 0);
    CHECK(repeat.update(2000, 512) == 0);
    CHECK(repeat.update(2029, 512) == 0);
    CHECK(repeat.update(2030, 0) == 0); // Not centred for 30 ms yet: blocked.
    CHECK(repeat.update(3000, 512) == 0);
    CHECK(repeat.update(3030, 512) == 0);
    CHECK(repeat.update(3040, 0) == 0);
    CHECK(repeat.update(3070, 0) == 1);
}

void testJoystickHeldAtBootDoesNotNavigate() {
    ui::JoystickRepeat repeat;
    repeat.reset(0);
    for (uint32_t now = 0; now <= 5000; now += 10)
        CHECK(repeat.update(now, 1023) == 0);
    CHECK(repeat.update(5010, 512) == 0);
    CHECK(repeat.update(5040, 512) == 0);
    CHECK(repeat.update(5050, 1023) == 0);
    CHECK(repeat.update(5080, 1023) == -1);
}

void testJoystickTimerRollover() {
    ui::JoystickRepeat repeat;
    const uint32_t start = UINT32_MAX - 50U;
    arm(repeat, start);
    CHECK(repeat.update(start + 40U, 0) == 0);
    CHECK(repeat.update(start + 70U, 0) == 1);
    CHECK(repeat.update(start + 569U, 0) == 0);
    CHECK(repeat.update(start + 570U, 0) == 1);
    CHECK(repeat.update(start + 870U, 0) == 1);
}

void testInvalidJoystickADCRequiresRearming() {
    ui::JoystickRepeat repeat;
    arm(repeat);
    repeat.update(40, 0);
    CHECK(repeat.update(70, 0) == 1);
    CHECK(repeat.update(80, 65535) == 0);
    CHECK(repeat.update(1000, 0) == 0);
    CHECK(repeat.update(1010, 512) == 0);
    CHECK(repeat.update(1040, 512) == 0);
    CHECK(repeat.update(1050, 0) == 0);
    CHECK(repeat.update(1080, 0) == 1);
}

void testButtonCandidatesExcludePeripheralAndMenuPins() {
    const uint8_t excluded[] = {0, 1, board::LedDataPin, board::BuzzerPin,
        board::StatusLedPin, 20, 21, board::MenuButtonPin,
        board::FirstAnalogGpioPin + board::MenuJoystickChannel};
    debug::PinSet candidates;
    for (uint8_t index = 0; index < debug::ButtonCandidateCount; ++index) {
        const uint8_t pin = debug::buttonCandidatePin(index);
        CHECK(debug::canTestButton(pin));
        CHECK(!candidates.contains(pin));
        candidates.set(pin);
    }
    CHECK(candidates.count() == 61);
    CHECK(debug::ButtonDebugRowCount == 65);
    for (uint8_t pin : excluded) {
        CHECK(!debug::canTestButton(pin));
        CHECK(!candidates.contains(pin));
    }
    CHECK(candidates.contains(board::GameButtonPin));
    CHECK(candidates.contains(board::FirstAnalogGpioPin));
    CHECK(candidates.contains(board::GpioPinCount - 1));
    CHECK(debug::buttonCandidatePin(61) == 255);
}

void testButtonTestAddsAndRestoresOnlyMissingPullups() {
    debug::ButtonTester tester(readMode, setPullup, readPressed);
    tester.setEnabled(true, 0);
    CHECK(tester.testedCount() == 61);
    tester.setEnabled(true, 1);
    tester.update(10);
    tester.setEnabled(false, 20);
    tester.setEnabled(false, 21);
    for (uint8_t pin = 0; pin < board::GpioPinCount; ++pin) {
        const bool existingPullup = hasInitialPullup(pin);
        if (debug::canTestButton(pin)) {
            CHECK(enables[pin] == (existingPullup ? 0U : 1U));
            CHECK(disables[pin] == (existingPullup ? 0U : 1U));
            CHECK(pinModes[pin] == (existingPullup ? debug::ButtonPinMode::PullupInput : debug::ButtonPinMode::FloatingInput));
            CHECK(reads[pin] == 1);
        } else {
            CHECK(enables[pin] == 0 && disables[pin] == 0 && reads[pin] == 0);
        }
    }
}

void testButtonScanSkipsAnAlreadyConfiguredOutput() {
    pinModes[UnusedDigitalPin] = debug::ButtonPinMode::Output;
    pressed[UnusedDigitalPin] = true;
    debug::ButtonTester tester(readMode, setPullup, readPressed);
    tester.setEnabled(true, 0);
    CHECK(tester.testedCount() == 60);
    tester.update(1);
    tester.update(31);
    CHECK(tester.pressedCount() == 0);
    CHECK(reads[UnusedDigitalPin] == 0 && enables[UnusedDigitalPin] == 0);
    char text[16];
    tester.formatValue(buttonRowForPin(UnusedDigitalPin), text, sizeof(text));
    CHECK(strcmp(text, "OUT") == 0);
    tester.setEnabled(false, 40);
    CHECK(pinModes[UnusedDigitalPin] == debug::ButtonPinMode::Output);
}

void testNewOutputOwnerCannotBeOverwrittenOnTestExit() {
    debug::ButtonTester tester(readMode, setPullup, readPressed);
    tester.setEnabled(true, 0);
    CHECK(enables[UnusedDigitalPin] == 1);
    pinModes[UnusedDigitalPin] = debug::ButtonPinMode::Output;
    CHECK(tester.update(10));
    CHECK(tester.testedCount() == 60);
    CHECK(reads[UnusedDigitalPin] == 0);
    tester.setEnabled(false, 20);
    CHECK(disables[UnusedDigitalPin] == 0);
    CHECK(pinModes[UnusedDigitalPin] == debug::ButtonPinMode::Output);
}

void testButtonPressAndReleaseAreDebounced() {
    debug::ButtonTester tester(readMode, setPullup, readPressed);
    tester.setEnabled(true, 0);
    pressed[board::GameButtonPin] = true;
    CHECK(!tester.update(10));
    pressed[board::GameButtonPin] = false;
    CHECK(!tester.update(20));
    pressed[board::GameButtonPin] = true;
    CHECK(!tester.update(25));
    CHECK(!tester.update(54));
    CHECK(tester.update(55));
    CHECK(tester.isPressed(board::GameButtonPin));
    CHECK(!tester.update(1000));
    pressed[board::GameButtonPin] = false;
    CHECK(!tester.update(1001));
    CHECK(!tester.update(1030));
    CHECK(tester.update(1031));
    CHECK(!tester.isPressed(board::GameButtonPin));
}

void testLastButtonPressRemainsVisibleAfterRelease() {
    debug::ButtonTester tester(readMode, setPullup, readPressed);
    tester.setEnabled(true, 0);
    pressed[board::GameButtonPin] = true;
    tester.update(1);
    CHECK(tester.update(31));
    pressed[board::GameButtonPin] = false;
    tester.update(32);
    CHECK(tester.update(62));
    char text[16];
    char expected[16];
    tester.formatValue(0, text, sizeof(text));
    snprintf(expected, sizeof(expected), "D%u", static_cast<unsigned int>(board::GameButtonPin));
    CHECK(strcmp(text, expected) == 0);
    tester.formatValue(1, text, sizeof(text));
    CHECK(strcmp(text, "NINCS") == 0);
    pressed[47] = true;
    tester.update(100);
    CHECK(tester.update(130));
    tester.formatValue(0, text, sizeof(text));
    CHECK(strcmp(text, "D47") == 0);
}

void testMultipleButtonsAndOverflowCount() {
    debug::ButtonTester tester(readMode, setPullup, readPressed);
    tester.setEnabled(true, 0);
    for (uint8_t pin = 47; pin <= 51; ++pin) pressed[pin] = true;
    tester.update(1);
    CHECK(tester.update(31));
    CHECK(tester.pressedCount() == 5);
    char text[16];
    tester.formatValue(1, text, sizeof(text));
    CHECK(strcmp(text, "D47 D48 D49 +2") == 0);
    tester.formatValue(2, text, sizeof(text));
    CHECK(strcmp(text, "5") == 0);
    tester.formatValue(3, text, sizeof(text));
    CHECK(strcmp(text, "61") == 0);
}

void testButtonTestWorksWhenButtonHeldOnEntry() {
    pressed[board::GameButtonPin] = true;
    debug::ButtonTester tester(readMode, setPullup, readPressed);
    tester.setEnabled(true, 0);
    CHECK(!tester.update(0));
    CHECK(tester.update(30));
    CHECK(tester.isPressed(board::GameButtonPin));
    CHECK(!tester.update(10000));
}

void testButtonDebounceTimerRollover() {
    const uint32_t now = UINT32_MAX - 10U;
    debug::ButtonTester tester(readMode, setPullup, readPressed);
    tester.setEnabled(true, now);
    pressed[board::GameButtonPin] = true;
    CHECK(!tester.update(now + 1U));
    CHECK(!tester.update(now + 30U));
    CHECK(tester.update(now + 31U));
    pressed[board::GameButtonPin] = false;
    CHECK(!tester.update(now + 32U));
    CHECK(tester.update(now + 62U));
    CHECK(tester.pressedCount() == 0);
}

void testButtonTestDisabledAndReentryLifecycle() {
    debug::ButtonTester tester(readMode, setPullup, readPressed);
    CHECK(!tester.update(100));
    tester.setEnabled(true, 100);
    pressed[board::GameButtonPin] = true;
    tester.update(101);
    tester.update(131);
    tester.setEnabled(false, 132);
    const unsigned int oldReads = reads[board::GameButtonPin];
    CHECK(!tester.update(1000));
    CHECK(reads[board::GameButtonPin] == oldReads);
    pressed[board::GameButtonPin] = false;
    tester.setEnabled(true, 1001);
    char text[16];
    tester.formatValue(0, text, sizeof(text));
    CHECK(strcmp(text, "NINCS") == 0);
    CHECK(tester.testedCount() == 61);
    CHECK(!tester.update(1040));
}

void testDigitalOutputCandidatesComeFromWorkbookAndSkipReservedPins() {
    uint8_t expectedCount = 0;
    for (uint8_t pin = 0; pin < board::DigitalPinCount; ++pin) {
        bool workbookOutput = false;
        for (const generated::pins::PinDefinition& definition : generated::pins::PinTable) {
            if (definition.type == generated::pins::PinType::DigitalOut &&
                (definition.activeLevel == generated::pins::ActiveLevel::High ||
                 definition.activeLevel == generated::pins::ActiveLevel::Low) &&
                definition.primaryPin == pin)
                workbookOutput = true;
        }
        const bool expected = workbookOutput && debug::digitalRole(pin) == debug::DigitalRole::Input;
        CHECK(debug::canTestDigitalOutput(pin) == expected);
        if (expected) CHECK(debug::digitalOutputCandidatePin(expectedCount++) == pin);
    }
    CHECK(expectedCount > 1);
    CHECK(debug::digitalOutputCandidateCount() == expectedCount);
    CHECK(debug::digitalOutputCandidatePin(expectedCount) == 255);
    CHECK(!debug::canTestDigitalOutput(board::LedDataPin));
    CHECK(!debug::canTestDigitalOutput(board::GameButtonPin));
    CHECK(!debug::canTestDigitalOutput(board::MenuButtonPin));
    CHECK(!debug::canTestDigitalOutput(0) && !debug::canTestDigitalOutput(1));
    CHECK(!debug::canTestDigitalOutput(255));
}

void testDigitalOutputDriveSelectionAndResetAreScoped() {
    debug::DigitalOutputTester tester(writeOutput, resetOutput);
    const uint8_t pin = debug::digitalOutputCandidatePin(0);
    char text[16];
    tester.formatStatus(text, sizeof(text));
    CHECK(strcmp(text, "NINCS") == 0);
    CHECK(!tester.driveSelected(true));
    tester.setEnabled(true);
    checkOutputStatus(tester, pin, "---");
    CHECK(tester.driveSelected(true));
    checkOutputStatus(tester, pin, "HIGH");
    CHECK(outputWrites[pin] == 1);
    CHECK(outputHigh[pin]);
    tester.setEnabled(true);
    CHECK(outputWrites[pin] == 1);
    CHECK(tester.driveSelected(false));
    checkOutputStatus(tester, pin, "LOW");
    CHECK(outputWrites[pin] == 2);
    CHECK(!outputHigh[pin]);
    tester.setEnabled(false);
    tester.setEnabled(false);
    CHECK(outputResets[pin] == 1);
    CHECK(!tester.driveSelected(true) && !tester.selectNext());
    tester.formatStatus(text, sizeof(text));
    CHECK(strcmp(text, "NINCS") == 0);
    tester.setEnabled(true);
    checkOutputStatus(tester, pin, "---");
    tester.setEnabled(false);
    CHECK(outputResets[pin] == 1);
}

void testDigitalOutputSelectionWrapsAndResetsOnlyDrivenPins() {
    debug::DigitalOutputTester tester(writeOutput, resetOutput);
    const uint8_t count = debug::digitalOutputCandidateCount();
    const uint8_t first = debug::digitalOutputCandidatePin(0);
    const uint8_t last = debug::digitalOutputCandidatePin(count - 1);
    tester.setEnabled(true);
    CHECK(tester.driveSelected(true));
    CHECK(tester.selectNext());
    CHECK(outputResets[first] == 1 && !outputHigh[first]);
    checkOutputStatus(tester, debug::digitalOutputCandidatePin(1), "---");
    for (uint8_t index = 2; index < count; ++index) {
        CHECK(tester.selectNext());
        CHECK(tester.selectedPin() == debug::digitalOutputCandidatePin(index));
    }
    CHECK(tester.selectNext() && tester.selectedPin() == first);
    CHECK(tester.selectPrevious() && tester.selectedPin() == last);
    CHECK(tester.driveSelected(true));
    tester.setEnabled(false);
    CHECK(outputResets[first] == 1 && outputResets[last] == 1);
    CHECK(!outputHigh[last]);
    for (uint8_t index = 1; index < count - 1; ++index) {
        const uint8_t pin = debug::digitalOutputCandidatePin(index);
        CHECK(outputWrites[pin] == 0 && outputResets[pin] == 0);
    }
}

void testWorkbookInputsIncludeRemappedGameAndBothSwitchContacts() {
    CHECK(board::isWorkbookDigitalInput(board::GameButtonPin));
    const generated::pins::PinDefinition contacts =
        generated::pins::definition(generated::pins::ControlId::Sw06);
    CHECK(contacts.secondaryPin != generated::pins::InvalidPin);
    CHECK(board::isWorkbookDigitalInput(contacts.primaryPin));
    CHECK(board::isWorkbookDigitalInput(contacts.secondaryPin));
    CHECK(!board::isWorkbookDigitalInput(UnusedDigitalPin));
    CHECK(!board::isWorkbookDigitalInput(board::DigitalPinCount));
    CHECK(!board::isWorkbookDigitalInput(255));
    CHECK(board::PotentiometerChannel ==
        generated::pins::pinNumber(generated::pins::ControlId::Pot1));
    CHECK(board::PotentiometerChannel < board::AnalogChannelCount);
    for (const generated::pins::IlluminatedSwitchDefinition& pair : generated::pins::IkarusSwitches) {
        const uint8_t input = generated::pins::pinNumber(pair.inputControl);
        const uint8_t led = generated::pins::pinNumber(pair.ledControl);
        CHECK(input != led);
        CHECK(board::isWorkbookDigitalInput(input));
        CHECK(!board::isWorkbookDigitalInput(led));
        CHECK(debug::canTestDigitalOutput(led));
    }
}

void testIkarusSwitchInputDoesNotAutomaticallyDriveItsLed() {
    debug::ButtonTester tester(readMode, setPullup, readPressed);
    tester.setEnabled(true, 0);
    const uint8_t input = generated::pins::pinNumber(generated::pins::IkarusSwitches[0].inputControl);
    pressed[input] = true;
    CHECK(!tester.update(1));
    CHECK(tester.update(31));
    CHECK(tester.isPressed(input));
    tester.setEnabled(false, 32);
    for (uint8_t pin = 0; pin < board::DigitalPinCount; ++pin)
        CHECK(outputWrites[pin] == 0 && outputResets[pin] == 0);
}

void testLampMetadataKeepsStripSeparateFromGpio() {
    CHECK(generated::pins::LampPairCount == 22);
    for (const auto& pair : generated::pins::LampPairs) {
        const auto input = generated::pins::definition(pair.inputControl);
        const auto lamp = generated::pins::definition(pair.lampControl);
        CHECK(input.type == generated::pins::PinType::DigitalIn);
        CHECK(lamp.type == generated::pins::PinType::DigitalOut);
        if (lamp.activeLevel == generated::pins::ActiveLevel::Ws2812b) {
            CHECK(lamp.primaryPin == board::LedDataPin);
            CHECK(!debug::canTestDigitalOutput(lamp.primaryPin));
        } else {
            CHECK(lamp.activeLevel == generated::pins::ActiveLevel::High);
            CHECK(!debug::digitalOutputInactiveHigh(lamp.primaryPin));
        }
    }
}

void testButtonLampGuardDrivesLowAndRestoresInputs() {
    debug::ButtonLampGuard lamps(readMode, writeLamp, restoreLamp);
    debug::ButtonTester buttons(readMode, setPullup, readPressed);
    const uint8_t pullupPin = debug::digitalOutputCandidatePin(0);
    pinModes[pullupPin] = debug::ButtonPinMode::PullupInput;
    lamps.setEnabled(true);
    lamps.setEnabled(true);
    buttons.setEnabled(true, 0);
    CHECK(buttons.testedCount() == 61 - debug::digitalOutputCandidateCount());
    for (uint8_t i = 0; i < debug::digitalOutputCandidateCount(); ++i) {
        const uint8_t pin = debug::digitalOutputCandidatePin(i);
        CHECK(pinModes[pin] == debug::ButtonPinMode::Output);
        CHECK(outputWrites[pin] == 1 && !outputHigh[pin]);
        CHECK(enables[pin] == 0);
    }
    buttons.setEnabled(false, 1);
    lamps.setEnabled(false);
    lamps.setEnabled(false);
    for (uint8_t i = 0; i < debug::digitalOutputCandidateCount(); ++i) {
        const uint8_t pin = debug::digitalOutputCandidatePin(i);
        CHECK(outputResets[pin] == 1);
        CHECK(pinModes[pin] == (pin == pullupPin ? debug::ButtonPinMode::PullupInput
                                               : debug::ButtonPinMode::FloatingInput));
    }
    lamps.setEnabled(true);
    lamps.setEnabled(false);
    CHECK(outputWrites[pullupPin] == 2 && outputResets[pullupPin] == 2);
    CHECK(outputWrites[board::LedDataPin] == 0);
}

void testButtonLampGuardPreservesOtherOwners() {
    debug::ButtonLampGuard lamps(readMode, writeLamp, restoreLamp);
    const uint8_t existing = debug::digitalOutputCandidatePin(0);
    const uint8_t changed = debug::digitalOutputCandidatePin(1);
    pinModes[existing] = debug::ButtonPinMode::Output;
    outputHigh[existing] = true;
    lamps.setEnabled(true);
    CHECK(outputWrites[existing] == 0 && outputHigh[existing]);
    pinModes[changed] = debug::ButtonPinMode::PullupInput;
    lamps.setEnabled(false);
    CHECK(outputResets[existing] == 0 && outputHigh[existing]);
    CHECK(outputResets[changed] == 0);
    CHECK(pinModes[changed] == debug::ButtonPinMode::PullupInput);
}

void testAllButtonRowsAreBoundedAndStable() {
    debug::ButtonTester tester(readMode, setPullup, readPressed);
    tester.setEnabled(true, 0);
    for (uint8_t pin = 0; pin < board::GpioPinCount; ++pin) pressed[pin] = true;
    tester.update(1);
    tester.update(31);
    CHECK(tester.pressedCount() == 61);
    for (size_t capacity = 0; capacity <= 21; ++capacity) {
        for (uint8_t row = 0; row <= debug::ButtonDebugRowCount; ++row) {
            char storage[23];
            memset(storage, '#', sizeof(storage));
            tester.formatValue(row, storage + 1, capacity);
            CHECK(storage[0] == '#' && storage[capacity + 1] == '#');
            if (capacity != 0) CHECK(memchr(storage + 1, '\0', capacity) != nullptr);
            memset(storage, '#', sizeof(storage));
            tester.formatName(row, storage + 1, capacity);
            CHECK(storage[0] == '#' && storage[capacity + 1] == '#');
            if (capacity != 0) CHECK(memchr(storage + 1, '\0', capacity) != nullptr);
        }
    }
    tester.formatValue(0, nullptr, 16);
    tester.formatName(0, nullptr, 16);
}

void testButtonTestWithoutReadersIsSafe() {
    debug::ButtonTester tester;
    tester.setEnabled(true, 0);
    CHECK(!tester.update(100));
    CHECK(tester.testedCount() == 0 && tester.pressedCount() == 0);
    tester.setEnabled(false, 101);
}

void testAllAvailableAnalogButtonsAreDetected() {
    debug::ButtonTester tester(readMode, setPullup, readPressed);
    tester.setEnabled(true, 0);
    for (uint8_t channel = 0; channel < board::AnalogChannelCount; ++channel)
        pressed[board::FirstAnalogGpioPin + channel] = true;
    tester.update(1);
    CHECK(tester.update(31));
    CHECK(tester.pressedCount() == 15);
    for (uint8_t channel = 0; channel < board::AnalogChannelCount; ++channel) {
        const uint8_t pin = board::FirstAnalogGpioPin + channel;
        if (channel == board::MenuJoystickChannel) {
            CHECK(!tester.isPressed(pin));
            CHECK(reads[pin] == 0 && enables[pin] == 0);
        } else {
            CHECK(tester.isPressed(pin));
            CHECK(reads[pin] == 2 && enables[pin] == 1);
        }
    }
    char text[16];
    tester.formatValue(0, text, sizeof(text));
    CHECK(strcmp(text, "A0 A1 A2 +12") == 0);
}

void testAnalogAndDigitalNamesShareButtonReport() {
    debug::ButtonTester tester(readMode, setPullup, readPressed);
    tester.setEnabled(true, 0);
    pressed[board::GameButtonPin] = true;
    pressed[board::FirstAnalogGpioPin] = true;
    pressed[board::GpioPinCount - 1] = true;
    tester.update(1);
    CHECK(tester.update(31));
    char text[16];
    char expected[16];
    tester.formatValue(1, text, sizeof(text));
    snprintf(expected, sizeof(expected), "D%u A0 A15", static_cast<unsigned int>(board::GameButtonPin));
    CHECK(strcmp(text, expected) == 0);
    for (uint8_t row = debug::ButtonSummaryRows; row < debug::ButtonDebugRowCount; ++row) {
        const uint8_t pin = debug::buttonCandidatePin(row - debug::ButtonSummaryRows);
        tester.formatName(row, text, sizeof(text));
        if (pin == board::FirstAnalogGpioPin) CHECK(strcmp(text, "A0") == 0);
        if (pin == board::GpioPinCount - 1) CHECK(strcmp(text, "A15") == 0);
    }
}

void testAnalogOutputAndExistingPullupArePreserved() {
    const uint8_t outputPin = board::FirstAnalogGpioPin + 3;
    const uint8_t pullupPin = board::FirstAnalogGpioPin + 15;
    pinModes[outputPin] = debug::ButtonPinMode::Output;
    pinModes[pullupPin] = debug::ButtonPinMode::PullupInput;
    debug::ButtonTester tester(readMode, setPullup, readPressed);
    tester.setEnabled(true, 0);
    CHECK(tester.testedCount() == 60);
    tester.update(1);
    tester.setEnabled(false, 31);
    CHECK(enables[outputPin] == 0 && disables[outputPin] == 0 && reads[outputPin] == 0);
    CHECK(enables[pullupPin] == 0 && disables[pullupPin] == 0);
    CHECK(pinModes[outputPin] == debug::ButtonPinMode::Output);
    CHECK(pinModes[pullupPin] == debug::ButtonPinMode::PullupInput);
    for (uint8_t channel = 0; channel < board::AnalogChannelCount; ++channel) {
        const uint8_t pin = board::FirstAnalogGpioPin + channel;
        if (channel == board::MenuJoystickChannel || pin == outputPin || pin == pullupPin) continue;
        CHECK(enables[pin] == 1 && disables[pin] == 1);
        CHECK(pinModes[pin] == debug::ButtonPinMode::FloatingInput);
    }
}

void testAnalogPinSetBoundsAndJoystickProtection() {
    debug::PinSet pins;
    pins.set(board::FirstAnalogGpioPin);
    pins.set(board::GpioPinCount - 1);
    pins.set(board::GpioPinCount);
    pins.set(255);
    CHECK(pins.count() == 2);
    CHECK(!pins.contains(board::GpioPinCount));
    CHECK(!pins.contains(255));
    pins.clear();
    CHECK(pins.count() == 0);
    CHECK(!debug::canTestButton(board::FirstAnalogGpioPin + board::MenuJoystickChannel));
    CHECK(!debug::canTestButton(board::GpioPinCount));
    CHECK(!debug::canTestButton(255));
}

struct Test { const char* name; void (*run)(); };

const Test Tests[] = {
    {"short joystick movements step exactly once", testShortJoystickMovementIsOneStep},
    {"joystick hold repeats fixed one-step intervals", testJoystickHoldRepeatsOneAtFixedIntervals},
    {"joystick force never accelerates", testJoystickForceDoesNotAccelerate},
    {"joystick noise and short spikes rejected", testJoystickNoiseAndShortSpikesAreIgnored},
    {"joystick hysteresis prevents duplicate steps", testJoystickHysteresisPreventsExtraSteps},
    {"joystick release stops repeat immediately", testJoystickReleaseStopsRepeatImmediately},
    {"delayed joystick cannot replay step backlog", testJoystickStallCannotReplayBacklog},
    {"menu change requires stable neutral joystick", testJoystickViewChangeRequiresStableNeutral},
    {"held joystick at boot cannot navigate", testJoystickHeldAtBootDoesNotNavigate},
    {"joystick repeat survives timer rollover", testJoystickTimerRollover},
    {"invalid joystick ADC requires rearming", testInvalidJoystickADCRequiresRearming},
    {"button candidates exclude all reserved pins", testButtonCandidatesExcludePeripheralAndMenuPins},
    {"button test adds and restores only missing pullups", testButtonTestAddsAndRestoresOnlyMissingPullups},
    {"button scan skips configured outputs", testButtonScanSkipsAnAlreadyConfiguredOutput},
    {"button test cannot overwrite a new output owner", testNewOutputOwnerCannotBeOverwrittenOnTestExit},
    {"button detection debounces press and release", testButtonPressAndReleaseAreDebounced},
    {"short button presses retained after release", testLastButtonPressRemainsVisibleAfterRelease},
    {"multiple pressed buttons and overflow count", testMultipleButtonsAndOverflowCount},
    {"button held on test entry is detected", testButtonTestWorksWhenButtonHeldOnEntry},
    {"button tester debounce survives timer rollover", testButtonDebounceTimerRollover},
    {"button test disable and reentry lifecycle", testButtonTestDisabledAndReentryLifecycle},
    {"digital output candidates follow workbook metadata", testDigitalOutputCandidatesComeFromWorkbookAndSkipReservedPins},
    {"digital output drive and reset are scoped", testDigitalOutputDriveSelectionAndResetAreScoped},
    {"multiple workbook outputs wrap and reset safely", testDigitalOutputSelectionWrapsAndResetsOnlyDrivenPins},
    {"workbook inputs retain remapped game and switch contacts", testWorkbookInputsIncludeRemappedGameAndBothSwitchContacts},
    {"Ikarus input and LED output remain independent", testIkarusSwitchInputDoesNotAutomaticallyDriveItsLed},
    {"lamp metadata separates strip and transistor outputs", testLampMetadataKeepsStripSeparateFromGpio},
    {"button lamp guard drives LOW and restores inputs", testButtonLampGuardDrivesLowAndRestoresInputs},
    {"button lamp guard preserves other pin owners", testButtonLampGuardPreservesOtherOwners},
    {"all button report rows honor buffer bounds", testAllButtonRowsAreBoundedAndStable},
    {"button tester with missing IO is safe", testButtonTestWithoutReadersIsSafe},
    {"all available analog GPIO buttons are detected", testAllAvailableAnalogButtonsAreDetected},
    {"digital and analog pin names share button report", testAnalogAndDigitalNamesShareButtonReport},
    {"analog output and existing pullups are preserved", testAnalogOutputAndExistingPullupArePreserved},
    {"analog pin-set bounds and joystick reservation", testAnalogPinSetBoundsAndJoystickProtection}
};

} // namespace

size_t runInputTests() {
    for (const Test& test : Tests) {
        for (uint8_t pin = 0; pin < board::GpioPinCount; ++pin) {
            pinModes[pin] = hasInitialPullup(pin)
                ? debug::ButtonPinMode::PullupInput : debug::ButtonPinMode::FloatingInput;
            pressed[pin] = false;
            reads[pin] = enables[pin] = disables[pin] = 0;
            if (pin < board::DigitalPinCount) {
                outputWrites[pin] = 0;
                outputResets[pin] = 0;
                outputHigh[pin] = false;
            }
        }
        test.run();
        printf("PASS: %s\n", test.name);
    }
    return sizeof(Tests) / sizeof(Tests[0]);
}