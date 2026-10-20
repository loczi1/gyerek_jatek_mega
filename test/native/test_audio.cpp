#include "test_audio.h"
#include "audio/SoundEffects.h"
#include "game/PanelGames.h"
#include "app/ModeController.h"
#include "music/MusicPlayer.h"
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

struct OutputEvent {
    uint16_t frequency; // Zero denotes a stop callback, never a tone request.
    uint32_t durationMs;
};

OutputEvent events[32];
size_t eventCount = 0;
unsigned int tones = 0;
unsigned int stops = 0;

void record(uint16_t frequency, uint32_t duration) {
    CHECK(eventCount < sizeof(events) / sizeof(events[0]));
    events[eventCount++] = {frequency, duration};
}

void playTone(uint16_t frequency, uint32_t duration) {
    CHECK(frequency != 0);
    CHECK(duration != 0);
    ++tones;
    record(frequency, duration);
}

void stopTone() {
    ++stops;
    record(0, 0);
}

uint8_t pickTarget() { return 20; }

struct Fixture {
    game::CycloneGame game{pickTarget};
    audio::SoundEffects effects{playTone, stopTone};
    app::ModeController modes{game, effects};
};

uint32_t startGame(Fixture& fixture) {
    CHECK(fixture.modes.select(app::Mode::Cyclone, 0, false));
    fixture.modes.update(1, true);
    CHECK(fixture.modes.update(31, true));
    CHECK(fixture.game.state() == game::State::Intro);
    fixture.modes.update(32, false);
    fixture.modes.update(62, false);
    const uint32_t now = 31 + game::IntroDurationMs;
    CHECK(fixture.modes.update(now, false));
    CHECK(fixture.game.state() == game::State::Playing);
    CHECK(tones == 0);
    return now;
}

uint32_t triggerWin(Fixture& fixture, uint32_t now) {
    while (fixture.game.position() != fixture.game.target()) {
        now += game::CycloneGame::movementInterval(fixture.game.level());
        fixture.modes.update(now, false);
    }
    fixture.modes.update(now + 1U, true);
    now += 31U;
    CHECK(fixture.modes.update(now, true));
    CHECK(fixture.game.state() == game::State::WinFeedback);
    return now;
}

uint32_t triggerLoss(Fixture& fixture, uint32_t now) {
    CHECK(fixture.game.position() == 0);
    fixture.modes.update(now + 1U, true);
    now += 31U;
    CHECK(fixture.modes.update(now, true));
    CHECK(fixture.game.state() == game::State::LossFeedback);
    return now;
}

void testPlayfulMelodyBeatsAndRollover() {
    audio::SoundEffects effects(playTone, stopTone);
    const uint32_t now = UINT32_MAX - 50U;
    const uint16_t notes[] = {523, 659, 784, 659, 698, 587, 523};
    effects.play(audio::Effect::Playful, now);
    for (uint8_t i = 0; i < 7; ++i) {
        effects.update(now + i * game::Random2BeatMs);
        CHECK(tones == i + 1U);
        CHECK(events[i].frequency == notes[i]);
        CHECK(events[i].durationMs == game::Random2BeatMs);
        effects.update(now + (i + 1U) * game::Random2BeatMs - 1U);
        CHECK(tones == i + 1U);
    }
    effects.update(now + 1050U);
    CHECK(stops == 1 && effects.isPlaying());
    effects.update(now + game::Random2DurationMs);
    CHECK(!effects.isPlaying() && tones == 7);
}

void testPlayfulReplacementDelayAndCancellation() {
    audio::SoundEffects effects(playTone, stopTone);
    effects.play(audio::Effect::Playful, 0);
    effects.update(920);
    CHECK(tones == 2 && events[1].frequency == 523 && events[1].durationMs == 130);
    effects.play(audio::Effect::Playful, 930);
    CHECK(stops == 1 && tones == 3);
    CHECK(events[3].frequency == 523 && events[3].durationMs == 150);
    effects.stop();
    effects.update(5000);
    CHECK(!effects.isPlaying() && stops == 2 && tones == 3);
}

