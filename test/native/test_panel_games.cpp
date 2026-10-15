#include "test_panel_games.h"
#include "game/PanelGames.h"
#include "config/PanelConfig.h"
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

void testSimonTimeoutAndRollover() {
    game::PanelGames panel(randomBelow);
    chosen = 0;
    const uint32_t now = playback(panel, start(panel, UINT32_MAX - 1000U));
    CHECK(!panel.update(now + 9999U, 0));
    CHECK(panel.update(now + 10000U, 0));
    CHECK(panel.phase() == game::PanelPhase::Loss);
    CHECK(panel.litChannel() == 0);
    CHECK(panel.takeFeedback() == game::PanelFeedback::Loss);
    CHECK(panel.update(now + 10740U, 0));
    CHECK(panel.phase() == game::PanelPhase::Ready);
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
    // The timeout starts anew after feedback, not at the round's first input.
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

struct Test { const char* name; void (*run)(); };
const Test Tests[] = {
    {"Flasher single visible target without growing memory", testFlasherSingleTargetWithoutMemory},
    {"Flasher wrong button, chord, timeout and rollover", testFlasherLossTimeoutAndRollover},
    {"Flasher held playback, reselection and cancellation", testFlasherHeldInputsReselectionAndCancel},
    {"panel workbook inputs and mirrored lamp pixels", testWorkbookPanelMapping},
    {"panel Random all 32 inputs and random bounds", testRandomAllInputsAndBounds},
    {"panel Random bounce, hold, retrigger and rollover", testRandomBounceRetriggerAndRollover},
    {"panel Simon start consumption and held playback", testSimonStartAndPlaybackHeldIsolation},
    {"panel Simon partially debounced playback press", testSimonPartialPlaybackPressDiscarded},
    {"panel Simon wrong/multiple press and loss rearm", testSimonWrongMultipleAndRearm},
    {"panel Simon timeout and rollover", testSimonTimeoutAndRollover},
    {"panel Simon all 32 rounds and per-answer feedback", testSimonFullCapacityAndHitTiming},
    {"panel Simon all channels, preserved prefix and renewed timeout", testSimonEveryChannelAndSequencePrefix},
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