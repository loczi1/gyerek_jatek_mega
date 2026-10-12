#include "test_led_strip.h"
#include "debug/LedStripTester.h"
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

void testDefaultStartLightsFirstPixelOnly() {
    debug::LedStripTester test(120000);
    CHECK(!test.isActive());
    CHECK(!test.isLit(0));
    test.start(board::DefaultLedTestCount, 0);
    CHECK(test.isActive());
    CHECK(test.length() == 100);
    CHECK(test.position() == 0);
    for (uint16_t index = 0; index < 101; ++index)
        CHECK(test.isLit(index) == (index == 0));
}

void testOneClickMovesOnePixelAndWrapsAt100() {
    debug::LedStripTester test(120000);
    test.start(100, 0);
    for (uint16_t click = 1; click <= 300; ++click) {
        CHECK(test.step(click));
        CHECK(test.position() == click % 100);
        unsigned int lit = 0;
        for (uint16_t index = 0; index < 100; ++index)
            if (test.isLit(index)) ++lit;
        CHECK(lit == 1);
    }
}

void testEveryLengthOneTo100HasBoundedPosition() {
    debug::LedStripTester test(120000);
    for (uint16_t length = 1; length <= 100; ++length) {
        test.start(length, 0);
        CHECK(test.length() == length);
        for (uint16_t click = 0; click <= length * 2; ++click) {
            CHECK(test.position() < length);
            CHECK(test.position() == click % length);
            CHECK(!test.isLit(length));
            CHECK(test.step(click));
        }
    }
}

void testInvalidLedCountIsClamped() {
    debug::LedStripTester test(120000);
    test.start(0, 0);
    CHECK(test.length() == 1);
    CHECK(test.step(1));
    CHECK(test.position() == 0);
    test.start(101, 2);
    CHECK(test.length() == 100);
    test.start(65535, 3);
    CHECK(test.length() == 100);
}

void testLedTestDoesNotMoveWithoutClicks() {
    debug::LedStripTester test(120000);
    test.start(75, 100);
    for (uint32_t now = 100; now < 120100; now += 100) {
        CHECK(!test.hasTimedOut(now));
        CHECK(test.position() == 0);
        CHECK(test.isLit(0));
    }
}

void testTwoMinuteTimeoutRenewedOnlyByClick() {
    debug::LedStripTester test(120000);
    CHECK(!test.hasTimedOut(120000));
    test.start(100, 1000);
    CHECK(!test.hasTimedOut(120999));
    CHECK(test.hasTimedOut(121000));
    CHECK(test.step(121000));
    CHECK(!test.hasTimedOut(240999));
    CHECK(test.hasTimedOut(241000));
    test.stop();
    CHECK(!test.hasTimedOut(1000000));
}

void testLedTimeoutAcrossMillisRollover() {
    debug::LedStripTester test(120000);
    const uint32_t now = UINT32_MAX - 20U;
    test.start(100, now);
    CHECK(!test.hasTimedOut(now + 119999U));
    CHECK(test.hasTimedOut(now + 120000U));
    CHECK(test.step(now + 120001U));
    CHECK(!test.hasTimedOut(now + 240000U));
    CHECK(test.hasTimedOut(now + 240001U));
}

void testStopClearsLogicalLightAndRestartResetsPosition() {
    debug::LedStripTester test(120000);
    test.start(100, 0);
    for (uint8_t step = 0; step < 99; ++step) test.step(step);
    CHECK(test.position() == 99);
    CHECK(test.isLit(99));
    test.stop();
    CHECK(!test.isActive());
    CHECK(!test.step(1000));
    for (uint16_t index = 0; index < 101; ++index) CHECK(!test.isLit(index));
    test.start(75, 1000);
    CHECK(test.position() == 0);
    CHECK(test.length() == 75);
    CHECK(test.isLit(0));
}

void testLCDPositionIsOneBasedAndBounded() {
    debug::LedStripTester test(120000);
    test.start(100, 0);
    char text[17];
    test.formatPosition(text, sizeof(text));
    CHECK(strcmp(text, "LED 1/100") == 0);
    for (uint8_t step = 0; step < 99; ++step) test.step(step);
    test.formatPosition(text, sizeof(text));
    CHECK(strcmp(text, "LED 100/100") == 0);
    for (size_t capacity = 0; capacity <= 17; ++capacity) {
        char storage[19];
        memset(storage, '#', sizeof(storage));
        test.formatPosition(storage + 1, capacity);
        CHECK(storage[0] == '#' && storage[capacity + 1] == '#');
        if (capacity != 0) CHECK(memchr(storage + 1, '\0', capacity) != nullptr);
    }
    test.formatPosition(nullptr, 17);
    test.stop();
    test.formatPosition(text, sizeof(text));
    CHECK(text[0] == '\0');
}

void testLedDebugDoesNotStartTheGame() {
    game::CycloneGame game;
    audio::SoundEffects effects;
    app::ModeController controller(game, effects);
    controller.select(app::Mode::Debug, 0, false);
    debug::LedStripTester test(120000);
    test.start(100, 0);
    for (uint32_t now = 1; now <= 1000; ++now) {
        CHECK(!controller.update(now, true));
        controller.updateAudio(now);
    }
    CHECK(game.state() == game::State::Disabled);
    CHECK(!effects.isPlaying());
    CHECK(test.position() == 0);
    CHECK(test.step(1001));
    CHECK(test.position() == 1);
    CHECK(game.state() == game::State::Disabled);
}

struct Test { const char* name; void (*run)(); };

const Test Tests[] = {
    {"LED test starts with one white pixel", testDefaultStartLightsFirstPixelOnly},
    {"LED step advances once and wraps at 100", testOneClickMovesOnePixelAndWrapsAt100},
    {"every selectable LED length has bounded positions", testEveryLengthOneTo100HasBoundedPosition},
    {"LED test length rejects zero and clamps overflow", testInvalidLedCountIsClamped},
    {"LED test never advances automatically", testLedTestDoesNotMoveWithoutClicks},
    {"LED test has a renewable two-minute timeout", testTwoMinuteTimeoutRenewedOnlyByClick},
    {"LED timeout survives millis rollover", testLedTimeoutAcrossMillisRollover},
    {"LED stop and restart clear old test state", testStopClearsLogicalLightAndRestartResetsPosition},
    {"LCD LED positions are one-based and safely formatted", testLCDPositionIsOneBasedAndBounded},
    {"LED Debug clicks do not start the game", testLedDebugDoesNotStartTheGame}
};

} // namespace

size_t runLedStripTests() {
    for (const Test& test : Tests) {
        test.run();
        printf("PASS: %s\n", test.name);
    }
    return sizeof(Tests) / sizeof(Tests[0]);
}