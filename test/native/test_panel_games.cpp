#include "test_panel_games.h"
#include "game/PanelGames.h"
#include "game/PairedLampOutputs.h"
#include "debug/DigitalOutputTester.h"
#include "config/PanelConfig.h"
#include <initializer_list>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

namespace {
#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition); \
        exit(EXIT_FAILURE); \
    } \
} while (false)

uint32_t chosen = 0;
uint32_t calls = 0;
uint32_t bounds[3] = {};

uint32_t randomBelow(uint32_t bound) {
    bounds[calls % 3U] = bound;
    ++calls;
    return chosen;
}

uint32_t press(game::PanelGames& panel, uint32_t now, uint32_t mask) {
    CHECK(!panel.update(now + 1U, mask));
    CHECK(!panel.update(now + 30U, mask));
    CHECK(panel.update(now + 31U, mask));
    return now + 31U;
}

uint32_t release(game::PanelGames& panel, uint32_t now) {
    panel.update(now + 1U, 0);
    panel.update(now + 31U, 0);
    return now + 31U;
}

// At a round's initial Gap, advance exactly one transition per update.
uint32_t playback(game::PanelGames& panel, uint32_t now, uint32_t held = 0) {
    const uint8_t length = panel.sequenceLength();
    CHECK(panel.phase() == game::PanelPhase::Gap);
    CHECK(!panel.update(now + 499U, held));
    now += 500U;
    CHECK(panel.update(now, held));
    for (uint8_t i = 0; i < length; ++i) {
        CHECK(panel.phase() == game::PanelPhase::Showing);
        CHECK(panel.litChannel() == chosen % game::PanelChannelCount);
        CHECK(panel.whiteTarget() == (panel.mode() == game::PanelMode::Simon));
        CHECK(panel.distractorMask() == 0);
        CHECK(!panel.update(now + 499U, held));
        now += 500U;
        CHECK(panel.update(now, held));
        CHECK(panel.phase() == game::PanelPhase::Gap);
        CHECK(panel.litChannel() == game::PanelNoChannel);
        CHECK(!panel.update(now + 199U, held));
        now += 200U;
        CHECK(panel.update(now, held));
    }
    CHECK(panel.phase() == game::PanelPhase::Input);
    CHECK(panel.inputIndex() == 0);
    return now;
}

uint32_t start(game::PanelGames& panel, uint32_t now = 0) {
    panel.select(game::PanelMode::Simon, now, 0);
    CHECK(panel.enabledMask() == 0x7ffU);
    now = press(panel, now, uint32_t(1) << 10);
    CHECK(panel.sequenceLength() == 1);
    CHECK(panel.inputIndex() == 0);
    CHECK(panel.takeFeedback() == game::PanelFeedback::None);
    return now;
}

void testRandomAllInputsAndBounds() {
    for (uint8_t i = 0; i < game::PanelInputCount; ++i) {
        game::PanelGames panel(randomBelow);
        chosen = UINT32_MAX;
        calls = 0;
        panel.select(game::PanelMode::Random, 0, 0);
        CHECK(panel.enabledMask() == UINT32_MAX);
        const uint32_t mask = uint32_t(1) << i;
        uint32_t now = press(panel, 0, mask);
        CHECK(panel.phase() == game::PanelPhase::Hit);
        CHECK(panel.litChannel() < game::PanelChannelCount);
        CHECK(panel.hue() != 0);
        CHECK(panel.frequency() >= 220 && panel.frequency() <= 1760);
        CHECK(calls == 3 && bounds[0] == 11 && bounds[1] == 255 && bounds[2] == 1541);
        CHECK(panel.takeFeedback() == game::PanelFeedback::None);
        CHECK(!panel.update(now + 249U, mask));
        CHECK(panel.update(now + 250U, mask));
        CHECK(panel.litChannel() == game::PanelNoChannel && panel.frequency() == 0);
        CHECK(!panel.update(now + 1000U, mask));
        now = release(panel, now + 1000U);
        press(panel, now, mask);
        CHECK(calls == 6);
    }
    game::PanelGames panel(randomBelow);
    panel.select(game::PanelMode::Random, 0, 0);
    chosen = 0;
    uint32_t now = press(panel, 0, 1);
    CHECK(panel.litChannel() == 0 && panel.hue() == 1 && panel.frequency() == 220);
    now = release(panel, now);
    chosen = 1540;
    press(panel, now, 2);
    CHECK(panel.frequency() == 1760);
}

void testRandomBounceRetriggerAndRollover() {
    game::PanelGames panel(randomBelow);
    const uint32_t boot = UINT32_MAX - 20U;
    chosen = 0;
    panel.select(game::PanelMode::Random, boot, 1);
    CHECK(!panel.update(boot + 100U, 1));
    uint32_t now = release(panel, boot + 100U);
    CHECK(!panel.update(now + 1U, 1));
    CHECK(!panel.update(now + 20U, 0));
    CHECK(!panel.update(now + 21U, 1));
    CHECK(!panel.update(now + 50U, 1));
    now += 51U;
    CHECK(panel.update(now, 1));
    // Another independently debounced input restarts the 250 ms frame.
    now = press(panel, now, 3);
    CHECK(!panel.update(now + 249U, 3));
    CHECK(panel.update(now + 250U, 3));
    CHECK(!panel.update(now + 251U, 3));
    panel.select(game::PanelMode::Random, UINT32_MAX - 40U, 0);
    now = press(panel, UINT32_MAX - 40U, 1);
    CHECK(!panel.update(now + 249U, 1));
    CHECK(panel.update(now + 250U, 1));
}

void testSimonStartAndPlaybackHeldIsolation() {
    game::PanelGames panel(randomBelow);
    chosen = 0;
    panel.select(game::PanelMode::Simon, 0, 1);
    CHECK(!panel.update(100, 1));
    uint32_t now = release(panel, 100);
    // Non-Simon bits never start the game.
    CHECK(!panel.update(now + 1U, uint32_t(1) << 31));
    CHECK(!panel.update(now + 31U, uint32_t(1) << 31));
    now = press(panel, now + 31U, 1);
    now = playback(panel, now, 1);
    CHECK(!panel.update(now + 50U, 1));
    CHECK(panel.phase() == game::PanelPhase::Input);
    now = release(panel, now + 50U);
    now = press(panel, now, 1);
    CHECK(panel.phase() == game::PanelPhase::RoundWin);
    CHECK(panel.hue() == 96 && panel.litChannel() == 0);
    CHECK(panel.inputIndex() == 1);
    CHECK(panel.takeFeedback() == game::PanelFeedback::Win);
    CHECK(panel.takeFeedback() == game::PanelFeedback::None);
    CHECK(!panel.update(now + 539U, 1));
    CHECK(panel.update(now + 540U, 1));
    CHECK(panel.sequenceLength() == 2);
}

void testSimonPartialPlaybackPressDiscarded() {
    game::PanelGames panel(randomBelow);
    chosen = 0;
    uint32_t now = start(panel);
    CHECK(panel.update(now + 500U, 0));
    CHECK(panel.update(now + 1000U, 0));
    CHECK(!panel.update(now + 1190U, 1));
    CHECK(panel.update(now + 1200U, 1));
    now += 1200U;
    CHECK(!panel.update(now + 40U, 1));
    CHECK(panel.phase() == game::PanelPhase::Input);
    now = release(panel, now + 40U);
    press(panel, now, 1);
    CHECK(panel.phase() == game::PanelPhase::RoundWin);
}

