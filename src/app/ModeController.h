#ifndef GYEREK_JATEK_MODE_CONTROLLER_H
#define GYEREK_JATEK_MODE_CONTROLLER_H

#include <stdint.h>
#include "audio/SoundEffects.h"
#include "game/CycloneGame.h"

namespace app {

enum class Mode : uint8_t { Cyclone = 0, Debug = 1 };

// Pure game/debug selection logic; no task creation in menu callbacks.
class ModeController {
public:
    ModeController(game::CycloneGame& game, audio::SoundEffects& soundEffects)
        : game_(game), soundEffects_(soundEffects) {}

    bool select(Mode mode, uint32_t now, bool buttonPressed);
    bool restart(uint32_t now, bool buttonPressed);
    // Configure 2..5 while the game is Disabled/Idle, before select/restart.
    // No state/audio reset or implicit game selection; false leaves it unchanged.
    bool selectCycloneLevel(uint8_t menuLevel);
    // Called by the game/input task; only state entry starts a result sound.
    bool update(uint32_t now, bool buttonPressed);
    // Called independently by the cooperative audio task. Debug is silent;
    // the only selectable game is Cyclon, with its win/loss effects.
    void updateAudio(uint32_t now);
    Mode mode() const { return mode_; }

private:
    game::CycloneGame& game_;
    audio::SoundEffects& soundEffects_;
    Mode mode_ = Mode::Cyclone;
    bool hasSelection_ = false;
};

} // namespace app

#endif