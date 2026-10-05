#ifndef GYEREK_JATEK_SOUND_EFFECTS_H
#define GYEREK_JATEK_SOUND_EFFECTS_H

#include <stdint.h>

namespace audio {

enum class Effect : uint8_t { None, Win, Loss };

// Cooperative, allocation-free sequencer. Call update() from an audio task;
// output callbacks start/stop a tone and must never wait for it to finish.
class SoundEffects {
public:
    using ToneOutput = void (*)(uint16_t frequency, uint32_t durationMs);
    using StopOutput = void (*)();

    SoundEffects(ToneOutput playTone = nullptr, StopOutput stopTone = nullptr)
        : playTone_(playTone), stopTone_(stopTone) {}

    void play(Effect effect, uint32_t now);
    void stop();
    void update(uint32_t now);
    bool isPlaying() const { return effect_ != Effect::None; }
    Effect currentEffect() const { return effect_; }

private:
    void startStep(uint8_t index, uint32_t remainingMs);

    ToneOutput playTone_;
    StopOutput stopTone_;
    uint32_t startedAt_ = 0;
    Effect effect_ = Effect::None;
    uint8_t step_ = 0;
};

} // namespace audio

#endif