void testSimonWrongMultipleAndRearm() {
    for (uint32_t mask = 2; mask <= 3; ++mask) {
        game::PanelGames panel(randomBelow);
        chosen = 0;
        uint32_t now = playback(panel, start(panel));
        now = press(panel, now, mask);
        CHECK(panel.phase() == game::PanelPhase::Loss);
        CHECK(panel.hue() == 0 && panel.litChannel() < game::PanelChannelCount);
        CHECK(panel.takeFeedback() == game::PanelFeedback::Loss);
        CHECK(panel.takeFeedback() == game::PanelFeedback::None);
        CHECK(!panel.update(now + 739U, mask));
        CHECK(panel.update(now + 740U, mask));
        CHECK(panel.phase() == game::PanelPhase::Ready);
        CHECK(panel.sequenceLength() == 0);
        CHECK(!panel.update(now + 1000U, mask));
        now = release(panel, now + 1000U);
        press(panel, now, mask);
        CHECK(panel.phase() == game::PanelPhase::Gap);
    }
}

void testSimonUnlimitedInputAndRollover() {
    game::PanelGames panel(randomBelow);
    chosen = 0;
    uint32_t now = playback(panel, start(panel, UINT32_MAX - 1000U));
    CHECK(panel.inputTimeoutMs() == 0);
    CHECK(!panel.update(now + 9999U, 0));
    CHECK(!panel.update(now + 10000U, 0));
    CHECK(!panel.update(now + 100000U, 0));
    CHECK(panel.phase() == game::PanelPhase::Input);
    CHECK(panel.litChannel() == game::PanelNoChannel && !panel.whiteTarget());
    CHECK(panel.takeFeedback() == game::PanelFeedback::None);
    now = press(panel, now + 100000U, 1);
    CHECK(panel.phase() == game::PanelPhase::RoundWin);
    CHECK(panel.takeFeedback() == game::PanelFeedback::Win);
    CHECK(panel.update(now + 540U, 0));
    CHECK(panel.sequenceLength() == 2);
}

void testSimonFullCapacityAndHitTiming() {
    game::PanelGames panel(randomBelow);
    chosen = 10;
    uint32_t now = start(panel);
    const uint32_t answer = uint32_t(1) << chosen;
    for (uint8_t round = 1; round <= game::PanelSequenceCapacity; ++round) {
        CHECK(panel.sequenceLength() == round);
        now = playback(panel, now);
        for (uint8_t index = 0; index < round; ++index) {
            now = press(panel, now, answer);
            CHECK(panel.inputIndex() == index + 1U);
            CHECK(panel.phase() == (index + 1U == round ?
                game::PanelPhase::RoundWin : game::PanelPhase::Hit));
            CHECK(panel.hue() == 96 && panel.litChannel() == chosen);
            CHECK(panel.takeFeedback() == game::PanelFeedback::Win);
            CHECK(panel.takeFeedback() == game::PanelFeedback::None);
            CHECK(!panel.update(now + 539U, answer));
            now += 540U;
            CHECK(panel.update(now, answer));
            if (index + 1U != round) {
                CHECK(panel.phase() == game::PanelPhase::Input);
                CHECK(!panel.update(now + 40U, answer));
                now = release(panel, now + 40U);
            }
        }
    }
    CHECK(calls == game::PanelSequenceCapacity);
    CHECK(panel.phase() == game::PanelPhase::Ready);
    CHECK(panel.sequenceLength() == 0 && panel.inputIndex() == 0);
    CHECK(!panel.update(now + 1000U, answer));
    now = release(panel, now + 1000U);
    press(panel, now, answer);
    CHECK(panel.sequenceLength() == 1);
}

void testSimonEveryChannelAndSequencePrefix() {
    for (uint8_t channel = 0; channel < game::PanelChannelCount; ++channel) {
        game::PanelGames panel(randomBelow);
        chosen = channel;
        uint32_t now = playback(panel, start(panel));
        press(panel, now, uint32_t(1) << channel);
        CHECK(panel.phase() == game::PanelPhase::RoundWin);
    }
    game::PanelGames panel(randomBelow);
    chosen = 0;
    uint32_t now = playback(panel, start(panel));
    now = press(panel, now, 1);
    chosen = 7; // Only the appended entry changes; preserve the old prefix.
    now += 540U;
    CHECK(panel.update(now, 0));
    CHECK(panel.update(now + 500U, 0));
    CHECK(panel.litChannel() == 0);
    CHECK(panel.update(now + 1000U, 0));
    CHECK(panel.update(now + 1200U, 0));
    CHECK(panel.litChannel() == 7);
    CHECK(panel.update(now + 1700U, 0));
    CHECK(panel.update(now + 1900U, 0));
    now += 1900U;
    now = press(panel, now, 1);
    CHECK(panel.phase() == game::PanelPhase::Hit);
    now += 540U;
    CHECK(panel.update(now, 1));
    now = release(panel, now);
    // Level2 remains unlimited after intermediate feedback as well.
    CHECK(!panel.update(now + 9937U, 0));
    CHECK(!panel.update(now + 9938U, uint32_t(1) << 7));
    CHECK(panel.update(now + 9968U, uint32_t(1) << 7));
    CHECK(panel.phase() == game::PanelPhase::RoundWin);
}

void testSimonBoundedLateUpdates() {
    game::PanelGames panel(randomBelow);
    chosen = 0;
    uint32_t now = start(panel);
    now += 100000U;
    CHECK(panel.update(now, 0));
    CHECK(panel.phase() == game::PanelPhase::Showing);
    CHECK(!panel.update(now, 0));
    CHECK(panel.update(now + 100000U, 0));
    CHECK(panel.phase() == game::PanelPhase::Gap);
    CHECK(!panel.update(now + 100000U, 0));
    CHECK(panel.update(now + 200000U, 0));
    CHECK(panel.phase() == game::PanelPhase::Input);
    CHECK(!panel.update(now + 200000U, 0));
}

void testSelectionCancelsEveryPhase() {
    game::PanelGames panel(randomBelow);
    chosen = 0;
    const game::PanelPhase phases[] = {game::PanelPhase::Ready,
        game::PanelPhase::Gap, game::PanelPhase::Showing, game::PanelPhase::Input,
        game::PanelPhase::Hit, game::PanelPhase::RoundWin, game::PanelPhase::Loss};
    for (game::PanelPhase phase : phases) {
        uint32_t now = 0;
        if (phase == game::PanelPhase::Ready) panel.select(game::PanelMode::Simon, 0, 0);
        else {
            now = start(panel);
            if (phase == game::PanelPhase::Showing) {
                now += 500U;
                CHECK(panel.update(now, 0));
            } else if (phase != game::PanelPhase::Gap) {
                now = playback(panel, now);
                if (phase == game::PanelPhase::Hit) {
                    now = press(panel, now, 1);
                    now += 540U;
                    CHECK(panel.update(now, 0));
                    now = playback(panel, now);
                    now = press(panel, now, 1);
                } else if (phase == game::PanelPhase::RoundWin) now = press(panel, now, 1);
                else if (phase == game::PanelPhase::Loss) now = press(panel, now, 2);
            }
        }
        CHECK(panel.phase() == phase);
        panel.select(game::PanelMode::Disabled, now, UINT32_MAX);
        CHECK(panel.phase() == game::PanelPhase::Disabled);
        CHECK(panel.enabledMask() == 0 && panel.frequency() == 0);
        CHECK(panel.litChannel() == game::PanelNoChannel && panel.sequenceLength() == 0);
        CHECK(panel.takeFeedback() == game::PanelFeedback::None);
        CHECK(!panel.update(now + 100000U, 0));
        panel.select(game::PanelMode::Random, now, UINT32_MAX);
        CHECK(!panel.update(now + 100U, UINT32_MAX));
    }
    panel.select(static_cast<game::PanelMode>(255), 0, 0);
    CHECK(panel.mode() == game::PanelMode::Disabled);
    game::PanelGames fallback;
    CHECK(!fallback.update(100, UINT32_MAX));
    fallback.select(game::PanelMode::Random, 0, 0);
    press(fallback, 0, 1);
    CHECK(fallback.litChannel() == 0 && fallback.frequency() == 220);
    fallback.select(game::PanelMode::Random, 100, 1);
    CHECK(fallback.phase() == game::PanelPhase::Ready);
    CHECK(!fallback.update(1000, 1));
}

