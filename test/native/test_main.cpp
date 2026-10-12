#include "game/CycloneGame.h"
#include "game/DebouncedButton.h"
#include "music/Melody.h"
#include "music/MusicPlayer.h"
#include "app/ModeController.h"
#include "ui/LcdText.h"
#include "test_audio.h"
#include "test_debug.h"
#include "test_input.h"
#include "test_led_strip.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

namespace {

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition); \
        exit(EXIT_FAILURE); \
    } \
} while (false)

uint8_t chosenTarget = 20;
unsigned int tones = 0;
unsigned int stops = 0;
uint16_t lastFrequency = 0;
uint32_t lastToneDuration = 0;

uint8_t pickTarget() { return chosenTarget; }
void playTone(uint16_t frequency, uint32_t duration) {
    ++tones;
    lastFrequency = frequency;
    lastToneDuration = duration;
}
void stopTone() { ++stops; }

uint32_t startPlaying(game::CycloneGame& game, uint32_t now = 0, bool held = false) {
    game.setEnabled(true, now, false);
    CHECK(game.state() == game::State::Idle);
    CHECK(!game.update(now + 1U, true));
    CHECK(game.update(now + 31U, true));
    CHECK(game.state() == game::State::Intro);
    const uint32_t introStart = now + 31U;
    if (!held) {
        game.update(introStart + 1U, false);
        game.update(introStart + 31U, false);
    }
    now = introStart + game::IntroDurationMs;
    CHECK(game.update(now, held));
    CHECK(game.state() == game::State::Playing);
    CHECK(game.position() == 0);
    return now;
}

uint32_t moveToTarget(game::CycloneGame& game, uint32_t now) {
    for (unsigned int steps = 0; game.position() != game.target(); ++steps) {
        CHECK(steps < game::GameLedCount);
        now += game::CycloneGame::movementInterval(game.level());
        CHECK(game.update(now, false));
    }
    return now;
}

uint32_t hitTarget(game::CycloneGame& game, uint32_t now) {
    now = moveToTarget(game, now);
    CHECK(!game.update(now + 1U, true));
    now += 31U;
    CHECK(game.update(now, true));
    CHECK(game.state() == game::State::WinFeedback);
    return now;
}

uint32_t finishFeedback(game::CycloneGame& game, uint32_t now) {
    game.update(now + 1U, false);
    game.update(now + 31U, false);
    now += game::FeedbackDurationMs;
    CHECK(game.update(now, false));
    return now;
}

void testDebounceAndHold() {
    game::DebouncedButton button;
    button.reset(0, false);
    CHECK(!button.update(10, true));
    CHECK(!button.update(20, false));
    CHECK(!button.update(25, true));
    CHECK(!button.update(54, true));
    CHECK(button.update(55, true));
    CHECK(!button.update(56, true));
    CHECK(!button.update(200, true));
    CHECK(!button.update(201, false));
    CHECK(!button.update(231, false));
    CHECK(!button.update(232, true));
    CHECK(button.update(262, true));
}

void testDebounceRollover() {
    game::DebouncedButton button;
    const uint32_t now = UINT32_MAX - 10U;
    button.reset(now, false);
    CHECK(!button.update(now, true));
    CHECK(!button.update(now + 29U, true));
    CHECK(button.update(now + 30U, true));
    CHECK(!button.update(now + 100U, true));
}

void testBootHeldButton() {
    game::CycloneGame game(pickTarget);
    game.setEnabled(true, 0, true);
    CHECK(!game.update(100, true));
    CHECK(game.state() == game::State::Idle);
    game.update(101, false);
    game.update(131, false);
    CHECK(!game.update(132, true));
    CHECK(game.update(162, true));
    CHECK(game.state() == game::State::Intro);
}

void testStartIsSingleTransition() {
    game::CycloneGame game(pickTarget);
    game.setEnabled(true, 0, false);
    CHECK(!game.update(1, true));
    CHECK(game.update(31, true));
    CHECK(game.state() == game::State::Intro);
    CHECK(!game.update(32, true));
    CHECK(game.state() == game::State::Intro);
}

void testHeldStartDoesNotLose() {
    game::CycloneGame game(pickTarget);
    const uint32_t now = startPlaying(game, 0, true);
    CHECK(!game.update(now + 1U, true));
    CHECK(game.update(now + 50U, true));
    CHECK(game.state() == game::State::Playing);
    CHECK(game.position() == 1);
}