void testWinPatternAndSilenceGaps() {
    audio::SoundEffects effects(playTone, stopTone);
    effects.play(audio::Effect::Win, 100);
    CHECK(effects.currentEffect() == audio::Effect::Win);
    CHECK(events[0].frequency == 523 && events[0].durationMs == 120);
    effects.update(219);
    CHECK(eventCount == 1);
    effects.update(220);
    CHECK(stops == 1);
    effects.update(249);
    CHECK(eventCount == 2);
    effects.update(250);
    CHECK(events[2].frequency == 659 && events[2].durationMs == 120);
    effects.update(370);
    CHECK(stops == 2);
    effects.update(400);
    CHECK(events[4].frequency == 784 && events[4].durationMs == 240);
    effects.update(639);
    CHECK(effects.isPlaying());
    effects.update(640);
    CHECK(!effects.isPlaying());
    CHECK(tones == 3 && stops == 3);
    effects.update(10000);
    CHECK(eventCount == 6);
}

void testLossPatternAndSilenceGaps() {
    audio::SoundEffects effects(playTone, stopTone);
    effects.play(audio::Effect::Loss, 0);
    CHECK(events[0].frequency == 392 && events[0].durationMs == 180);
    effects.update(179);
    CHECK(eventCount == 1);
    effects.update(180);
    CHECK(stops == 1);
    effects.update(219);
    CHECK(eventCount == 2);
    effects.update(220);
    CHECK(events[2].frequency == 294 && events[2].durationMs == 180);
    effects.update(400);
    CHECK(stops == 2);
    effects.update(440);
    CHECK(events[4].frequency == 196 && events[4].durationMs == 300);
    effects.update(739);
    CHECK(effects.isPlaying());
    effects.update(740);
    CHECK(effects.currentEffect() == audio::Effect::None);
    CHECK(tones == 3 && stops == 3);
}

void testEffectTimerRollover() {
    audio::SoundEffects effects(playTone, stopTone);
    const uint32_t now = UINT32_MAX - 20U;
    effects.play(audio::Effect::Win, now);
    effects.update(now + 119U);
    CHECK(eventCount == 1);
    effects.update(now + 120U);
    CHECK(stops == 1);
    effects.update(now + 150U);
    CHECK(events[2].frequency == 659);
    effects.update(now + 270U);
    CHECK(stops == 2);
    effects.update(now + 300U);
    CHECK(events[4].frequency == 784);
    effects.update(now + 539U);
    CHECK(effects.isPlaying());
    effects.update(now + 540U);
    CHECK(!effects.isPlaying());
    CHECK(stops == 3);
}

void testDelayedAudioSkipsExpiredNotes() {
    audio::SoundEffects effects(playTone, stopTone);
    effects.play(audio::Effect::Win, 0);
    effects.update(320);
    CHECK(tones == 2);
    CHECK(events[1].frequency == 784);
    CHECK(events[1].durationMs == 220);
    effects.update(540);
    CHECK(!effects.isPlaying());
    CHECK(stops == 1);
}

void testDelayedAudioPastEndIsSilent() {
    audio::SoundEffects effects(playTone, stopTone);
    effects.play(audio::Effect::Loss, 0);
    effects.update(10000);
    CHECK(!effects.isPlaying());
    CHECK(tones == 1 && stops == 1);
    effects.update(20000);
    CHECK(eventCount == 2);
}

void testReplaceAndCancelEffect() {
    audio::SoundEffects effects(playTone, stopTone);
    effects.play(audio::Effect::Win, 0);
    effects.play(audio::Effect::Loss, 20);
    CHECK(tones == 2 && stops == 1);
    CHECK(events[1].frequency == 0); // Stop old tone before replacing it.
    CHECK(events[2].frequency == 392);
    effects.stop();
    CHECK(!effects.isPlaying());
    CHECK(stops == 2);
    effects.stop();
    effects.update(10000);
    effects.play(audio::Effect::None, 10001);
    effects.play(static_cast<audio::Effect>(99), 10002);
    CHECK(eventCount == 4);
}