void testWorkbookPanelMapping() {
    const uint8_t inputPins[] = {33, 34, 35, 44, 27, 43, 26, 41, 30, 31, 32};
    const uint8_t pixels[] = {84, 85, 86, 75, 74, 73, 72, 71, 83, 82, 81};
    for (uint8_t i = 0; i < game::PanelChannelCount; ++i) {
        CHECK(board::panelInputPin(i) == inputPins[i]);
        CHECK(board::panelStripPixel(i) == pixels[i]);
        CHECK(board::isWorkbookDigitalInput(inputPins[i]));
        CHECK(board::panelStripPixel(i) <= 100);
    }
    // Export stays faithful to Excel; only the physical adapter corrects the row.
    CHECK(generated::pins::definition(generated::pins::ControlId::Led03StripDown51).stripPixel == 71);
    CHECK(generated::pins::definition(generated::pins::ControlId::Led03StripDown55).stripPixel == 75);
    CHECK(board::panelStripPixel(11) == 0);
    CHECK(board::panelStripPixel(255) == 0);
    CHECK(board::panelInputPin(11) == generated::pins::InvalidPin);
    CHECK(board::panelInputPin(255) == generated::pins::InvalidPin);
}

void testFlasherSingleTargetWithoutMemory() {
    game::PanelGames panel(randomBelow);
    panel.select(game::PanelMode::Flasher, 0, 0);
    CHECK(panel.enabledMask() == 0x7ffU);
    chosen = 0;
    uint32_t now = press(panel, 0, uint32_t(1) << 10);
    // Beyond Simon's 32-step limit the target must still be a single element.
    for (uint8_t round = 0; round < 70; ++round) {
        const uint8_t target = chosen % game::PanelChannelCount;
        CHECK(panel.sequenceLength() == 1);
        now = playback(panel, now);
        CHECK(panel.litChannel() == target); // Visible while awaiting an answer.
        CHECK(panel.inputIndex() == 0);
        now = press(panel, now, uint32_t(1) << target);
        CHECK(panel.phase() == game::PanelPhase::RoundWin);
        CHECK(panel.hue() == 96);
        CHECK(panel.takeFeedback() == game::PanelFeedback::Win);
        CHECK(panel.takeFeedback() == game::PanelFeedback::None);
        CHECK(!panel.update(now + 539U, 0));
        chosen = (target + 1U) % game::PanelChannelCount;
        now += 540U;
        CHECK(panel.update(now, 0));
        CHECK(panel.phase() == game::PanelPhase::Gap);
        CHECK(panel.sequenceLength() == 1);
    }
}

void testFlasherLossTimeoutAndRollover() {
    for (uint8_t scenario = 0; scenario < 3; ++scenario) {
        game::PanelGames panel(randomBelow);
        chosen = 0;
        uint32_t now = UINT32_MAX - 1000U;
        panel.select(game::PanelMode::Flasher, now, 0);
        now = press(panel, now, 1);
        now = playback(panel, now);
        if (scenario == 2) {
            CHECK(!panel.update(now + 9999U, 0));
            now += 10000U;
            CHECK(panel.update(now, 0));
        } else {
            now = press(panel, now, scenario == 0 ? 2 : 3);
        }
        CHECK(panel.phase() == game::PanelPhase::Loss);
        CHECK(panel.hue() == 0);
        CHECK(panel.takeFeedback() == game::PanelFeedback::Loss);
        CHECK(!panel.update(now + 739U, 0));
        now += 740U;
        CHECK(panel.update(now, 0));
        CHECK(panel.phase() == game::PanelPhase::Ready);
        now = press(panel, now, 1);
        CHECK(panel.phase() == game::PanelPhase::Gap);
        CHECK(panel.sequenceLength() == 1);
    }
}

void testFlasherHeldInputsReselectionAndCancel() {
    game::PanelGames panel(randomBelow);
    chosen = 0;
    panel.select(game::PanelMode::Flasher, 0, 1);
    CHECK(!panel.update(100, 1));
    uint32_t now = release(panel, 100);
    now = press(panel, now, 1);
    now = playback(panel, now, 1);
    CHECK(!panel.update(now + 100U, 1));
    CHECK(panel.phase() == game::PanelPhase::Input);
    now = release(panel, now + 100U);
    now = press(panel, now, 1);
    CHECK(panel.takeFeedback() == game::PanelFeedback::Win);
    panel.select(game::PanelMode::Flasher, now, 1);
    CHECK(panel.phase() == game::PanelPhase::Ready && panel.sequenceLength() == 0);
    CHECK(!panel.update(now + 100U, 1));
    panel.select(game::PanelMode::Disabled, now, 1);
    CHECK(panel.litChannel() == game::PanelNoChannel);
    CHECK(panel.frequency() == 0 && panel.takeFeedback() == game::PanelFeedback::None);
    CHECK(!panel.update(now + 1000U, 1));
}

void testRandom2AllInputsAndChords() {
    for (uint8_t i = 0; i < game::PanelInputCount; ++i) {
        game::PanelGames panel(randomBelow);
        panel.select(game::PanelMode::Random2, 0, 0);
        CHECK(panel.enabledMask() == UINT32_MAX);
        chosen = UINT32_MAX; // Faulty/random boundary callback stays bounded.
        const uint32_t mask = uint32_t(1) << i;
        const uint32_t now = press(panel, 0, mask);
        CHECK(panel.lampInputMask() == mask);
        CHECK(panel.phase() == game::PanelPhase::Hit);
        CHECK(panel.sequenceLength() == 0 && panel.frequency() == 0);
        CHECK(panel.takeFeedback() == game::PanelFeedback::Melody);
        CHECK(panel.takeFeedback() == game::PanelFeedback::None);
        for (uint8_t pixel = 0; pixel < 3; ++pixel) CHECK(panel.discoPixel(pixel) < 70);
        CHECK(!panel.update(now + 149U, mask));
        CHECK(panel.update(now + game::Random2DurationMs, mask));
        CHECK(panel.lampInputMask() == 0);
        CHECK(panel.phase() == game::PanelPhase::Ready);
        CHECK(!panel.update(now + 5000U, mask)); // Holding never repeats.
    }
    game::PanelGames panel;
    panel.select(game::PanelMode::Random2, 0, 0);
    press(panel, 0, UINT32_MAX);
    CHECK(panel.lampInputMask() == UINT32_MAX); // Chords welcome, no loss.
}

