#ifndef GYEREK_JATEK_MUSIC_PLAYER_H
#define GYEREK_JATEK_MUSIC_PLAYER_H

#include <stdint.h>

namespace music {

class MusicPlayer {
public:
    using ToneOutput = void (*)(uint16_t frequency, uint32_t durationMs);
    using StopOutput = void (*)();

    MusicPlayer(ToneOutput playTone = nullptr, StopOutput stopTone = nullptr)
        : playTone_(playTone), stopTone_(stopTone) {}

    void setEnabled(bool enabled, uint32_t now);
    void restart(uint32_t now);
    void update(uint32_t now);
    bool isPlaying() const { return enabled_; }
    uint16_t currentNote() const { return currentNote_; }
    uint32_t currentNoteDuration() const { return noteDuration_; }

private:
    void stop();
    void startNote(uint32_t now);

    ToneOutput playTone_;
    StopOutput stopTone_;
    uint32_t noteStartedAt_ = 0;
    uint32_t noteDuration_ = 0;
    uint16_t currentNote_ = 0;
    bool enabled_ = false;
};

} // namespace music

#endif