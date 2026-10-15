#include "test_debug.h"
#include "debug/InputMonitor.h"
#include "debug/SystemChecks.h"
#include "app/ModeController.h"
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

unsigned int digitalReads[board::DigitalPinCount];
unsigned int analogReads[board::AnalogChannelCount];
unsigned int digitalTotal = 0;
unsigned int analogTotal = 0;
bool invertValues = false;
bool invalidAnalogValues = false;
constexpr uint8_t TestOutputPin = 7;

debug::DigitalState readDigital(uint8_t pin) {
    CHECK(pin < board::DigitalPinCount);
    CHECK(debug::canReadDigital(pin));
    ++digitalReads[pin];
    ++digitalTotal;
    if (pin == TestOutputPin) return debug::DigitalState::Output;
    return (pin == board::GameButtonPin) != invertValues
        ? debug::DigitalState::Low : debug::DigitalState::High;
}

uint16_t readAnalog(uint8_t channel) {
    CHECK(channel < board::AnalogChannelCount);
    ++analogReads[channel];
    ++analogTotal;
    if (invalidAnalogValues && channel == 3) return debug::AnalogOutput;
    if (invalidAnalogValues && channel == 4) return 2048;
    return static_cast<uint16_t>(channel * 64U + (invertValues ? 16U : 32U));
}

void sampleAll(debug::InputMonitor& monitor) {
    monitor.setEnabled(true);
    for (uint8_t tick = 0; tick < board::AnalogChannelCount; ++tick) monitor.update();
}

void testReservedPinPolicy() {
    const uint8_t reserved[] = {0, 1, board::LedDataPin, board::BuzzerPin,
                               board::StatusLedPin, 20, 21};
    unsigned int readable = 0;
    for (uint8_t pin = 0; pin < board::DigitalPinCount; ++pin) {
        bool protectedPin = false;
        for (uint8_t entry : reserved) if (entry == pin) protectedPin = true;
        CHECK(debug::canReadDigital(pin) == !protectedPin);
        if (debug::canReadDigital(pin)) ++readable;
    }
    CHECK(readable == 47);
    CHECK(debug::digitalRole(board::GameButtonPin) == debug::DigitalRole::GameButton);
    CHECK(debug::digitalRole(53) == debug::DigitalRole::MenuButton);
    CHECK(!debug::canReadDigital(54));
    CHECK(!debug::canReadDigital(255));
}

void testDisabledSamplerDoesNoHardwareWork() {
    debug::InputMonitor monitor(readDigital, readAnalog);
    for (unsigned int tick = 0; tick < 100; ++tick) monitor.update();
    CHECK(!monitor.isEnabled());
    CHECK(digitalTotal == 0 && analogTotal == 0);
    CHECK(monitor.digitalValue(board::GameButtonPin) == debug::DigitalState::Pending);
    CHECK(monitor.analogValue(15) == debug::AnalogPending);
}

void testBoundedSamplingCoversEveryInput() {
    debug::InputMonitor monitor(readDigital, readAnalog);
    monitor.setEnabled(true);
    for (uint8_t tick = 0; tick < 16; ++tick) {
        const unsigned int previousDigital = digitalTotal;
        const unsigned int previousAnalog = analogTotal;
        monitor.update();
        CHECK(digitalTotal - previousDigital <= debug::DigitalSamplesPerTick);
        CHECK(analogTotal - previousAnalog == 1);
    }
    for (uint8_t pin = 0; pin < board::DigitalPinCount; ++pin) {
        if (debug::canReadDigital(pin)) {
            CHECK(digitalReads[pin] >= 1);
            CHECK(monitor.digitalValue(pin) != debug::DigitalState::Pending);
        } else {
            CHECK(digitalReads[pin] == 0);
            CHECK(monitor.digitalValue(pin) == debug::DigitalState::Reserved);
        }
    }
    for (uint8_t channel = 0; channel < board::AnalogChannelCount; ++channel) {
        CHECK(analogReads[channel] == 1);
        CHECK(monitor.analogValue(channel) == channel * 64U + 32U);
    }
}

void testLiveSamplesRefreshWithoutReinitializing() {
    debug::InputMonitor monitor(readDigital, readAnalog);
    sampleAll(monitor);
    CHECK(monitor.digitalValue(board::GameButtonPin) == debug::DigitalState::Low);
    invertValues = true;
    sampleAll(monitor);
    CHECK(monitor.digitalValue(board::GameButtonPin) == debug::DigitalState::High);
    CHECK(monitor.digitalValue(3) == debug::DigitalState::Low);
    CHECK(monitor.analogValue(15) == 976);
    for (uint8_t channel = 0; channel < board::AnalogChannelCount; ++channel)
        CHECK(analogReads[channel] == 2);
}