void testRandom2BeatsRetriggerAndRollover() {
    game::PanelGames panel(randomBelow);
    uint32_t now = UINT32_MAX - 60U;
    panel.select(game::PanelMode::Random2, now, 0);
    chosen = 1;
    now = press(panel, now, 1);
    CHECK(panel.takeFeedback() == game::PanelFeedback::Melody);
    CHECK(panel.discoPixel(0) == 1);
    chosen = 2;
    CHECK(!panel.update(now + 149U, 1));
    CHECK(panel.update(now + 150U, 1));
    CHECK(panel.discoPixel(0) == 2 && panel.hue() == 2);
    CHECK(panel.lampInputMask() == 1);
    CHECK(panel.takeFeedback() == game::PanelFeedback::None); // Beat != new tune.
    CHECK(panel.update(now + 1000U, 1)); // One bounded late frame.
    CHECK(!panel.update(now + 1000U, 1));
    now = press(panel, now + 1000U, 3); // New input replaces effect while playing.
    CHECK(panel.lampInputMask() == 2);
    CHECK(panel.takeFeedback() == game::PanelFeedback::Melody);
    CHECK(panel.update(now + 1199U, 3)); // Still active, last delayed beat.
    CHECK(panel.phase() == game::PanelPhase::Hit);
    CHECK(panel.update(now + 1200U, 3));
    CHECK(panel.phase() == game::PanelPhase::Ready);
}

void testRandom2BounceReselectAndCancel() {
    game::PanelGames panel;
    panel.select(game::PanelMode::Random2, 0, 1);
    CHECK(!panel.update(100, 1));
    uint32_t now = release(panel, 100);
    CHECK(!panel.update(now + 1U, 1));
    CHECK(!panel.update(now + 20U, 0));
    CHECK(!panel.update(now + 21U, 1));
    CHECK(!panel.update(now + 50U, 1));
    now += 51U;
    CHECK(panel.update(now, 1));
    CHECK(panel.lampInputMask() == 1);
    panel.select(game::PanelMode::Random2, now, 1);
    CHECK(panel.lampInputMask() == 0);
    CHECK(panel.takeFeedback() == game::PanelFeedback::None);
    CHECK(!panel.update(now + 1000U, 1));
    now = release(panel, now + 1000U);
    now = press(panel, now, 1);
    panel.select(game::PanelMode::Disabled, now, 1);
    CHECK(panel.lampInputMask() == 0);
    CHECK(panel.takeFeedback() == game::PanelFeedback::None);
    CHECK(!panel.update(now + 5000U, 1));
    panel.select(game::PanelMode::Random, now, 0);
    press(panel, now, 1);
    CHECK(panel.lampInputMask() == 0); // Original Random never drives GPIO lamps.
}

void testRandom2WorkbookPairs() {
    for (uint8_t i = 0; i < game::PanelChannelCount; ++i)
        CHECK(board::pairedStripPixel(board::panelInputPin(i)) == board::panelStripPixel(i));
    for (const auto& pair : generated::pins::IkarusSwitches) {
        const uint8_t input = generated::pins::pinNumber(pair.inputControl);
        CHECK(board::pairedGpioLamp(input) == generated::pins::pinNumber(pair.ledControl));
        CHECK(board::pairedStripPixel(input) == 0);
    }
    const uint8_t switches[] = {40, 42, 38, 36, 37};
    for (uint8_t i = 0; i < 5; ++i) CHECK(board::pairedStripPixel(switches[i]) == 76 + i);
    const uint8_t unpaired[] = {28, 47, 39, 25, 24, 53, 255};
    for (uint8_t pin : unpaired) {
        CHECK(board::pairedStripPixel(pin) == 0);
        CHECK(board::pairedGpioLamp(pin) == generated::pins::InvalidPin);
    }
}

debug::ButtonPinMode lampModes[board::GpioPinCount];
bool lampHigh[board::GpioPinCount];
unsigned lampWrites = 0;
debug::ButtonPinMode readLampMode(uint8_t pin) { return lampModes[pin]; }
bool readLampHigh(uint8_t pin) { return lampHigh[pin]; }
void writeLamp(uint8_t pin, bool high) {
    CHECK(debug::canTestDigitalOutput(pin));
    lampModes[pin] = debug::ButtonPinMode::Output;
    lampHigh[pin] = high;
    ++lampWrites;
}
void restoreLamp(uint8_t pin, bool pullup) {
    CHECK(lampHigh[pin] == debug::digitalOutputInactiveHigh(pin));
    lampModes[pin] = pullup ? debug::ButtonPinMode::PullupInput : debug::ButtonPinMode::FloatingInput;
    lampHigh[pin] = pullup;
}
void resetLamps() {
    for (uint8_t pin = 0; pin < board::GpioPinCount; ++pin) {
        lampModes[pin] = debug::ButtonPinMode::FloatingInput;
        lampHigh[pin] = false;
    }
    lampWrites = 0;
}

void testPairedGpioLampsAndRestoration() {
    resetLamps();
    lampModes[16] = debug::ButtonPinMode::PullupInput;
    lampHigh[16] = true;
    game::PairedLampOutputs lamps(readLampMode, readLampHigh, writeLamp, restoreLamp);
    const uint8_t inputs[] = {52, 50, 46, 48, 23, 51};
    const uint8_t outputs[] = {2, 16, 17, 3, 18, 19};
    lamps.draw(UINT32_MAX, inputs, 6);
    CHECK(lampWrites == 0); // Disabled is inert.
    lamps.setEnabled(true);
    CHECK(lampWrites == 6);
    lamps.setEnabled(true);
    CHECK(lampWrites == 6);
    for (uint8_t i = 0; i < 6; ++i) {
        lamps.draw(uint32_t(1) << i, inputs, 6);
        for (uint8_t j = 0; j < 6; ++j) CHECK(lampHigh[outputs[j]] == (i == j));
    }
    lamps.draw(0x3f, inputs, 6);
    for (uint8_t pin : outputs) CHECK(lampHigh[pin]);
    const unsigned writes = lampWrites;
    lamps.draw(0x3f, inputs, 6);
    CHECK(lampWrites == writes); // Disco frames don't toggle unchanged GPIO.
    lamps.setEnabled(false); // Cancellation while all six lamps are lit.
    for (uint8_t pin : outputs) {
        CHECK(lampModes[pin] == (pin == 16 ? debug::ButtonPinMode::PullupInput :
                                         debug::ButtonPinMode::FloatingInput));
        CHECK(lampHigh[pin] == (pin == 16));
    }
    lamps.setEnabled(false);
}

