#ifndef GYEREK_JATEK_CHALLENGE_GAMES_H
#define GYEREK_JATEK_CHALLENGE_GAMES_H

#include "game/PanelGames.h"
#include <stdint.h>

namespace game {

enum class ChallengeMode : uint8_t { Disabled, Lock3, Lock4, Duel };

constexpr uint8_t ChallengeSwitchMask = 0x1f;
constexpr uint16_t DuelButtonMask = 0x0707;
constexpr uint8_t DuelLeft = 1;
constexpr uint8_t DuelRight = 2;
constexpr uint8_t DuelCueChannel = 5; // PanelConfig bottom-middle channel.

// Portable, allocation-free engine. No GPIO, clock, audio or scheduler ownership.
// switchHighMask bits 0..4 are literal HIGH readings of SW01..05, NOT active-low
// "pressed" bits. upperPressedMask uses PanelConfig channels 0..2 and 8..10;
// startPressed is logical D39 pressed (the adapter handles its active LOW).
class ChallengeGames {
public:
    explicit ChallengeGames(uint32_t (*randomBelow)(uint32_t) = nullptr);

    // Selection/reselection starts the first round immediately. Snapshot ALL
    // inputs to suppress controls held on selection. Invalid modes disable.
    void select(ChallengeMode mode, uint32_t now, uint8_t switchHighMask = 0,
                bool startPressed = false, uint16_t upperPressedMask = 0);
    // True on phase, warning or visible blink-frame changes. At most one phase
    // transition per update; delayed updates never replay old warnings/rounds.
    bool update(uint32_t now, uint8_t switchHighMask, bool startPressed,
                uint16_t upperPressedMask = 0);
    PanelFeedback takeFeedback();

    ChallengeMode mode() const { return mode_; }
    PanelPhase phase() const { return phase_; }
    // Lit metal LEDs during Input. Lock3 requires HIGH exactly at lit bits;
    // Lock4 requires the inverse. There is no "active LOW" reinterpretation.
    uint8_t targetMask() const { return targetMask_; }
    uint8_t requiredSwitchMask() const;
    // Six fixed hues are valid during Showing/Input; invalid channels return 0
    // (also a valid red hue, so the renderer must check channel and phase).
    uint8_t duelHue(uint8_t channel) const;
    uint8_t colorHue(uint8_t side, uint8_t index) const;
    uint8_t cueHue() const { return cueHue_; }
    bool cueVisible() const { return mode_ == ChallengeMode::Duel && phase_ == PanelPhase::Input; }
    // Duel result side bits: DuelLeft=1, DuelRight=2. Single answer: correct
    // side wins / other loses; wrong side loses / other wins. Same-update
    // both-correct is a green draw (Win); both-wrong a red draw (Loss).
    // Mixed simultaneous answers award the correct side. A raw same-side chord
    // is wrong even if its extra button has not finished debouncing yet.
    uint8_t wonSides() const { return wonSides_; }
    uint8_t lostSides() const { return lostSides_; }
    // Results: lock Win 540 ms / Loss 740 ms; EVERY duel result 540 ms.
    // Renderer colors whole side groups using won/lost masks, only while this
    // is true. 90 ms on/off, except lock Loss uses 100 ms on/off. Ready is dark.
    bool resultVisible(uint32_t now) const;

private:
    uint32_t (*randomBelow_)(uint32_t);
    DebouncedButton startButton_;
    DebouncedButton upperButtons_[6];
    ChallengeMode mode_ = ChallengeMode::Disabled;
    PanelPhase phase_ = PanelPhase::Disabled;
    PanelFeedback feedback_ = PanelFeedback::None;
    uint32_t phaseStarted_ = 0;
    uint32_t switchChangedAt_ = 0;
    uint32_t releasedAt_ = 0;
    uint8_t rawSwitches_ = 0;
    uint8_t stableSwitches_ = 0;
    uint8_t targetMask_ = 0;
    uint8_t hues_[6] = {};
    uint8_t cueHue_ = 0;
    uint8_t warnings_ = 0;
    uint8_t wonSides_ = 0;
    uint8_t lostSides_ = 0;
    bool upperReleased_ = true;
    bool inputArmed_ = false;
    bool resultWasVisible_ = false;

    uint32_t pick(uint32_t bound);
    void resetControls(uint32_t now, bool startPressed, uint16_t upperPressedMask);
    void resetUpper(uint32_t now, uint16_t upperPressedMask);
    void beginRound(uint32_t now, uint8_t switchHighMask);
    void ready(uint32_t now, bool startPressed, uint16_t upperPressedMask);
    void result(uint32_t now, bool win, uint8_t won = 0, uint8_t lost = 0);
    uint32_t resultDuration() const;
    bool warn(uint32_t elapsed);
};

} // namespace game

#endif