void testIntroWipeTiming() {
    game::CycloneGame game(pickTarget);
    game.setEnabled(true, 0, false);
    game.update(1, true);
    game.update(31, true);
    const uint32_t start = 31;
    CHECK(!game.update(start + game::IntroPauseMs, true));
    CHECK(game.clearedGameLeds() == 0);
    CHECK(game.update(start + game::IntroPauseMs + game::IntroGameStepMs, true));
    CHECK(game.clearedGameLeds() == 1);
    CHECK(game.update(start + 3300U, true));
    CHECK(game.clearedGameLeds() == game::GameLedCount);
    CHECK(game.clearedScoreLeds() == 0);
    CHECK(game.update(start + 3400U, true));
    CHECK(game.clearedScoreLeds() == 1);
    CHECK(game.update(start + game::IntroDurationMs - 1U, true));
    CHECK(game.clearedScoreLeds() == 4);
    CHECK(game.state() == game::State::Intro);
    CHECK(game.update(start + game::IntroDurationMs, true));
    CHECK(game.state() == game::State::Playing);
}

void testMovementAndWrap() {
    game::CycloneGame game(pickTarget);
    uint32_t now = startPlaying(game);
    CHECK(!game.update(now + 49U, false));
    for (unsigned int step = 1; step <= game::GameLedCount * 3U; ++step) {
        now += 50U;
        CHECK(game.update(now, false));
        CHECK(game.position() == step % game::GameLedCount);
        CHECK(game.position() < game::GameLedCount);
    }
}

void testEveryWinBoundary() {
    for (uint8_t level = 1; level <= game::LevelCount; ++level) {
        const int radius = level == 1 ? 2 : (level == 2 ? 1 : 0);
        for (uint8_t target = game::MinimumTarget; target <= game::MaximumTarget; ++target) {
            for (uint8_t position = 0; position < game::GameLedCount; ++position) {
                const int difference = static_cast<int>(position) - target;
                const bool expected = difference >= -radius && difference <= radius;
                CHECK(game::CycloneGame::isHit(level, position, target) == expected);
            }
        }
    }
    CHECK(!game::CycloneGame::isHit(0, 20, 20));
    CHECK(!game::CycloneGame::isHit(5, 20, 20));
    CHECK(!game::CycloneGame::isHit(1, 70, 20));
    CHECK(!game::CycloneGame::isHit(1, 255, 20));
    CHECK(!game::CycloneGame::isHit(1, 0, 0));
    CHECK(!game::CycloneGame::isHit(1, 69, 255));
}

void testLevelTwoLowerBound() {
    CHECK(!game::CycloneGame::isHit(2, 0, 20));
    CHECK(!game::CycloneGame::isHit(2, 18, 20));
    CHECK(game::CycloneGame::isHit(2, 19, 20));
    CHECK(game::CycloneGame::isHit(2, 20, 20));
    CHECK(game::CycloneGame::isHit(2, 21, 20));
    CHECK(!game::CycloneGame::isHit(2, 22, 20));
}

void testAllLevelsAndScore() {
    game::CycloneGame game(pickTarget);
    uint32_t now = startPlaying(game);
    for (uint8_t level = 1; level <= game::LevelCount; ++level) {
        CHECK(game.level() == level);
        CHECK(game.scoreLeds() == level);
        const uint32_t interval = game::CycloneGame::movementInterval(level);
        CHECK(!game.update(now + interval - 1U, false));
        now += interval;
        CHECK(game.update(now, false));
        CHECK(game.position() == 1);
        now = hitTarget(game, now);
        CHECK(game.scoreLeds() == (level == 4 ? 5 : level));
        now = finishFeedback(game, now);
        CHECK(game.state() == (level == 4 ? game::State::Idle : game::State::Playing));
    }
    CHECK(game.scoreLeds() == 0);
    CHECK(game::CycloneGame::movementInterval(1) == 50);
    CHECK(game::CycloneGame::movementInterval(2) == 40);
    CHECK(game::CycloneGame::movementInterval(3) == 35);
    CHECK(game::CycloneGame::movementInterval(4) == 20);
}

void testFeedbackFlashTiming() {
    game::CycloneGame game(pickTarget);
    uint32_t now = hitTarget(game, startPlaying(game));
    CHECK(game.feedbackOn());
    CHECK(!game.update(now + 499U, true));
    for (uint8_t phase = 1; phase < game::FeedbackPhaseCount; ++phase) {
        CHECK(game.update(now + phase * game::FeedbackPhaseMs, true));
        CHECK(game.feedbackOn() == (phase % 2 == 0));
        CHECK(game.state() == game::State::WinFeedback);
    }
    CHECK(!game.update(now + game::FeedbackDurationMs - 1U, true));
    CHECK(game.update(now + game::FeedbackDurationMs, true));
    CHECK(game.state() == game::State::Playing);
    CHECK(game.level() == 2);
    CHECK(!game.update(now + game::FeedbackDurationMs + 1U, true));
    CHECK(game.state() == game::State::Playing);
}

