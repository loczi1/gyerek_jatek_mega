#include "CycloneGame.h"

namespace game {

void CycloneGame::setEnabled(bool enabled, uint32_t now, bool buttonPressed) {
    if (enabled) {
        if (state_ == State::Disabled) restart(now, buttonPressed);
    } else {
        state_ = State::Disabled;
        level_ = 0;
        position_ = 0;
        button_.reset(now, buttonPressed);
    }
}

void CycloneGame::restart(uint32_t now, bool buttonPressed) {
    state_ = State::Idle;
    stateStartedAt_ = now;
    lastMoveAt_ = now;
    level_ = 0;
    position_ = 0;
    feedbackPhase_ = 0;
    clearedGameLeds_ = 0;
    clearedScoreLeds_ = 0;
    button_.reset(now, buttonPressed);
}

uint8_t CycloneGame::targetRadius(uint8_t level) {
    return level == 1 ? 2 : (level == 2 ? 1 : 0);
}

bool CycloneGame::setMenuLevel(uint8_t menuLevel) {
    if (menuLevel < 2 || menuLevel > 5 ||
        (state_ != State::Disabled && state_ != State::Idle)) return false;
    menuLevel_ = menuLevel;
    return true;
}

uint8_t CycloneGame::targetWindowSize(uint8_t menuLevel, uint8_t level) {
    if (level < 1 || level > LevelCount) return 0;
    if (menuLevel == 0) return static_cast<uint8_t>(2 * targetRadius(level) + 1);
    if (menuLevel == 2 || menuLevel == 3) {
        switch (level) {
            case 1: return 9;
            case 2: case 3: return 5;
            default: return 3;
        }
    }
    if (menuLevel == 4 || menuLevel == 5) {
        switch (level) {
            case 1: return 5;
            case 2: return 3;
            case 3: return 2;
            default: return menuLevel == 4 ? 2 : 1;
        }
    }
    return 0;
}

uint32_t CycloneGame::movementInterval(uint8_t menuLevel, uint8_t level) {
    if (level < 1 || level > LevelCount) return 0;
    switch (menuLevel) {
        case 0: return movementInterval(level);
        case 2: return level == 1 ? 50 : 40;
        case 3: return level == 1 ? 40 : (level == 2 ? 35 : 30);
        case 4: return movementInterval(level);
        case 5: return level == 1 ? 40 : (level == 2 ? 35 : (level == 3 ? 30 : 20));
        default: return 0;
    }
}

uint32_t CycloneGame::movementInterval(uint8_t level) {
    switch (level) {
        case 1: return 50;
        case 2: return 40;
        case 3: return 35;
        case 4: return 20;
        default: return 0;
    }
}

bool CycloneGame::isHit(uint8_t level, uint8_t position, uint8_t target) {
    return isHit(0, level, position, target);
}

bool CycloneGame::isHit(uint8_t menuLevel, uint8_t level, uint8_t position, uint8_t target) {
    const uint8_t width = targetWindowSize(menuLevel, level);
    if (width == 0 || position >= GameLedCount ||
        target < MinimumTarget || target > MaximumTarget) return false;

    // Two pixels mean exactly centre and centre+1, not a radius of one.
    const int before = (width - 1) / 2;
    const int after = width / 2;
    if (target < before || static_cast<int>(target) + after >= GameLedCount) return false;
    const int distance = static_cast<int>(position) - static_cast<int>(target);
    return distance >= -before && distance <= after;
}

uint8_t CycloneGame::scoreLeds() const {
    if (state_ == State::Playing || state_ == State::WinFeedback ||
        state_ == State::LossFeedback) {
        // The fifth pixel celebrates completing all four difficulty levels.
        return state_ == State::WinFeedback && level_ == LevelCount
            ? ScoreLedCount : level_;
    }
    return 0;
}

void CycloneGame::startLevel(uint8_t level, uint32_t now) {
    level_ = level;
    position_ = 0;
    target_ = pickTarget_ ? pickTarget_() : MinimumTarget;
    if (target_ < MinimumTarget) target_ = MinimumTarget;
    const uint8_t before = static_cast<uint8_t>((targetWindowSize() - 1) / 2);
    if (target_ < before) target_ = before;
    if (target_ > MaximumTarget) target_ = MaximumTarget;
    lastMoveAt_ = now;
    stateStartedAt_ = now;
    state_ = State::Playing;
}

void CycloneGame::enterFeedback(bool won, uint32_t now) {
    state_ = won ? State::WinFeedback : State::LossFeedback;
    stateStartedAt_ = now;
    feedbackPhase_ = 0;
}

bool CycloneGame::update(uint32_t now, bool buttonPressed) {
    const bool pressed = button_.update(now, buttonPressed);

    // A switch deliberately processes only the state present on entry. A start
    // event cannot also become a stop event in this invocation.
    switch (state_) {
        case State::Disabled:
            return false;

        case State::Idle:
            if (!pressed) return false;
            state_ = State::Intro;
            stateStartedAt_ = now;
            clearedGameLeds_ = 0;
            clearedScoreLeds_ = 0;
            return true;

        case State::Intro: {
            const uint32_t elapsed = static_cast<uint32_t>(now - stateStartedAt_);
            if (elapsed >= IntroDurationMs) {
                startLevel(1, now);
                return true;
            }

            const uint32_t gameSteps = elapsed < IntroPauseMs ? 0 :
                (elapsed - IntroPauseMs) / IntroGameStepMs;
            const uint8_t clearedGame = gameSteps >= GameLedCount
                ? GameLedCount : static_cast<uint8_t>(gameSteps);
            const uint32_t scoreStart = IntroPauseMs + GameLedCount * IntroGameStepMs;
            const uint8_t clearedScore = elapsed < scoreStart ? 0 :
                static_cast<uint8_t>((elapsed - scoreStart) / IntroScoreStepMs);

            if (clearedGame == clearedGameLeds_ && clearedScore == clearedScoreLeds_)
                return false;
            clearedGameLeds_ = clearedGame;
            clearedScoreLeds_ = clearedScore;
            return true;
        }

        case State::Playing:
            // Judge the pixel the player actually saw before moving it.
            if (pressed) {
                enterFeedback(isTargetPixel(position_), now);
                return true;
            }
            if (static_cast<uint32_t>(now - lastMoveAt_) < movementInterval())
                return false;
            lastMoveAt_ = now;
            position_ = static_cast<uint8_t>((position_ + 1) % GameLedCount);
            return true;

        case State::WinFeedback:
        case State::LossFeedback: {
            const uint32_t elapsed = static_cast<uint32_t>(now - stateStartedAt_);
            if (elapsed >= FeedbackDurationMs) {
                if (state_ == State::WinFeedback && level_ < LevelCount) {
                    startLevel(static_cast<uint8_t>(level_ + 1), now);
                } else {
                    // Keep the debounce state: a held press must not restart.
                    state_ = State::Idle;
                    level_ = 0;
                    position_ = 0;
                }
                return true;
            }
            const uint8_t phase = static_cast<uint8_t>(elapsed / FeedbackPhaseMs);
            if (phase == feedbackPhase_) return false;
            feedbackPhase_ = phase;
            return true;
        }
    }
    return false;
}

} // namespace game