void testEnableIsIdempotentAndReentryClearsStaleData() {
    debug::InputMonitor monitor(readDigital, readAnalog);
    monitor.setEnabled(true);
    monitor.update();
    monitor.setEnabled(true);
    monitor.update();
    CHECK(monitor.digitalValue(6) == debug::DigitalState::High);
    CHECK(monitor.analogValue(1) == 96);
    monitor.setEnabled(false);
    const unsigned int oldTotal = digitalTotal;
    monitor.update();
    CHECK(digitalTotal == oldTotal);
    monitor.setEnabled(true);
    CHECK(monitor.digitalValue(6) == debug::DigitalState::Pending);
    CHECK(monitor.analogValue(1) == debug::AnalogPending);
    monitor.update();
    CHECK(monitor.analogValue(0) == 32);
}

void testMissingReadersShowPendingNotFakeValues() {
    debug::InputMonitor monitor;
    sampleAll(monitor);
    char text[16];
    monitor.formatDigitalValue(board::GameButtonPin, text, sizeof(text));
    CHECK(strcmp(text, "---") == 0);
    monitor.formatAnalogValue(15, text, sizeof(text));
    CHECK(strcmp(text, "---") == 0);
    CHECK(monitor.analogValue(255) == debug::AnalogPending);
}

void testInputLabelsAndReservedRoles() {
    debug::InputMonitor monitor(readDigital, readAnalog);
    char text[16];
    char expected[16];
    debug::InputMonitor::formatDigitalName(board::GameButtonPin, text, sizeof(text));
    snprintf(expected, sizeof(expected), "D%u Jatek", static_cast<unsigned int>(board::GameButtonPin));
    CHECK(strcmp(text, expected) == 0);
    debug::InputMonitor::formatDigitalName(53, text, sizeof(text));
    CHECK(strcmp(text, "D53 Menu") == 0);
    debug::InputMonitor::formatAnalogName(9, text, sizeof(text));
    CHECK(strcmp(text, "A9 Joy") == 0);
    debug::InputMonitor::formatAnalogName(15, text, sizeof(text));
    CHECK(strcmp(text, "A15") == 0);
    monitor.formatDigitalValue(0, text, sizeof(text));
    CHECK(strcmp(text, "UART") == 0);
    monitor.formatDigitalValue(board::LedDataPin, text, sizeof(text));
    CHECK(strcmp(text, "LED") == 0);
    monitor.formatDigitalValue(5, text, sizeof(text));
    CHECK(strcmp(text, "HANG") == 0);
    monitor.formatDigitalValue(20, text, sizeof(text));
    CHECK(strcmp(text, "I2C") == 0);
    monitor.formatDigitalValue(13, text, sizeof(text));
    CHECK(strcmp(text, "OUT") == 0);
}

void testOutputAndInvalidAnalogAreNotInputMeasurements() {
    invalidAnalogValues = true;
    debug::InputMonitor monitor(readDigital, readAnalog);
    sampleAll(monitor);
    char text[16];
    monitor.formatDigitalValue(TestOutputPin, text, sizeof(text));
    CHECK(strcmp(text, "OUT") == 0);
    monitor.formatAnalogValue(3, text, sizeof(text));
    CHECK(strcmp(text, "OUT") == 0);
    monitor.formatAnalogValue(4, text, sizeof(text));
    CHECK(strcmp(text, "ERR") == 0);
    monitor.formatAnalogValue(15, text, sizeof(text));
    CHECK(strcmp(text, "992") == 0);
}

void testDiagnosticTextBoundsForAllRows() {
    debug::InputMonitor monitor(readDigital, readAnalog);
    sampleAll(monitor);
    for (size_t capacity = 0; capacity <= 17; ++capacity) {
        for (uint8_t row = 0; row <= board::DigitalPinCount; ++row) {
            for (uint8_t format = 0; format < 4; ++format) {
                char storage[19];
                memset(storage, '#', sizeof(storage));
                char* buffer = storage + 1;
                if (format == 0) monitor.formatDigitalName(row, buffer, capacity);
                if (format == 1) monitor.formatDigitalValue(row, buffer, capacity);
                if (format == 2) monitor.formatAnalogName(row, buffer, capacity);
                if (format == 3) monitor.formatAnalogValue(row, buffer, capacity);
                CHECK(storage[0] == '#');
                CHECK(storage[capacity + 1] == '#');
                if (capacity != 0) CHECK(memchr(buffer, '\0', capacity) != nullptr);
            }
        }
    }
    monitor.formatDigitalValue(0, nullptr, 16);
    monitor.formatAnalogValue(0, nullptr, 16);
}

void testIndependentTaskHealthAndErrorRows() {
    debug::SystemChecks checks;
    char text[16];
    checks.formatValue(0, text, sizeof(text));
    CHECK(strcmp(text, "---") == 0);
    checks.setTaskHealth(true, false);
    checks.formatValue(0, text, sizeof(text));
    CHECK(strcmp(text, "OK") == 0);
    checks.formatValue(1, text, sizeof(text));
    CHECK(strcmp(text, "FALLBACK") == 0);
    checks.setTaskHealth(false, true);
    checks.formatValue(0, text, sizeof(text));
    CHECK(strcmp(text, "FALLBACK") == 0);
    checks.formatValue(1, text, sizeof(text));
    CHECK(strcmp(text, "OK") == 0);
    checks.formatName(2, text, sizeof(text));
    CHECK(strcmp(text, "LCD 0x27") == 0);
    checks.formatValue(255, text, sizeof(text));
    CHECK(text[0] == '\0');
}