void testPairedGpioPreservesOtherOwnersAndBounds() {
    resetLamps();
    lampModes[2] = debug::ButtonPinMode::Output;
    lampHigh[2] = true; // Pre-existing output may not be borrowed.
    game::PairedLampOutputs lamps(readLampMode, readLampHigh, writeLamp, restoreLamp);
    lamps.setEnabled(true);
    CHECK(lampWrites == 5 && lampHigh[2]);
    const uint8_t inputs[] = {52, 50, 46, 48, 23, 51};
    lamps.draw(0x3f, inputs, 6);
    lampModes[16] = debug::ButtonPinMode::PullupInput; // Ownership changed.
    lampHigh[17] = false; // Another writer changed its driven level.
    lamps.draw(0x3f, inputs, 6);
    CHECK(lampModes[16] == debug::ButtonPinMode::PullupInput);
    CHECK(!lampHigh[17]);
    lamps.draw(UINT32_MAX, nullptr, 255); // Null and excessive counts are bounded.
    lamps.setEnabled(false);
    CHECK(lampModes[2] == debug::ButtonPinMode::Output && lampHigh[2]);
    CHECK(lampModes[16] == debug::ButtonPinMode::PullupInput);
    CHECK(lampModes[17] == debug::ButtonPinMode::Output && !lampHigh[17]);
    CHECK(lampModes[board::LedDataPin] == debug::ButtonPinMode::FloatingInput);
    CHECK(lampModes[board::MenuButtonPin] == debug::ButtonPinMode::FloatingInput);
    // Full 32-bit mask, including bit 31; never read inputs beyond the bound.
    resetLamps();
    uint8_t manyInputs[game::PanelInputCount];
    for (uint8_t& pin : manyInputs) pin = 255;
    manyInputs[31] = 51;
    lamps.setEnabled(true);
    lamps.draw(uint32_t(1) << 31, manyInputs, 255);
    CHECK(lampHigh[19]);
    lamps.setEnabled(false);
}

const game::PanelMode MemoryModes[] = {game::PanelMode::Simon, game::PanelMode::Sorrend3,
    game::PanelMode::Sorrend4, game::PanelMode::Sorrend5};
const game::PanelMode SearchModes[] = {game::PanelMode::Sorrend3,
    game::PanelMode::Sorrend4, game::PanelMode::Sorrend5};
uint8_t minimumDistractors(game::PanelMode mode) {
    return mode == game::PanelMode::Simon ? 0 : (mode == game::PanelMode::Sorrend5 ? 8 : 4);
}
uint8_t distractorRange(game::PanelMode mode) {
    return mode == game::PanelMode::Simon ? 1 : (mode == game::PanelMode::Sorrend4 ? 7 : 3);
}
uint32_t profileTimeout(game::PanelMode mode) {
    return mode == game::PanelMode::Simon ? 0U : (mode == game::PanelMode::Sorrend3 ? 3000U :
        (mode == game::PanelMode::Sorrend4 ? 2000U : 1200U));
}
uint32_t firstWarningAt(game::PanelMode mode) {
    return profileTimeout(mode) - (mode == game::PanelMode::Sorrend3 ? 2000U : 1000U);
}
uint32_t secondWarningAt(game::PanelMode mode) {
    return profileTimeout(mode) - (mode == game::PanelMode::Sorrend3 ? 1000U : 500U);
}
uint8_t searchTarget = 0, searchCount = 4;
game::PanelMode randomProfile = game::PanelMode::Sorrend3;
uint32_t searchCalls = 0;

uint32_t searchRandom(uint32_t bound) {
    const uint32_t index = searchCalls++;
    CHECK(index < 13U); // Target, count, at most ten ranks, hue: no rejection loop.
    CHECK(bound == (index == 0 ? 11U : (index == 1 ? distractorRange(randomProfile) :
        (index < 2U + searchCount ? 12U - index : 256U))));
    return index == 0 ? searchTarget :
        (index == 1 ? searchCount - minimumDistractors(randomProfile) : 0U);
}

void checkSearchHidden(const game::PanelGames& panel) {
    CHECK(!panel.whiteTarget() && panel.distractorMask() == 0);
    CHECK(panel.frequency() == 0 && panel.lampInputMask() == 0);
}

uint32_t startSearch(game::PanelGames& panel, game::PanelMode mode, uint32_t now = 0) {
    panel.select(mode, now, 0);
    CHECK(panel.mode() == mode && panel.isMemory());
    CHECK(panel.enabledMask() == 0x7ffU);
    CHECK(panel.inputTimeoutMs() == profileTimeout(mode));
    checkSearchHidden(panel);
    return press(panel, now, 1U << 10);
}

uint32_t searchPlayback(game::PanelGames& panel, uint32_t now, uint32_t held = 0) {
    CHECK(panel.phase() == game::PanelPhase::Gap && panel.sequenceLength() >= 1);
    CHECK(panel.litChannel() == game::PanelNoChannel && panel.hue() == 0);
    checkSearchHidden(panel);
    CHECK(panel.takeFeedback() == game::PanelFeedback::None);
    CHECK(!panel.update(now + 499U, held));
    CHECK(panel.update(now + 500U, held));
    CHECK(panel.phase() == game::PanelPhase::Showing && panel.whiteTarget());
    now += 500U;
    for (uint8_t i = 0; i < panel.sequenceLength(); ++i) {
        CHECK(panel.phase() == game::PanelPhase::Showing && panel.whiteTarget());
        const uint8_t target = panel.litChannel();
        const uint16_t distractors = panel.distractorMask();
        uint8_t hues[game::PanelChannelCount];
        for (uint8_t channel = 0; channel < game::PanelChannelCount; ++channel)
            hues[channel] = panel.distractorHue(channel);
        CHECK(target < game::PanelChannelCount);
        CHECK((distractors & (1U << target)) == 0);
        CHECK(panel.isSearch() ? distractors != 0 : distractors == 0);
        CHECK(!panel.update(now + 499U, held));
        CHECK(panel.litChannel() == target && panel.distractorMask() == distractors);
        for (uint8_t channel = 0; channel < game::PanelChannelCount; ++channel)
            CHECK(hues[channel] == panel.distractorHue(channel));
        CHECK(panel.update(now += 500U, held));
        CHECK(panel.phase() == game::PanelPhase::Gap);
        CHECK(panel.litChannel() == game::PanelNoChannel && panel.hue() == 0);
        checkSearchHidden(panel);
        CHECK(!panel.update(now + 199U, held));
        CHECK(panel.update(now += 200U, held));
    }
    CHECK(panel.phase() == game::PanelPhase::Input && panel.inputIndex() == 0);
    CHECK(panel.litChannel() == game::PanelNoChannel && panel.hue() == 0);
    checkSearchHidden(panel);
    CHECK(panel.takeFeedback() == game::PanelFeedback::None);
    return now;
}

void checkDistractors(const game::PanelGames& panel, uint8_t target, uint8_t count) {
    CHECK(panel.whiteTarget() && panel.litChannel() == target);
    const uint16_t mask = panel.distractorMask();
    CHECK((mask & ~0x7ffU) == 0 && (mask & (1U << target)) == 0);
    uint8_t actual = 0;
    for (uint8_t i = 0; i < game::PanelChannelCount; ++i) {
        if (mask & (1U << i)) ++actual;
        for (uint8_t j = 0; j < i; ++j)
            CHECK(panel.distractorHue(i) != panel.distractorHue(j));
    }
    CHECK(actual == count); // Popcount proves distinct selections, not just draws.
}

void testSearchEveryTargetAndDistractorCount() {
    for (game::PanelMode mode : SearchModes) {
        randomProfile = mode;
        for (searchTarget = 0; searchTarget < game::PanelChannelCount; ++searchTarget) {
            const uint8_t minimum = minimumDistractors(mode);
            for (searchCount = minimum; searchCount < minimum + distractorRange(mode); ++searchCount) {
                game::PanelGames panel(searchRandom);
                searchCalls = 0;
                const uint32_t now = startSearch(panel, mode);
                CHECK(searchCalls == 1); // Distractors are generated only when shown.
                CHECK(panel.update(now + 500U, 0));
                checkDistractors(panel, searchTarget, searchCount);
                CHECK(searchCalls == 3U + searchCount);
                CHECK(!panel.update(now + 999U, 0));
                checkDistractors(panel, searchTarget, searchCount);
                CHECK(panel.update(now + 1000U, 0));
                checkSearchHidden(panel);
                CHECK(panel.update(now + 1200U, 0));
                CHECK(panel.litChannel() == game::PanelNoChannel);
                checkSearchHidden(panel);
                CHECK(searchCalls == 3U + searchCount);
                panel.takeFeedback();
                press(panel, now + 1200U, uint32_t(1) << searchTarget);
                CHECK(panel.phase() == game::PanelPhase::RoundWin && panel.hue() == 96);
                CHECK(panel.litChannel() == searchTarget && panel.inputIndex() == 1);
                CHECK(panel.takeFeedback() == game::PanelFeedback::Win);
                checkSearchHidden(panel);
            }
        }
    }
}

