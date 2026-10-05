#include "ModeController.h"

namespace app {

bool ModeController::select(Mode mode, uint32_t now, bool buttonPressed) {
    if (mode != Mode::Cyclone && mode != Mode::Music) return false;
    if (hasSelection_ && mode == mode_) return false;

    // Release the effect's buzzer ownership before music can start.
    soundEffects_.stop();
    mode_ = mode;
    hasSelection_ = true;
    game_.setEnabled(mode == Mode::Cyclone, now, buttonPressed);
    music_.setEnabled(mode == Mode::Music, now);
    return true;
}

bool ModeController::restart(uint32_t now, bool buttonPressed) {
    if (!hasSelection_) return false;
    soundEffects_.stop();
    if (mode_ == Mode::Cyclone) game_.restart(now, buttonPressed);
    else music_.restart(now);
    return true;
}

bool ModeController::update(uint32_t now, bool buttonPressed) {
    if (!hasSelection_) return false;
    const game::State previousState = game_.state();
    const bool frameChanged = game_.update(now, buttonPressed);
    const game::State currentState = game_.state();
    if (mode_ == Mode::Cyclone && previousState == game::State::Playing) {
        if (currentState == game::State::WinFeedback)
            soundEffects_.play(audio::Effect::Win, now);
        else if (currentState == game::State::LossFeedback)
            soundEffects_.play(audio::Effect::Loss, now);
    } else if ((previousState == game::State::WinFeedback ||
                previousState == game::State::LossFeedback) && currentState != previousState) {
        // Also cancel if feedback ends before a delayed audio task catches up.
        soundEffects_.stop();
    }
    return frameChanged;
}

void ModeController::updateAudio(uint32_t now) {
    if (!hasSelection_) return;
    if (mode_ == Mode::Music) music_.update(now);
    else soundEffects_.update(now);
}

} // namespace app