void testLossAndHeldReset() {
    game::CycloneGame game(pickTarget);
    uint32_t now = startPlaying(game);
    game.update(now + 1U, true);
    now += 31U;
    CHECK(game.update(now, true));
    CHECK(game.state() == game::State::LossFeedback);
    CHECK(game.update(now + game::FeedbackDurationMs, true));
    CHECK(game.state() == game::State::Idle);
    CHECK(!game.update(now + game::FeedbackDurationMs + 100U, true));
    CHECK(game.state() == game::State::Idle);
}

void testIntroRollover() {
    game::CycloneGame game(pickTarget);
    const uint32_t now = startPlaying(game, UINT32_MAX - 1000U);
    CHECK(now < 4000U);
    CHECK(game.level() == 1);
}

void testMovementRollover() {
    game::CycloneGame game(pickTarget);
    const uint32_t base = UINT32_MAX - game::IntroDurationMs - 31U - 10U;
    const uint32_t now = startPlaying(game, base);
    CHECK(now == UINT32_MAX - 10U);
    CHECK(!game.update(now + 49U, false));
    CHECK(game.update(now + 50U, false));
    CHECK(game.position() == 1);
}

void testFeedbackRollover() {
    game::CycloneGame game(pickTarget);
    const uint32_t base = UINT32_MAX - game::IntroDurationMs - 31U - 1000U - 100U;
    const uint32_t now = hitTarget(game, startPlaying(game, base));
    CHECK(game.update(now + game::FeedbackPhaseMs, true));
    CHECK(!game.feedbackOn());
    CHECK(game.update(now + game::FeedbackDurationMs, true));
    CHECK(game.level() == 2);
}

void testTargetClamping() {
    chosenTarget = 0;
    game::CycloneGame low(pickTarget);
    startPlaying(low);
    CHECK(low.target() == game::MinimumTarget);
    chosenTarget = 255;
    game::CycloneGame high(pickTarget);
    startPlaying(high);
    CHECK(high.target() == game::MaximumTarget);
}

void testRestartAndDisable() {
    game::CycloneGame game(pickTarget);
    uint32_t now = startPlaying(game);
    CHECK(game.update(now + 50U, false));
    now += 50U;
    game.restart(now, true);
    CHECK(game.state() == game::State::Idle);
    CHECK(game.position() == 0);
    CHECK(game.level() == 0);
    CHECK(!game.update(now + 100U, true));
    game.setEnabled(false, now + 101U, true);
    CHECK(game.state() == game::State::Disabled);
    CHECK(!game.update(now + 500U, false));
}

void testNoteDurations() {
    CHECK(music::noteDurationMs(8) == 428);
    CHECK(music::noteDurationMs(-8) == 642);
    CHECK(music::noteDurationMs(16) == 214);
    CHECK(music::noteDurationMs(-2) == 2571);
    CHECK(music::noteDurationMs(0) == 0);
    CHECK(music::noteDurationMs(4, 0) == 0);
}

void testMelodyBoundsAndRepetition() {
    const uint16_t count = music::melodyNoteCount();
    CHECK(count == 136);
    for (uint16_t i = 0; i < count / 2; ++i) {
        const music::Note first = music::melodyNote(i);
        const music::Note second = music::melodyNote(i + count / 2);
        CHECK(first.frequency == second.frequency);
        CHECK(first.divider == second.divider);
        CHECK(music::noteDurationMs(first.divider) != 0);
    }
    CHECK(music::melodyNote(count).frequency == 0);
    CHECK(music::melodyNote(UINT16_MAX).divider == 0);
}

void testMusicAdvanceAndStop() {
    music::MusicPlayer player(playTone, stopTone);
    player.setEnabled(true, 0);
    CHECK(tones == 1);
    CHECK(lastFrequency == 392);
    CHECK(lastToneDuration == 428U * 9U / 10U);
    player.update(427);
    CHECK(tones == 1);
    player.update(428);
    CHECK(tones == 2);
    CHECK(player.currentNote() == 1);
    player.setEnabled(false, 429);
    CHECK(!player.isPlaying());
    const unsigned int stopped = stops;
    player.setEnabled(false, 430);
    player.update(10000);
    CHECK(stops == stopped);
    CHECK(tones == 2);
}

