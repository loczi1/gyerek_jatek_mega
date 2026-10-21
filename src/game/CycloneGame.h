#ifndef GYEREK_JATEK_CYCLONE_GAME_H
#define GYEREK_JATEK_CYCLONE_GAME_H

#include <stdint.h>
#include "DebouncedButton.h"

namespace game {

constexpr uint8_t GameLedCount = 70;
constexpr uint8_t ScoreLedCount = 5;
constexpr uint8_t TotalLedCount = GameLedCount + ScoreLedCount;
constexpr uint8_t LevelCount = 4;

// Preserve the original target zone. Nine-pixel profiles clamp the picked
// centre to at least 4 so their complete window fits on the game strip.
constexpr uint8_t MinimumTarget = 3;
constexpr uint8_t MaximumTarget = 46;
constexpr uint32_t IntroPauseMs = 500;
constexpr uint32_t IntroGameStepMs = 40;
constexpr uint32_t IntroScoreStepMs = 100;
constexpr uint32_t IntroDurationMs = IntroPauseMs +
    GameLedCount * IntroGameStepMs + ScoreLedCount * IntroScoreStepMs;
constexpr uint32_t FeedbackPhaseMs = 500;
constexpr uint8_t FeedbackPhaseCount = 6; // Three on/off flashes.
constexpr uint32_t FeedbackDurationMs = FeedbackPhaseMs * FeedbackPhaseCount;

static_assert(MinimumTarget >= 2, "Target window would underflow");
static_assert(MaximumTarget + 2 < GameLedCount, "Target window would overflow");
static_assert(MaximumTarget + 4 < GameLedCount, "Nine-pixel window would overflow");
static_assert(ScoreLedCount >= LevelCount + 1, "Need a final-victory indicator");

enum class State : uint8_t {
    Disabled,
    Idle,
    Intro,
    Playing,
    WinFeedback,
    LossFeedback
};

class CycloneGame {
public:
    using TargetPicker = uint8_t (*)();

    explicit CycloneGame(TargetPicker pickTarget = nullptr)
        : pickTarget_(pickTarget) {}

    void setEnabled(bool enabled, uint32_t now, bool buttonPressed = false);
    void restart(uint32_t now, bool buttonPressed = false);
    // Select 2..5 only while Disabled/Idle; invalid or active changes fail
    // without side effects. Default 0 preserves the legacy four-stage profile.
    bool setMenuLevel(uint8_t menuLevel);
    uint8_t menuLevel() const { return menuLevel_; }
    uint32_t movementInterval() const { return movementInterval(menuLevel_, level_); }
    uint8_t targetWindowSize() const { return targetWindowSize(menuLevel_, level_); }
    // Renderer must use this same predicate, not the legacy symmetric radius.
    bool isTargetPixel(uint8_t position) const {
        return isHit(menuLevel_, level_, position, target_);
    }
    // Returns true only when the LED frame needs updating.
    bool update(uint32_t now, bool buttonPressed);

    State state() const { return state_; }
    uint8_t level() const { return level_; }
    uint8_t position() const { return position_; } // The displayed pixel, not the next one.
    uint8_t target() const { return target_; }
    uint8_t clearedGameLeds() const { return clearedGameLeds_; }
    uint8_t clearedScoreLeds() const { return clearedScoreLeds_; }
    bool feedbackOn() const { return feedbackPhase_ % 2 == 0; }
    uint8_t scoreLeds() const;

    static uint8_t targetRadius(uint8_t level);
    static uint32_t movementInterval(uint8_t level);
    static bool isHit(uint8_t level, uint8_t position, uint8_t target);
    // menuLevel 0 is legacy; 2..5 are workbook profiles; other values fail closed.
    static uint8_t targetWindowSize(uint8_t menuLevel, uint8_t level);
    static uint32_t movementInterval(uint8_t menuLevel, uint8_t level);
    static bool isHit(uint8_t menuLevel, uint8_t level, uint8_t position, uint8_t target);

private:
    void startLevel(uint8_t level, uint32_t now);
    void enterFeedback(bool won, uint32_t now);

    TargetPicker pickTarget_;
    DebouncedButton button_;
    State state_ = State::Disabled;
    uint32_t stateStartedAt_ = 0;
    uint32_t lastMoveAt_ = 0;
    uint8_t level_ = 0;
    uint8_t menuLevel_ = 0;
    uint8_t position_ = 0;
    uint8_t target_ = MinimumTarget;
    uint8_t feedbackPhase_ = 0;
    uint8_t clearedGameLeds_ = 0;
    uint8_t clearedScoreLeds_ = 0;
};

} // namespace game

#endif