void testSearchConstantFaultyAndNullRandom() {
    const uint32_t values[] = {0, 1, 7, 10, UINT32_MAX};
    for (game::PanelMode mode : SearchModes) {
        for (uint32_t value : values) {
            game::PanelGames panel(randomBelow);
            chosen = value;
            calls = 0;
            const uint32_t now = startSearch(panel, mode);
            CHECK(panel.update(now + 500U, 0));
            const uint8_t count = static_cast<uint8_t>(minimumDistractors(mode) + value % distractorRange(mode));
            CHECK(calls == 3U + count);
            checkDistractors(panel, value % game::PanelChannelCount, count);
            CHECK(!panel.update(now + 999U, 0));
            checkDistractors(panel, value % game::PanelChannelCount, count);
        }
        game::PanelGames fallback;
        const uint32_t now = startSearch(fallback, mode);
        CHECK(fallback.update(now + 500U, 0));
        checkDistractors(fallback, 0, minimumDistractors(mode));
    }
}

void testMemoryGrowingCapacityAndPerAnswerTimers() {
    for (game::PanelMode mode : MemoryModes) {
        game::PanelGames panel(randomBelow);
        chosen = 0;
        uint32_t now = startSearch(panel, mode, UINT32_MAX - 800U);
        for (uint8_t round = 1; round <= game::PanelSequenceCapacity; ++round) {
            CHECK(panel.sequenceLength() == round);
            now = searchPlayback(panel, now);
            for (uint8_t index = 0; index < round; ++index) {
                CHECK(panel.litChannel() == game::PanelNoChannel);
                // Wait nearly a full *new* deadline on every answer. Intermediate
                // green feedback is outside that deadline, even for long sequences.
                if (panel.isSearch()) {
                    CHECK(panel.update(now + firstWarningAt(mode), 0));
                    CHECK(panel.takeFeedback() == game::PanelFeedback::Warning);
                    CHECK(panel.update(now + secondWarningAt(mode), 0));
                    CHECK(panel.takeFeedback() == game::PanelFeedback::Warning);
                    now += panel.inputTimeoutMs() - 32U;
                } else {
                    CHECK(!panel.update(now + 100000U, 0));
                    now += 100000U;
                }
                now = press(panel, now, 1);
                CHECK(panel.inputIndex() == index + 1U && panel.sequenceLength() == round);
                CHECK(panel.phase() == (index + 1U == round ?
                    game::PanelPhase::RoundWin : game::PanelPhase::Hit));
                CHECK(panel.hue() == 96 && panel.litChannel() == 0);
                CHECK(panel.takeFeedback() == game::PanelFeedback::Win);
                CHECK(panel.takeFeedback() == game::PanelFeedback::None);
                checkSearchHidden(panel);
                CHECK(!panel.update(now + 539U, 0));
                CHECK(panel.update(now += 540U, 0));
                if (index + 1U != round) {
                    CHECK(panel.phase() == game::PanelPhase::Input);
                    CHECK(panel.litChannel() == game::PanelNoChannel);
                    CHECK(panel.takeFeedback() == game::PanelFeedback::None);
                }
            }
        }
        CHECK(panel.phase() == game::PanelPhase::Ready && panel.sequenceLength() == 0);
        CHECK(panel.inputIndex() == 0);
        checkSearchHidden(panel);
        now = press(panel, now, 1);
        CHECK(panel.sequenceLength() == 1 && panel.phase() == game::PanelPhase::Gap);
    }
}

void testMemoryPrefixAndPerStepDistractions() {
    for (game::PanelMode mode : MemoryModes) {
        game::PanelGames panel(randomBelow);
        chosen = 2;
        uint32_t now = startSearch(panel, mode);
        chosen = 0;
        CHECK(panel.update(now + 500U, 0));
        CHECK(panel.whiteTarget() && panel.litChannel() == 2);
        if (panel.isSearch()) checkDistractors(panel, 2, minimumDistractors(mode));
        CHECK(panel.update(now + 1000U, 0));
        CHECK(panel.update(now + 1200U, 0));
        CHECK(panel.litChannel() == game::PanelNoChannel);
        now = press(panel, now + 1200U, 1U << 2);
        CHECK(panel.takeFeedback() == game::PanelFeedback::Win);
        chosen = 7; // Only append a new target; never overwrite the prefix.
        CHECK(panel.update(now += 540U, 0));
        CHECK(panel.sequenceLength() == 2);
        const uint32_t beforePlayback = calls;
        chosen = 0;
        CHECK(panel.update(now + 500U, 0));
        CHECK(panel.whiteTarget() && panel.litChannel() == 2);
        if (panel.isSearch()) checkDistractors(panel, 2, minimumDistractors(mode));
        CHECK(calls == beforePlayback + (panel.isSearch() ? minimumDistractors(mode) + 2U : 0U));
        const uint32_t firstCalls = calls;
        const uint16_t firstMask = panel.distractorMask();
        const uint8_t firstHue = panel.distractorHue(0);
        chosen = 1;
        CHECK(!panel.update(now + 999U, 0));
        CHECK(panel.distractorMask() == firstMask && panel.distractorHue(0) == firstHue);
        CHECK(calls == firstCalls); // No random calls or repaint inside Showing.
        CHECK(panel.update(now + 1000U, 0));
        checkSearchHidden(panel);
        CHECK(panel.update(now + 1200U, 0));
        CHECK(panel.whiteTarget() && panel.litChannel() == 7);
        if (panel.isSearch()) {
            checkDistractors(panel, 7, minimumDistractors(mode) + 1U);
            CHECK(panel.distractorHue(0) != firstHue);
            CHECK(panel.distractorMask() != firstMask);
            CHECK(calls == firstCalls + minimumDistractors(mode) + 3U);
        } else CHECK(calls == firstCalls);
        CHECK(panel.update(now + 1700U, 0));
        CHECK(panel.update(now + 1900U, 0));
        now += 1900U;
        CHECK(panel.litChannel() == game::PanelNoChannel);
        checkSearchHidden(panel);
        now = press(panel, now, 1U << 2);
        CHECK(panel.phase() == game::PanelPhase::Hit && panel.inputIndex() == 1);
        CHECK(panel.takeFeedback() == game::PanelFeedback::Win);
        CHECK(!panel.update(now + 539U, 1U << 2));
        CHECK(panel.update(now += 540U, 1U << 2));
        CHECK(panel.phase() == game::PanelPhase::Input && panel.litChannel() == game::PanelNoChannel);
        CHECK(!panel.update(now + 40U, 1U << 2)); // Intermediate feedback also gates holds.
        now = release(panel, now + 40U);
        now = press(panel, now, 1U << 7);
        CHECK(panel.phase() == game::PanelPhase::RoundWin && panel.inputIndex() == 2);
        CHECK(panel.takeFeedback() == game::PanelFeedback::Win);
        checkSearchHidden(panel);
    }
}