void testMusicRollover() {
    music::MusicPlayer player(playTone, stopTone);
    const uint32_t now = UINT32_MAX - 10U;
    player.setEnabled(true, now);
    player.update(now + 427U);
    CHECK(player.currentNote() == 0);
    player.update(now + 428U);
    CHECK(player.currentNote() == 1);
}

void testMusicFinishesAndRestarts() {
    music::MusicPlayer player(playTone, stopTone);
    uint32_t now = 0;
    player.setEnabled(true, now);
    for (uint16_t i = 0; i < music::melodyNoteCount(); ++i) {
        CHECK(player.isPlaying());
        now += player.currentNoteDuration();
        player.update(now);
    }
    CHECK(!player.isPlaying());
    CHECK(tones == music::melodyNoteCount());
    player.update(now + 100000U);
    CHECK(tones == music::melodyNoteCount());
    player.restart(now);
    CHECK(player.isPlaying());
    CHECK(player.currentNote() == 0);
}

void testModeSelectionIsIdempotent() {
    game::CycloneGame game(pickTarget);
    audio::SoundEffects effects(playTone, stopTone);
    app::ModeController modes(game, effects);
    CHECK(modes.select(app::Mode::Cyclone, 0, false));
    CHECK(!modes.select(app::Mode::Cyclone, 1, false));
    CHECK(game.state() == game::State::Idle);
    CHECK(modes.select(app::Mode::Debug, 2, false));
    CHECK(game.state() == game::State::Disabled);
    for (uint32_t i = 3; i < 1000; ++i)
        CHECK(!modes.select(app::Mode::Debug, i, false));
    CHECK(tones == 0);
    CHECK(stops == 0);
    CHECK(modes.select(app::Mode::Cyclone, 1000, false));
    CHECK(stops == 0);
    CHECK(game.state() == game::State::Idle);
}

void testRepeatedModeChanges() {
    game::CycloneGame game(pickTarget);
    audio::SoundEffects effects(playTone, stopTone);
    app::ModeController modes(game, effects);
    modes.select(app::Mode::Cyclone, 0, false);
    for (uint32_t i = 0; i < 500; ++i) {
        CHECK(modes.select(app::Mode::Debug, i * 2U + 1U, false));
        CHECK(game.state() == game::State::Disabled);
        CHECK(modes.select(app::Mode::Cyclone, i * 2U + 2U, false));
        CHECK(game.state() == game::State::Idle);
        CHECK(!effects.isPlaying());
    }
    CHECK(tones == 0);
    CHECK(stops == 0);
    CHECK(!modes.select(static_cast<app::Mode>(99), 2000, false));
    CHECK(game.state() == game::State::Idle);
}

void testSongCompletionDoesNotResetGame() {
    game::CycloneGame game(pickTarget);
    startPlaying(game);
    music::MusicPlayer player(playTone, stopTone);
    uint32_t now = 0;
    player.setEnabled(true, now);
    while (player.isPlaying()) {
        now += player.currentNoteDuration();
        player.update(now);
    }
    CHECK(game.state() == game::State::Playing);
    CHECK(game.level() == 1);
}

void testLongLcdValueIsClipped() {
    char storage[19];
    memset(storage, '#', sizeof(storage));
    char* row = storage + 1;
    const size_t start = ui::composeNameValue(row, 17, 16, '>', "DigIn", "12345678901234567890");
    CHECK(start == 1);
    CHECK(row[0] == '>');
    CHECK(strlen(row) == 16);
    CHECK(strncmp(row + 1, "123456789012345", 15) == 0);
    CHECK(storage[0] == '#');
    CHECK(storage[18] == '#');
}

void testLcdNameValueAlignment() {
    char row[17];
    CHECK(ui::composeNameValue(row, sizeof(row), 16, '>', "DigIn", "123") == 13);
    CHECK(strcmp(row, ">DigIn       123") == 0);
    ui::blankRow(row, sizeof(row), 16, '>');
    CHECK(ui::writeAligned(row, sizeof(row), 1, 15, "ABC", ui::Alignment::Right) == 13);
    CHECK(strcmp(row + 13, "ABC") == 0);
    ui::blankRow(row, sizeof(row), 16, '>');
    CHECK(ui::writeAligned(row, sizeof(row), 1, 15, "ABC", ui::Alignment::Centre) == 7);
    CHECK(strncmp(row + 7, "ABC", 3) == 0);
}