void testLcdAckMissingBusErrorAndTimeout() {
    debug::SystemChecks checks;
    char text[16];
    checks.formatValue(2, text, sizeof(text));
    CHECK(strcmp(text, "---") == 0);
    const char* expected[] = {"OK", "I2C HIBA", "NINCS", "NINCS", "I2C HIBA", "TIMEOUT"};
    for (uint8_t result = 0; result < 6; ++result) {
        checks.setLcdResult(result, false);
        checks.formatValue(2, text, sizeof(text));
        CHECK(strcmp(text, expected[result]) == 0);
    }
    checks.setLcdResult(0, true);
    checks.formatValue(2, text, sizeof(text));
    CHECK(strcmp(text, "TIMEOUT") == 0);
    checks.setLcdResult(0, false);
    checks.recordI2cTimeout();
    checks.setTaskHealth(true, true);
    checks.formatValue(2, text, sizeof(text));
    CHECK(strcmp(text, "TIMEOUT") == 0);
}

void testErrorTextHandlesTinyBuffers() {
    debug::SystemChecks checks;
    checks.setTaskHealth(false, false);
    checks.setLcdResult(4, false);
    for (size_t capacity = 0; capacity <= 17; ++capacity) {
        for (uint8_t row = 0; row <= debug::SystemCheckCount; ++row) {
            char storage[19];
            memset(storage, '#', sizeof(storage));
            checks.formatValue(row, storage + 1, capacity);
            CHECK(storage[0] == '#' && storage[capacity + 1] == '#');
            if (capacity != 0) CHECK(memchr(storage + 1, '\0', capacity) != nullptr);
        }
    }
    checks.formatName(0, nullptr, 17);
    checks.formatValue(0, nullptr, 17);
}

void testDebugIgnoresGameButtonAndHeldExit() {
    game::CycloneGame game;
    audio::SoundEffects effects;
    app::ModeController controller(game, effects);
    controller.select(app::Mode::Cyclone, 0, false);
    CHECK(controller.select(app::Mode::Debug, 1, false));
    for (uint32_t now = 2; now < 1000; ++now) {
        CHECK(!controller.update(now, true));
        controller.updateAudio(now);
    }
    CHECK(game.state() == game::State::Disabled);
    CHECK(!effects.isPlaying());
    CHECK(controller.select(app::Mode::Cyclone, 1000, true));
    CHECK(!controller.update(1100, true));
    CHECK(game.state() == game::State::Idle);
    controller.update(1101, false);
    controller.update(1131, false);
    controller.update(1132, true);
    CHECK(controller.update(1162, true));
    CHECK(game.state() == game::State::Intro);
}

void testRestartFromDebugReturnsToCyclonIdle() {
    game::CycloneGame game;
    audio::SoundEffects effects;
    app::ModeController controller(game, effects);
    controller.select(app::Mode::Debug, 0, true);
    CHECK(controller.restart(1, true));
    CHECK(controller.mode() == app::Mode::Cyclone);
    CHECK(game.state() == game::State::Idle);
    CHECK(!controller.update(100, true));
    CHECK(!effects.isPlaying());
}

struct Test { const char* name; void (*run)(); };

const Test Tests[] = {
    {"all digital pins respect reserved ownership", testReservedPinPolicy},
    {"disabled Debug sampler performs no IO", testDisabledSamplerDoesNoHardwareWork},
    {"bounded scan covers D0-D53 and A0-A15", testBoundedSamplingCoversEveryInput},
    {"input snapshots update live", testLiveSamplesRefreshWithoutReinitializing},
    {"Debug enable/reentry preserves sampling lifecycle", testEnableIsIdempotentAndReentryClearsStaleData},
    {"pending samples are not fake measurements", testMissingReadersShowPendingNotFakeValues},
    {"input and reserved-role labels", testInputLabelsAndReservedRoles},
    {"output pins and invalid ADC readings", testOutputAndInvalidAnalogAreNotInputMeasurements},
    {"all input labels honor buffer bounds", testDiagnosticTextBoundsForAllRows},
    {"independent game/audio task health", testIndependentTaskHealthAndErrorRows},
    {"LCD ACK errors and timeout reporting", testLcdAckMissingBusErrorAndTimeout},
    {"error rows honor buffer bounds", testErrorTextHandlesTinyBuffers},
    {"Debug ignores game presses and held exit", testDebugIgnoresGameButtonAndHeldExit},
    {"restart exits Debug to Cyclon idle", testRestartFromDebugReturnsToCyclonIdle}
};

} // namespace

size_t runDebugTests() {
    for (const Test& test : Tests) {
        memset(digitalReads, 0, sizeof(digitalReads));
        memset(analogReads, 0, sizeof(analogReads));
        digitalTotal = analogTotal = 0;
        invertValues = invalidAnalogValues = false;
        test.run();
        printf("PASS: %s\n", test.name);
    }
    return sizeof(Tests) / sizeof(Tests[0]);
}