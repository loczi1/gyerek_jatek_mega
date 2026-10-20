#include "SoundEffects.h"
#include "game/PanelGames.h"

#ifdef ARDUINO_ARCH_AVR
#include <avr/pgmspace.h>
#define EFFECT_STORAGE PROGMEM
#else
#define EFFECT_STORAGE
#endif

namespace audio {
namespace {

struct Step {
    uint16_t frequency;
    uint16_t durationMs;
};

// frequency 0 is an explicit silent gap. Edit these flash-backed patterns to
// customize the sounds; durations include the gaps and never block a task.
const Step WinPattern[] EFFECT_STORAGE = {
    {523, 120}, {0, 30}, {659, 120}, {0, 30}, {784, 240}
};
const Step LossPattern[] EFFECT_STORAGE = {
    {392, 180}, {0, 40}, {294, 180}, {0, 40}, {196, 300}
};
const Step WarningPattern[] EFFECT_STORAGE = {{880, 80}};
// Tiny original tune, not the full standalone score/player. Eight 150 ms
// beats share Random2's light-effect duration; just 32 bytes of flash data.
const Step PlayfulPattern[] EFFECT_STORAGE = {
    {523, game::Random2BeatMs}, {659, game::Random2BeatMs},
    {784, game::Random2BeatMs}, {659, game::Random2BeatMs},
    {698, game::Random2BeatMs}, {587, game::Random2BeatMs},
    {523, game::Random2BeatMs}, {0, game::Random2BeatMs}
};
static_assert(sizeof(PlayfulPattern) / sizeof(PlayfulPattern[0]) * game::Random2BeatMs ==
              game::Random2DurationMs, "Random2 audio/light duration mismatch");

uint8_t stepCount(Effect effect) {
    if (effect == Effect::Win) return sizeof(WinPattern) / sizeof(WinPattern[0]);
    if (effect == Effect::Loss) return sizeof(LossPattern) / sizeof(LossPattern[0]);
    if (effect == Effect::Playful) return sizeof(PlayfulPattern) / sizeof(PlayfulPattern[0]);
    if (effect == Effect::Warning) return sizeof(WarningPattern) / sizeof(WarningPattern[0]);
    return 0;
}

Step readStep(Effect effect, uint8_t index) {
    if (index >= stepCount(effect)) return {0, 0};
    const Step* pattern = effect == Effect::Win ? WinPattern :
        (effect == Effect::Playful ? PlayfulPattern :
         (effect == Effect::Warning ? WarningPattern : LossPattern));
#ifdef ARDUINO_ARCH_AVR
    return {
        pgm_read_word(&pattern[index].frequency),
        pgm_read_word(&pattern[index].durationMs)
    };
#else
    return pattern[index];
#endif
}

} // namespace

void SoundEffects::stop() {
    // An idle effects player must not silence another owner of the buzzer.
    if (isPlaying() && stopTone_) stopTone_();
    effect_ = Effect::None;
    step_ = 0;
}

void SoundEffects::play(Effect effect, uint32_t now) {
    stop();
    if (stepCount(effect) == 0) return;
    effect_ = effect;
    startedAt_ = now;
    startStep(0, readStep(effect_, 0).durationMs);
}

void SoundEffects::startStep(uint8_t index, uint32_t remainingMs) {
    step_ = index;
    const Step step = readStep(effect_, index);
    if (step.frequency != 0) {
        if (playTone_) playTone_(step.frequency, remainingMs);
    } else if (stopTone_) {
        stopTone_();
    }
}

void SoundEffects::update(uint32_t now) {
    if (!isPlaying()) return;

    const uint32_t elapsed = static_cast<uint32_t>(now - startedAt_);
    uint32_t stepEndsAt = 0;
    // Bounded scan, not a wait loop. Skip expired notes if the scheduler is
    // delayed rather than replaying a stale effect during the next level.
    for (uint8_t index = 0; index < stepCount(effect_); ++index) {
        stepEndsAt += readStep(effect_, index).durationMs;
        if (elapsed < stepEndsAt) {
            if (index != step_) startStep(index, stepEndsAt - elapsed);
            return;
        }
    }
    stop();
}

} // namespace audio