void testLcdSmallAndEmptyBuffers() {
    char storage[3] = {'X', 'Y', 'Z'};
    CHECK(ui::composeNameValue(storage + 1, 0, 16, '>', "Name", "Value") == 0);
    CHECK(storage[1] == 'Y');
    CHECK(ui::composeNameValue(storage + 1, 1, 16, '>', "Name", "Value") == 0);
    CHECK(storage[1] == '\0');
    CHECK(storage[0] == 'X');
    CHECK(storage[2] == 'Z');
    CHECK(ui::composeNameValue(nullptr, 0, 0, '>', nullptr, nullptr) == 0);
    char row[17];
    CHECK(ui::composeNameValue(row, sizeof(row), SIZE_MAX, '>', nullptr, nullptr) == 16);
    CHECK(strlen(row) == 16);
    CHECK(ui::copyClipped(row, sizeof(row), SIZE_MAX, "x", 1) == 0);
}

void testLcdGuardBytesAcrossSizes() {
    char name[41];
    char value[41];
    memset(name, 'N', sizeof(name) - 1);
    name[sizeof(name) - 1] = '\0';
    for (size_t capacity = 1; capacity <= 31; ++capacity) {
        for (size_t width = 0; width <= 40; ++width) {
            for (size_t length = 0; length <= 40; ++length) {
                memset(value, 'V', length);
                value[length] = '\0';
                char storage[33];
                memset(storage, '#', sizeof(storage));
                char* row = storage + 1;
                ui::composeNameValue(row, capacity, width, '>', name, value);
                const size_t columns = width < capacity ? width : capacity - 1;
                CHECK(storage[0] == '#');
                CHECK(storage[capacity + 1] == '#');
                CHECK(row[columns] == '\0');
                CHECK(strlen(row) == columns);
                if (columns > 0) CHECK(row[0] == '>');
                ui::writeAligned(row, capacity, columns == 0 ? 0 : 1,
                    columns == 0 ? 0 : columns - 1, value, ui::Alignment::Right);
                CHECK(storage[0] == '#');
                CHECK(storage[capacity + 1] == '#');
                CHECK(row[columns] == '\0');
            }
        }
    }
}

struct Test { const char* name; void (*run)(); };

const Test Tests[] = {
    {"button bounce and hold", testDebounceAndHold},
    {"button millis rollover", testDebounceRollover},
    {"button held at boot", testBootHeldButton},
    {"start is one state transition", testStartIsSingleTransition},
    {"held start cannot cause a loss", testHeldStartDoesNotLose},
    {"non-blocking intro wipe", testIntroWipeTiming},
    {"movement and zero-position wrap", testMovementAndWrap},
    {"every win boundary at every target", testEveryWinBoundary},
    {"level two lower bound", testLevelTwoLowerBound},
    {"four levels and five score pixels", testAllLevelsAndScore},
    {"three timed feedback flashes", testFeedbackFlashTiming},
    {"loss and held-button reset", testLossAndHeldReset},
    {"intro timer rollover", testIntroRollover},
    {"movement timer rollover", testMovementRollover},
    {"feedback timer rollover", testFeedbackRollover},
    {"random target bounds", testTargetClamping},
    {"restart and disable", testRestartAndDisable},
    {"integer note durations", testNoteDurations},
    {"melody bounds and repetition", testMelodyBoundsAndRepetition},
    {"non-blocking music and cancellation", testMusicAdvanceAndStop},
    {"music timer rollover", testMusicRollover},
    {"song completion and restart", testMusicFinishesAndRestarts},
    {"idempotent menu selection", testModeSelectionIsIdempotent},
    {"500 repeated mode changes", testRepeatedModeChanges},
    {"song completion leaves game untouched", testSongCompletionDoesNotResetGame},
    {"long LCD value clipping", testLongLcdValueIsClipped},
    {"LCD name/value alignment", testLcdNameValueAlignment},
    {"empty and tiny LCD buffers", testLcdSmallAndEmptyBuffers},
    {"LCD guard bytes across sizes", testLcdGuardBytesAcrossSizes}
};

} // namespace

int main() {
    for (const Test& test : Tests) {
        chosenTarget = 20;
        tones = stops = 0;
        lastFrequency = 0;
        lastToneDuration = 0;
        test.run();
        printf("PASS: %s\n", test.name);
    }
    const size_t audioTests = runAudioTests();
    const size_t debugTests = runDebugTests();
    const size_t inputTests = runInputTests();
    const size_t ledTests = runLedStripTests();
    printf("\n%zu native regression tests passed.\n", sizeof(Tests) / sizeof(Tests[0]) + audioTests + debugTests + inputTests + ledTests);
    return EXIT_SUCCESS;
}