void testIdleEffectsDoNotSilenceMusic() {
    music::MusicPlayer music(playTone, stopTone);
    audio::SoundEffects effects(playTone, stopTone);
    music.setEnabled(true, 0);
    effects.stop();
    effects.play(audio::Effect::None, 1);
    effects.update(1000);
    CHECK(tones == 1 && stops == 0);
    CHECK(music.isPlaying());
}

void testWinTriggersOnceAlongsideFlashes() {
    Fixture fixture;
    const uint32_t now = triggerWin(fixture, startGame(fixture));
    CHECK(fixture.effects.currentEffect() == audio::Effect::Win);
    CHECK(fixture.modes.mode() == app::Mode::Cyclone);
    for (uint32_t elapsed = 0; elapsed <= game::FeedbackDurationMs; elapsed += 5U) {
        fixture.modes.update(now + elapsed, true);
        fixture.modes.updateAudio(now + elapsed);
    }
    CHECK(tones == 3 && stops == 3);
    CHECK(!fixture.effects.isPlaying());
    CHECK(fixture.game.state() == game::State::Playing);
    CHECK(fixture.game.level() == 2);
}

void testLossTriggersOnceAlongsideFlashes() {
    Fixture fixture;
    const uint32_t now = triggerLoss(fixture, startGame(fixture));
    CHECK(fixture.effects.currentEffect() == audio::Effect::Loss);
    for (uint32_t elapsed = 0; elapsed <= game::FeedbackDurationMs; elapsed += 5U) {
        fixture.modes.update(now + elapsed, true);
        fixture.modes.updateAudio(now + elapsed);
    }
    CHECK(tones == 3 && stops == 3);
    CHECK(!fixture.effects.isPlaying());
    CHECK(fixture.game.state() == game::State::Idle);
}

void testDebugStopsEffectAndGame() {
    Fixture fixture;
    const uint32_t now = triggerWin(fixture, startGame(fixture));
    CHECK(fixture.modes.select(app::Mode::Debug, now + 1U, false));
    CHECK(!fixture.effects.isPlaying());
    CHECK(fixture.game.state() == game::State::Disabled);
    CHECK(events[1].frequency == 0);
    fixture.modes.updateAudio(now + 150U);
    CHECK(tones == 1 && stops == 1);
    fixture.modes.updateAudio(now + 429U);
    CHECK(fixture.modes.select(app::Mode::Cyclone, now + 430U, false));
    CHECK(!fixture.effects.isPlaying());
    CHECK(fixture.game.state() == game::State::Idle);
    CHECK(stops == 1);
}

void testRestartCancelsResultSound() {
    Fixture fixture;
    const uint32_t now = triggerLoss(fixture, startGame(fixture));
    CHECK(fixture.modes.restart(now + 1U, false));
    CHECK(fixture.game.state() == game::State::Idle);
    CHECK(!fixture.effects.isPlaying());
    fixture.modes.updateAudio(now + 1000U);
    CHECK(tones == 1 && stops == 1);
}

void testSameSelectionDoesNotRetriggerSound() {
    Fixture fixture;
    const uint32_t now = triggerWin(fixture, startGame(fixture));
    for (uint32_t offset = 1; offset <= 50; ++offset)
        CHECK(!fixture.modes.select(app::Mode::Cyclone, now + offset, true));
    CHECK(!fixture.modes.select(static_cast<app::Mode>(99), now + 51U, true));
    CHECK(tones == 1 && stops == 0);
    fixture.modes.updateAudio(now + 150U);
    CHECK(tones == 2);
    CHECK(events[1].frequency == 659);
    fixture.modes.updateAudio(now + 540U);
    CHECK(!fixture.effects.isPlaying());
}

void testFeedbackEndCancelsDelayedSound() {
    Fixture fixture;
    const uint32_t now = triggerWin(fixture, startGame(fixture));
    // No audio updates: still cancel when game feedback finishes.
    fixture.modes.update(now + game::FeedbackDurationMs, true);
    CHECK(fixture.game.level() == 2);
    CHECK(!fixture.effects.isPlaying());
    fixture.modes.updateAudio(now + game::FeedbackDurationMs + 1U);
    CHECK(tones == 1 && stops == 1);
}