void testSearchWrongChordDeadlineAndRollover() {
    const uint32_t boots[] = {0, UINT32_MAX - 2000U};
    for (game::PanelMode mode : SearchModes) {
        for (uint32_t boot : boots) {
            for (uint8_t scenario = 0; scenario < 6; ++scenario) {
                game::PanelGames panel;
                uint32_t now = searchPlayback(panel, startSearch(panel, mode, boot));
                uint32_t mask = scenario == 0 ? 2U : (scenario == 1 ? 3U : 0U);
                if (scenario < 2) now = press(panel, now, mask);
                else {
                    // Answer just before, exactly on, or after the deadline; or silence.
                    CHECK(panel.update(now + firstWarningAt(mode), 0));
                    CHECK(panel.takeFeedback() == game::PanelFeedback::Warning);
                    CHECK(panel.update(now + secondWarningAt(mode), 0));
                    CHECK(panel.takeFeedback() == game::PanelFeedback::Warning);
                    const uint32_t deadline = now + panel.inputTimeoutMs();
                    if (scenario == 2) {
                        CHECK(!panel.update(deadline - 1U, 0));
                        now = deadline;
                        CHECK(panel.update(now, 0));
                    } else {
                        mask = 1;
                        const uint32_t answerAt = deadline + (scenario == 3 ? UINT32_MAX :
                            (scenario == 4 ? 0U : 1U));
                        CHECK(!panel.update(answerAt - 30U, mask));
                        CHECK(!panel.update(deadline - (scenario == 3 ? 2U : 1U), mask));
                        now = answerAt;
                        CHECK(panel.update(now, mask));
                    }
                }
                const bool won = scenario == 3;
                CHECK(panel.phase() == (won ? game::PanelPhase::RoundWin : game::PanelPhase::Loss));
                CHECK(panel.hue() == (won ? 96 : 0));
                CHECK(panel.takeFeedback() == (won ? game::PanelFeedback::Win : game::PanelFeedback::Loss));
                CHECK(panel.takeFeedback() == game::PanelFeedback::None);
                checkSearchHidden(panel);
                if (won) continue;
                CHECK(!panel.update(now + 739U, mask));
                CHECK(panel.update(now + 740U, mask));
                CHECK(panel.phase() == game::PanelPhase::Ready && panel.mode() == mode);
                CHECK(panel.sequenceLength() == 0 && panel.litChannel() == game::PanelNoChannel);
                checkSearchHidden(panel);
                CHECK(!panel.update(now + 1000U, mask));
                now = release(panel, now + 1000U);
                press(panel, now, 1);
                CHECK(panel.phase() == game::PanelPhase::Gap && panel.sequenceLength() == 1);
            }
        }
    }
}

void testSearchHeldPlaybackAndFreshStart() {
    for (game::PanelMode mode : MemoryModes) {
        for (bool partial : {false, true}) {
            game::PanelGames panel;
            panel.select(mode, 0, 1);
            CHECK(!panel.update(100, 1)); // Selection-held cannot start.
            uint32_t now = release(panel, 100);
            CHECK(!panel.update(now + 1U, 1UL << 31));
            CHECK(!panel.update(now + 31U, 1UL << 31));
            now = press(panel, now + 31U, 1);
            if (!partial) now = searchPlayback(panel, now, 1);
            else {
                CHECK(panel.update(now + 500U, 0));
                CHECK(panel.update(now + 1000U, 0));
                CHECK(!panel.update(now + 1190U, 1));
                CHECK(panel.update(now + 1200U, 1));
                now += 1200U;
                panel.takeFeedback();
            }
            CHECK(!panel.update(now + 100U, 1));
            CHECK(panel.phase() == game::PanelPhase::Input && panel.inputIndex() == 0);
            now = release(panel, now + 100U);
            now = press(panel, now, 1);
            CHECK(panel.phase() == game::PanelPhase::RoundWin);
            CHECK(panel.takeFeedback() == game::PanelFeedback::Win);
            panel.select(mode, now, 1);
            CHECK(panel.phase() == game::PanelPhase::Ready && panel.sequenceLength() == 0);
            checkSearchHidden(panel);
            CHECK(panel.takeFeedback() == game::PanelFeedback::None);
            CHECK(!panel.update(now + 100U, 1));
            now = release(panel, now + 100U);
            press(panel, now, 1);
            CHECK(panel.phase() == game::PanelPhase::Gap);
        }
    }
}

void testSearchReselectAndCancelEveryPhase() {
    const game::PanelPhase phases[] = {game::PanelPhase::Ready, game::PanelPhase::Gap,
        game::PanelPhase::Showing, game::PanelPhase::Input,
        game::PanelPhase::Hit, game::PanelPhase::RoundWin, game::PanelPhase::Loss};
    for (game::PanelMode mode : MemoryModes) {
        for (game::PanelPhase phase : phases) {
                for (game::PanelMode next : {mode, game::PanelMode::Disabled, game::PanelMode::Simon,
                    game::PanelMode::Sorrend3, game::PanelMode::Sorrend4, game::PanelMode::Sorrend5,
                    game::PanelMode::Random, game::PanelMode::Random2, game::PanelMode::Flasher}) {
                game::PanelGames panel;
                uint32_t now = 0;
                panel.select(mode, now, 0);
                if (phase != game::PanelPhase::Ready) {
                    now = press(panel, now, 1);
                    if (phase == game::PanelPhase::Showing) {
                        now += 500U;
                        CHECK(panel.update(now, 0));
                    } else if (phase != game::PanelPhase::Gap) {
                        now = searchPlayback(panel, now);
                        if (phase == game::PanelPhase::Hit) {
                            now = press(panel, now, 1);
                            // Consume the completed round's event before replay;
                            // leave the later Hit event pending for cancellation.
                            CHECK(panel.takeFeedback() == game::PanelFeedback::Win);
                            CHECK(panel.update(now += 540U, 0));
                            now = searchPlayback(panel, now);
                            now = press(panel, now, 1);
                        } else if (phase != game::PanelPhase::Input)
                            now = press(panel, now, phase == game::PanelPhase::RoundWin ? 1 : 2);
                        else if (panel.isSearch())
                            CHECK(panel.update(now += firstWarningAt(mode), 0)); // Pending warning must clear too.
                    }
                }
                CHECK(panel.phase() == phase);
                panel.select(next, now, 1);
                CHECK(panel.mode() == next && panel.sequenceLength() == 0 && panel.inputIndex() == 0);
                CHECK(panel.phase() == (next == game::PanelMode::Disabled ?
                    game::PanelPhase::Disabled : game::PanelPhase::Ready));
                CHECK(panel.litChannel() == game::PanelNoChannel && panel.hue() == 0);
                checkSearchHidden(panel);
                CHECK(panel.takeFeedback() == game::PanelFeedback::None);
                CHECK(!panel.update(now + 10000U, 1));
            }
        }
    }
}

