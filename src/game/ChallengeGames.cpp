#include "game/ChallengeGames.h"

namespace game {
namespace {
constexpr uint32_t DebounceMs = 30;
constexpr uint32_t PreviewMs = 2000;
constexpr uint8_t Channels[6] = {0, 1, 2, 8, 9, 10};
constexpr uint8_t Palette[5] = {0, 32, 64, 96, 160};

void swapHue(uint8_t& left, uint8_t& right) {
    const uint8_t value = left;
    left = right;
    right = value;
}
} // namespace

ChallengeGames::ChallengeGames(uint32_t (*randomBelow)(uint32_t))
    : randomBelow_(randomBelow) {}

uint32_t ChallengeGames::pick(uint32_t bound) {
    // All callers supply a nonzero bound; faulty callbacks are bounded too.
    return randomBelow_ ? randomBelow_(bound) % bound : 0;
}

void ChallengeGames::resetUpper(uint32_t now, uint16_t upperPressedMask) {
    for (uint8_t i = 0; i < 6; ++i)
        upperButtons_[i].reset(now, (upperPressedMask & (uint16_t(1) << Channels[i])) != 0);
}

void ChallengeGames::resetControls(uint32_t now, bool startPressed, uint16_t upperPressedMask) {
    startButton_.reset(now, startPressed);
    resetUpper(now, upperPressedMask);
    upperReleased_ = (upperPressedMask & DuelButtonMask) == 0;
    releasedAt_ = now;
    inputArmed_ = false;
}

void ChallengeGames::select(ChallengeMode mode, uint32_t now, uint8_t switchHighMask,
                            bool startPressed, uint16_t upperPressedMask) {
    mode_ = (mode == ChallengeMode::Lock3 || mode == ChallengeMode::Lock4 ||
             mode == ChallengeMode::Duel) ? mode : ChallengeMode::Disabled;
    ready(now, startPressed, upperPressedMask);
    rawSwitches_ = stableSwitches_ = switchHighMask & ChallengeSwitchMask;
    switchChangedAt_ = now;
    if (mode_ == ChallengeMode::Disabled) phase_ = PanelPhase::Disabled;
    else beginRound(now, rawSwitches_);
}

void ChallengeGames::beginRound(uint32_t now, uint8_t switchHighMask) {
    phaseStarted_ = switchChangedAt_ = now;
    feedback_ = PanelFeedback::None;
    warnings_ = wonSides_ = lostSides_ = 0;
    resultWasVisible_ = inputArmed_ = false;
    rawSwitches_ = stableSwitches_ = switchHighMask & ChallengeSwitchMask;
    if (mode_ != ChallengeMode::Duel) {
        phase_ = PanelPhase::Input;
        // Sample 31 possibilities, skip the displayed pattern whose required
        // GPIO combination already matches. No retry loop, even for constant RNG.
        const uint8_t excluded = mode_ == ChallengeMode::Lock3 ? rawSwitches_ :
            static_cast<uint8_t>(rawSwitches_ ^ ChallengeSwitchMask);
        targetMask_ = static_cast<uint8_t>(pick(31));
        if (targetMask_ >= excluded) ++targetMask_;
        return;
    }
    phase_ = PanelPhase::Showing;
    uint8_t palette[5];
    for (uint8_t i = 0; i < 5; ++i) palette[i] = Palette[i];
    // Five-color permutation: one shared color, two exclusive colors per side.
    // Fisher-Yates plus independent side shuffles has fixed bounded work.
    for (uint8_t count = 5; count > 1; --count)
        swapHue(palette[count - 1U], palette[pick(count)]);
    cueHue_ = palette[0];
    hues_[0] = hues_[3] = cueHue_;
    hues_[1] = palette[1];
    hues_[2] = palette[2];
    hues_[4] = palette[3];
    hues_[5] = palette[4];
    for (uint8_t base = 0; base < 6; base += 3)
        for (uint8_t count = 3; count > 1; --count)
            swapHue(hues_[base + count - 1U], hues_[base + pick(count)]);
}

void ChallengeGames::ready(uint32_t now, bool startPressed, uint16_t upperPressedMask) {
    phase_ = PanelPhase::Ready;
    phaseStarted_ = now;
    feedback_ = PanelFeedback::None;
    targetMask_ = cueHue_ = warnings_ = wonSides_ = lostSides_ = 0;
    for (uint8_t i = 0; i < 6; ++i) hues_[i] = 0;
    resultWasVisible_ = false;
    // Discard partial presses from feedback, not just fully debounced ones.
    resetControls(now, startPressed, upperPressedMask);
}

uint8_t ChallengeGames::requiredSwitchMask() const {
    if (mode_ == ChallengeMode::Lock3) return targetMask_;
    if (mode_ == ChallengeMode::Lock4) return targetMask_ ^ ChallengeSwitchMask;
    return 0;
}

uint8_t ChallengeGames::duelHue(uint8_t channel) const {
    for (uint8_t i = 0; i < 6; ++i)
        if (Channels[i] == channel) return hues_[i];
    return 0;
}

uint8_t ChallengeGames::colorHue(uint8_t side, uint8_t index) const {
    if (index >= 3 || (side != DuelLeft && side != DuelRight)) return 0;
    return hues_[(side == DuelLeft ? 0 : 3) + index];
}

uint32_t ChallengeGames::resultDuration() const {
    return mode_ != ChallengeMode::Duel && phase_ == PanelPhase::Loss ? 740U : 540U;
}

bool ChallengeGames::resultVisible(uint32_t now) const {
    if (phase_ != PanelPhase::RoundWin && phase_ != PanelPhase::Loss) return false;
    const uint32_t elapsed = static_cast<uint32_t>(now - phaseStarted_);
    const uint32_t beat = mode_ != ChallengeMode::Duel && phase_ == PanelPhase::Loss ? 100U : 90U;
    return elapsed < resultDuration() && (elapsed / beat) % 2U == 0;
}

void ChallengeGames::result(uint32_t now, bool win, uint8_t won, uint8_t lost) {
    phase_ = win ? PanelPhase::RoundWin : PanelPhase::Loss;
    phaseStarted_ = now;
    feedback_ = win ? PanelFeedback::Win : PanelFeedback::Loss;
    wonSides_ = won;
    lostSides_ = lost;
    resultWasVisible_ = true;
}

bool ChallengeGames::warn(uint32_t elapsed) {
    const uint8_t due = elapsed >= 1500U ? 3U : (elapsed >= 1000U ? 1U : 0U);
    if ((due & static_cast<uint8_t>(~warnings_)) == 0) return false;
    warnings_ |= due; // Coalesce missed warnings; never emit a catch-up burst.
    feedback_ = PanelFeedback::Warning;
    return true;
}

PanelFeedback ChallengeGames::takeFeedback() {
    const PanelFeedback feedback = feedback_;
    feedback_ = PanelFeedback::None;
    return feedback;
}

bool ChallengeGames::update(uint32_t now, uint8_t switchHighMask, bool startPressed,
                            uint16_t upperPressedMask) {
    if (mode_ == ChallengeMode::Disabled) return false;
    switchHighMask &= ChallengeSwitchMask;
    upperPressedMask &= DuelButtonMask;
    const bool start = startButton_.update(now, startPressed);
    uint16_t rising = 0;
    for (uint8_t i = 0; i < 6; ++i)
        if (upperButtons_[i].update(now, (upperPressedMask & (uint16_t(1) << Channels[i])) != 0))
            rising |= uint16_t(1) << Channels[i];
    if (upperReleased_ != (upperPressedMask == 0)) {
        upperReleased_ = upperPressedMask == 0;
        releasedAt_ = now;
    }
    if (switchHighMask != rawSwitches_) {
        rawSwitches_ = switchHighMask;
        switchChangedAt_ = now;
    }
    const bool stable = static_cast<uint32_t>(now - switchChangedAt_) >= DebounceMs;
    if (stable) stableSwitches_ = rawSwitches_;
    const uint32_t elapsed = static_cast<uint32_t>(now - phaseStarted_);

    if (phase_ == PanelPhase::RoundWin || phase_ == PanelPhase::Loss) {
        if (elapsed >= resultDuration()) {
            ready(now, startPressed, upperPressedMask);
            return true;
        }
        const bool visible = resultVisible(now);
        if (visible == resultWasVisible_) return false;
        resultWasVisible_ = visible;
        return true;
    }
    if (phase_ == PanelPhase::Ready) {
        if (mode_ == ChallengeMode::Duel ? rising != 0 : start) {
            beginRound(now, switchHighMask);
            return true;
        }
        return false;
    }
    if (mode_ != ChallengeMode::Duel) {
        // The deadline wins even if the final switch becomes stable this tick.
        if (mode_ == ChallengeMode::Lock4 && elapsed >= PreviewMs) {
            result(now, false);
            return true;
        }
        // Check the actual whole vector, including stable LOW levels. A fresh
        // mismatch invalidates matching immediately, even before debounce ends.
        if (stable && rawSwitches_ == requiredSwitchMask() && stableSwitches_ == rawSwitches_) {
            result(now, true);
            return true;
        }
        return mode_ == ChallengeMode::Lock4 && warn(elapsed);
    }
    if (phase_ == PanelPhase::Showing) {
        if (elapsed >= PreviewMs) {
            phase_ = PanelPhase::Input;
            phaseStarted_ = now;
            feedback_ = PanelFeedback::None;
            resetUpper(now, upperPressedMask); // Discard even partial preview presses.
            inputArmed_ = upperReleased_ &&
                static_cast<uint32_t>(now - releasedAt_) >= DebounceMs;
            return true;
        }
        return warn(elapsed);
    }
    if (!inputArmed_) {
        if (upperReleased_ && static_cast<uint32_t>(now - releasedAt_) >= DebounceMs) {
            inputArmed_ = true;
            resetUpper(now, 0);
        }
        return false;
    }
    if (rising == 0) return false;

    uint8_t answered = 0;
    uint8_t correct = 0;
    for (uint8_t side = 0; side < 2; ++side) {
        const uint16_t mask = side == 0 ? 0x0007U : 0x0700U;
        if ((rising & mask) == 0) continue;
        const uint8_t sideBit = static_cast<uint8_t>(1U << side);
        answered |= sideBit;
        uint16_t expected = 0;
        for (uint8_t i = 0; i < 3; ++i)
            if (hues_[side * 3U + i] == cueHue_)
                expected = uint16_t(1) << Channels[side * 3U + i];
        if ((upperPressedMask & mask) == expected) correct |= sideBit;
    }
    uint8_t won = correct;
    if (answered == DuelLeft || answered == DuelRight)
        won = correct != 0 ? answered : static_cast<uint8_t>(3U ^ answered);
    result(now, correct != 0, won, static_cast<uint8_t>(3U ^ won));
    return true;
}

} // namespace game