void testEveryLevelPlaysOneWinSound() {
    Fixture fixture;
    uint32_t now = startGame(fixture);
    for (uint8_t level = 1; level <= game::LevelCount; ++level) {
        CHECK(fixture.game.level() == level);
        now = triggerWin(fixture, now);
        CHECK(fixture.effects.currentEffect() == audio::Effect::Win);
        for (uint32_t elapsed = 1; elapsed <= game::FeedbackDurationMs + 1U; elapsed += 5U) {
            fixture.modes.update(now + elapsed, false);
            fixture.modes.updateAudio(now + elapsed);
        }
        now += game::FeedbackDurationMs + 1U;
        CHECK(tones == level * 3U);
    }
    CHECK(fixture.game.state() == game::State::Idle);
    CHECK(tones == 12 && stops == 12);
}

void testEffectsAdvanceOnlyInAudioTask() {
    Fixture fixture;
    const uint32_t now = triggerWin(fixture, startGame(fixture));
    for (uint32_t elapsed = 1; elapsed <= 300; elapsed += 5U)
        fixture.modes.update(now + elapsed, false);
    CHECK(fixture.effects.currentEffect() == audio::Effect::Win);
    CHECK(tones == 1);
    fixture.modes.updateAudio(now + 300U);
    CHECK(tones == 2);
    CHECK(events[1].frequency == 784);
    fixture.modes.updateAudio(now + 540U);
    CHECK(!fixture.effects.isPlaying());
    CHECK(fixture.game.state() == game::State::WinFeedback);
}

void testNoSelectionAndNullOutputAreSafe() {
    Fixture fixture;
    CHECK(!fixture.modes.update(1000, true));
    CHECK(!fixture.modes.restart(1000, true));
    fixture.modes.updateAudio(1000);
    CHECK(tones == 0 && stops == 0);
    audio::SoundEffects silent;
    silent.play(audio::Effect::Win, 0);
    CHECK(silent.isPlaying());
    silent.update(1000);
    CHECK(!silent.isPlaying());
}

struct Test { const char* name; void (*run)(); };

const Test Tests[] = {
    {"Random2 playful melody beats and rollover", testPlayfulMelodyBeatsAndRollover},
    {"Random2 melody replacement, delay and cancellation", testPlayfulReplacementDelayAndCancellation},
    {"rising win chime and silent gaps", testWinPatternAndSilenceGaps},
    {"descending loss sound and silent gaps", testLossPatternAndSilenceGaps},
    {"sound effect timer rollover", testEffectTimerRollover},
    {"delayed audio skips expired notes", testDelayedAudioSkipsExpiredNotes},
    {"delayed audio past end stays silent", testDelayedAudioPastEndIsSilent},
    {"sound replacement and cancellation", testReplaceAndCancelEffect},
    {"idle effects do not interrupt music", testIdleEffectsDoNotSilenceMusic},
    {"win plays once alongside LED flashes", testWinTriggersOnceAlongsideFlashes},
    {"loss plays once alongside LED flashes", testLossTriggersOnceAlongsideFlashes},
    {"Debug cancels result sound and game", testDebugStopsEffectAndGame},
    {"restart cancels result sound", testRestartCancelsResultSound},
    {"repeated selection does not retrigger sound", testSameSelectionDoesNotRetriggerSound},
    {"feedback end cancels delayed audio", testFeedbackEndCancelsDelayedSound},
    {"each of four levels plays one win sound", testEveryLevelPlaysOneWinSound},
    {"effects have independent audio updates", testEffectsAdvanceOnlyInAudioTask},
    {"unselected mode and null audio outputs", testNoSelectionAndNullOutputAreSafe}
};

} // namespace

size_t runAudioTests() {
    for (const Test& test : Tests) {
        eventCount = 0;
        tones = stops = 0;
        test.run();
        printf("PASS: %s\n", test.name);
    }
    return sizeof(Tests) / sizeof(Tests[0]);
}