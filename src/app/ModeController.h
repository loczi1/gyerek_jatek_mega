#ifndef GYEREK_JATEK_MODE_CONTROLLER_H
#define GYEREK_JATEK_MODE_CONTROLLER_H

#include <stdint.h>
#include "audio/SoundEffects.h"
#include "game/CycloneGame.h"
#include "music/MusicPlayer.h"

namespace app {

enum class Mode : uint8_t { Cyclone = 0, Music = 1 };

// Pure selection logic: no hardware or task creation in menu callbacks.
class ModeController {
public:
    ModeController(game::CycloneGame& game, music::MusicPlayer& music,
                   audio::SoundEffects& soundEffects)
        : game_(game), music_(music), soundEffects_(soundEffects) {}

    bool select(Mode mode, uint32_t now, bool buttonPressed);
    bool restart(uint32_t now, bool buttonPressed);
    // Called by the game/input task; only state entry starts a result sound.
    bool update(uint32_t now, bool buttonPressed);
    // Called independently by the cooperative audio task. The single buzzer
    // plays either the selected music or a game effect, never both at once.
    void updateAudio(uint32_t now);
    Mode mode() const { return mode_; }

private:
    game::CycloneGame& game_;
    music::MusicPlayer& music_;
    audio::SoundEffects& soundEffects_;
    Mode mode_ = Mode::Cyclone;
    bool hasSelection_ = false;
};

} // namespace app

#endif