void testSearchWarningBoundariesPriorityAndLateUpdates() {
    for (game::PanelMode mode : SearchModes) {
        const uint32_t first = firstWarningAt(mode), second = secondWarningAt(mode);
        const uint32_t timeout = profileTimeout(mode);
        // Second boot places Input just before wrap, so warnings/deadline cross it.
        for (uint32_t boot : {0U, UINT32_MAX - 1300U}) {
            game::PanelGames exact;
            const uint32_t entered = searchPlayback(exact, startSearch(exact, mode, boot));
            CHECK(!exact.update(entered + first - 1U, 0));
            CHECK(exact.takeFeedback() == game::PanelFeedback::None);
            CHECK(exact.update(entered + first, 0));
            CHECK(exact.takeFeedback() == game::PanelFeedback::Warning);
            CHECK(exact.takeFeedback() == game::PanelFeedback::None);
            CHECK(!exact.update(entered + first, 0));
            CHECK(!exact.update(entered + second - 1U, 0));
            CHECK(exact.update(entered + second, 0));
            CHECK(exact.takeFeedback() == game::PanelFeedback::Warning);
            CHECK(!exact.update(entered + second, 0));
            CHECK(!exact.update(entered + timeout - 1U, 0));
            CHECK(exact.update(entered + timeout, 0));
            CHECK(exact.takeFeedback() == game::PanelFeedback::Loss);
            for (uint32_t delay : {second, timeout - 1U, timeout, timeout + 10000U}) {
                game::PanelGames panel;
                const uint32_t now = searchPlayback(panel, startSearch(panel, mode, boot));
                CHECK(panel.update(now + delay, 0));
                CHECK(panel.takeFeedback() == (delay < timeout ?
                    game::PanelFeedback::Warning : game::PanelFeedback::Loss));
                CHECK(panel.takeFeedback() == game::PanelFeedback::None);
                CHECK(!panel.update(now + delay, 0)); // Coalesced, never catch-up beeps.
                checkSearchHidden(panel);
                if (delay < timeout) {
                    CHECK(panel.litChannel() == game::PanelNoChannel);
                    CHECK(!panel.update(now + timeout - 1U, 0));
                    CHECK(panel.update(now + timeout, 0));
                    CHECK(panel.takeFeedback() == game::PanelFeedback::Loss);
                }
            }
            for (uint32_t mask : {1U, 2U, 3U}) {
                for (uint32_t elapsed : {first, second, timeout}) {
                    game::PanelGames panel;
                    const uint32_t now = searchPlayback(panel, startSearch(panel, mode, boot));
                    if (elapsed > first) {
                        CHECK(panel.update(now + first, 0));
                        CHECK(panel.takeFeedback() == game::PanelFeedback::Warning);
                    }
                    if (elapsed > second) {
                        CHECK(panel.update(now + second, 0));
                        CHECK(panel.takeFeedback() == game::PanelFeedback::Warning);
                    }
                    CHECK(!panel.update(now + elapsed - 30U, mask));
                    CHECK(panel.update(now + elapsed, mask)); // Answer beats warning; deadline beats both.
                    CHECK(panel.takeFeedback() == (mask == 1 && elapsed < timeout ?
                        game::PanelFeedback::Win : game::PanelFeedback::Loss));
                    CHECK(panel.takeFeedback() == game::PanelFeedback::None);
                    checkSearchHidden(panel);
                }
            }
        }
        game::PanelGames panel;
        uint32_t now = startSearch(panel, mode);
        CHECK(panel.takeFeedback() == game::PanelFeedback::None);
        CHECK(panel.update(now += 100000U, 0));
        CHECK(panel.phase() == game::PanelPhase::Showing);
        CHECK(panel.takeFeedback() == game::PanelFeedback::None);
        CHECK(!panel.update(now, 0));
        CHECK(panel.update(now += 100000U, 0));
        CHECK(panel.phase() == game::PanelPhase::Gap);
        CHECK(panel.takeFeedback() == game::PanelFeedback::None);
        CHECK(panel.update(now += 100000U, 0));
        CHECK(panel.phase() == game::PanelPhase::Input);
        CHECK(panel.takeFeedback() == game::PanelFeedback::None); // No warning at entry.
        CHECK(!panel.update(now, 0));
        CHECK(!panel.update(now + first - 1U, 0));
        CHECK(panel.update(now + first, 0)); // Deadline starts at actual Input entry.
        CHECK(panel.takeFeedback() == game::PanelFeedback::Warning);
    }
}

struct Test { const char* name; void (*run)(); };
const Test Tests[] = {
    {"Sorrend3/4/5 all targets and profile-specific unique distractors", testSearchEveryTargetAndDistractorCount},
    {"Sorrend3/4/5 bounded constant, faulty and null RNG", testSearchConstantFaultyAndNullRandom},
    {"Sorrend2..5 growing 1..32 memory and per-answer feedback/deadlines", testMemoryGrowingCapacityAndPerAnswerTimers},
    {"Sorrend2..5 preserved prefix, per-step distractions and intermediate hold gating", testMemoryPrefixAndPerStepDistractions},
    {"Sorrend3/4/5 wrong, chord, exact deadlines and rollover", testSearchWrongChordDeadlineAndRollover},
    {"Sorrend2..5 fresh start and stable/partial playback holds", testSearchHeldPlaybackAndFreshStart},
    {"Sorrend2..5 reselection and cancellation in every phase", testSearchReselectAndCancelEveryPhase},
    {"Sorrend3/4/5 warning boundaries, priority, take-once and late updates", testSearchWarningBoundariesPriorityAndLateUpdates},
    {"Random2 all inputs, chords, duration and hold", testRandom2AllInputsAndChords},
    {"Random2 disco beats, retrigger, delayed updates and rollover", testRandom2BeatsRetriggerAndRollover},
    {"Random2 bounce, fresh selection and cancellation", testRandom2BounceReselectAndCancel},
    {"Random2 corrected strip, metal and Ikarus pair mapping", testRandom2WorkbookPairs},
    {"Random2 six GPIO lamps, chords and scoped restoration", testPairedGpioLampsAndRestoration},
    {"Random2 GPIO ownership, reserved pins and bounds", testPairedGpioPreservesOtherOwnersAndBounds},
    {"Flasher single visible target without growing memory", testFlasherSingleTargetWithoutMemory},
    {"Flasher wrong button, chord, timeout and rollover", testFlasherLossTimeoutAndRollover},
    {"Flasher held playback, reselection and cancellation", testFlasherHeldInputsReselectionAndCancel},
    {"panel workbook inputs and mirrored lamp pixels", testWorkbookPanelMapping},
    {"panel Random all 32 inputs and random bounds", testRandomAllInputsAndBounds},
    {"panel Random bounce, hold, retrigger and rollover", testRandomBounceRetriggerAndRollover},
    {"panel Simon start consumption and held playback", testSimonStartAndPlaybackHeldIsolation},
    {"panel Simon partially debounced playback press", testSimonPartialPlaybackPressDiscarded},
    {"panel Simon wrong/multiple press and loss rearm", testSimonWrongMultipleAndRearm},
    {"panel Simon unlimited input and rollover", testSimonUnlimitedInputAndRollover},
    {"panel Simon all 32 rounds and per-answer feedback", testSimonFullCapacityAndHitTiming},
    {"panel Simon all channels, preserved prefix and unlimited input", testSimonEveryChannelAndSequencePrefix},
    {"panel bounded late-update transitions", testSimonBoundedLateUpdates},
    {"panel selection cancellation and fallback", testSelectionCancelsEveryPhase}
};
} // namespace

size_t runPanelGamesTests() {
    for (const Test& test : Tests) {
        calls = chosen = 0;
        test.run();
        printf("PASS: %s\n", test.name);
    }
    return sizeof(Tests) / sizeof(Tests[0]);
}