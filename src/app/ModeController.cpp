#include "ModeController.h"

namespace app {

bool ModeController::selectCycloneLevel(uint8_t menuLevel) {
    return game_.setMenuLevel(menuLevel);
}

bool ModeController::select(Mode mode, uint32_t now, bool buttonPressed) {
    if (mode != Mode::Cyclone && mode != Mode::Debug) return false;
    if (hasSelection_ && mode == mode_) return false;

    // Debug must not play result sounds while inputs are being tested.
    soundEffects_.stop();
    mode_ = mode;
    hasSelection_ = true;
    game_.setEnabled(mode == Mode::Cyclone, now, buttonPressed);
    return true;
}

bool ModeController::restart(uint32_t now, bool buttonPressed) {
    if (!hasSelection_) return false;
    soundEffects_.stop();
    mode_ = Mode::Cyclone;
    game_.restart(now, buttonPressed);
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
    if (mode_ == Mode::Cyclone) soundEffects_.update(now);
}

} // namespace app