#include "game/PanelGames.h"

namespace game {
namespace {
constexpr uint32_t SimonMask = (uint32_t(1) << PanelChannelCount) - 1U;
constexpr uint32_t InitialPauseMs = 500;
constexpr uint32_t ShowingMs = 500;
constexpr uint32_t GapMs = 200;
constexpr uint32_t HitMs = 540;
constexpr uint32_t LossMs = 740;
constexpr uint32_t RandomHitMs = 250;

uint8_t firstChannel(uint32_t mask) {
    for (uint8_t i = 0; i < PanelInputCount; ++i) {
        if (mask & (uint32_t(1) << i)) return i;
    }
    return PanelNoChannel;
}
} // namespace

PanelGames::PanelGames(uint32_t (*randomBelow)(uint32_t))
    : randomBelow_(randomBelow) {}

uint32_t PanelGames::enabledMask() const {
    return (mode_ == PanelMode::Random || mode_ == PanelMode::Random2) ? UINT32_MAX :
        ((mode_ == PanelMode::Simon || mode_ == PanelMode::Flasher || isSearch()) ? SimonMask : 0);
}

uint32_t PanelGames::pick(uint32_t bound) {
    // Also bounds a faulty callback; null gives deterministic playable output.
    return randomBelow_ ? randomBelow_(bound) % bound : 0;
}

void PanelGames::resetButtons(uint32_t now, uint32_t pressedMask) {
    for (uint8_t i = 0; i < PanelInputCount; ++i) {
        buttons_[i].reset(now, (pressedMask & (uint32_t(1) << i)) != 0);
    }
}

void PanelGames::enter(PanelPhase phase, uint32_t now) {
    phase_ = phase;
    phaseStarted_ = now;
    litChannel_ = PanelNoChannel;
    hue_ = 0;
    frequency_ = 0;
    lampInputMask_ = 0;
}

void PanelGames::ready(uint32_t now, uint32_t pressedMask) {
    enter(PanelPhase::Ready, now);
    sequenceLength_ = 0;
    inputIndex_ = 0;
    showingIndex_ = 0;
    distractors_ = 0;
    secondWarning_ = false;
    resetButtons(now, pressedMask);
}

void PanelGames::select(PanelMode mode, uint32_t now, uint32_t pressedMask) {
    mode_ = (mode == PanelMode::Random || mode == PanelMode::Simon ||
             mode == PanelMode::Flasher || mode == PanelMode::Random2 ||
             mode == PanelMode::Sorrend4 || mode == PanelMode::Sorrend5)
        ? mode : PanelMode::Disabled;
    feedback_ = PanelFeedback::None;
    ready(now, pressedMask);
    if (mode_ == PanelMode::Disabled) enter(PanelPhase::Disabled, now);
}

void PanelGames::appendRound(uint32_t now) {
    // Flasher replaces its single target; it never grows or replays a sequence.
    if (mode_ == PanelMode::Flasher || isSearch()) sequenceLength_ = 0;
    sequence_[sequenceLength_++] = static_cast<uint8_t>(pick(PanelChannelCount));
    distractors_ = 0;
    if (isSearch()) {
        // Select without replacement in bounded time, even with a constant RNG.
        uint16_t available = static_cast<uint16_t>(SimonMask & ~(uint32_t(1) << sequence_[0]));
        const uint8_t count = static_cast<uint8_t>(3U + pick(8));
        for (uint8_t i = 0; i < count; ++i) {
            uint8_t rank = static_cast<uint8_t>(pick(PanelChannelCount - 1U - i));
            for (uint8_t channel = 0; channel < PanelChannelCount; ++channel) {
                const uint16_t bit = uint16_t(1) << channel;
                if (!(available & bit)) continue;
                if (rank != 0) { --rank; continue; }
                distractors_ |= bit;
                available &= static_cast<uint16_t>(~bit);
                break;
            }
        }
        distractorHue_ = static_cast<uint8_t>(pick(256));
    }
    inputIndex_ = 0;
    showingIndex_ = 0;
    // Gap with showingIndex=0 denotes the initial 500 ms dark pause.
    enter(PanelPhase::Gap, now);
}

void PanelGames::show(uint32_t now) {
    enter(PanelPhase::Showing, now);
    litChannel_ = sequence_[showingIndex_];
    hue_ = static_cast<uint8_t>(1U + litChannel_ * 20U);
}

void PanelGames::lose(uint32_t now, uint8_t channel) {
    enter(PanelPhase::Loss, now);
    litChannel_ = channel;
    feedback_ = PanelFeedback::Loss;
}

PanelFeedback PanelGames::takeFeedback() {
    const PanelFeedback result = feedback_;
    feedback_ = PanelFeedback::None;
    return result;
}

bool PanelGames::update(uint32_t now, uint32_t pressedMask) {
    if (mode_ == PanelMode::Disabled) return false;
    pressedMask &= enabledMask();
    uint32_t rising = 0;
    for (uint8_t i = 0; i < PanelInputCount; ++i) {
        if (buttons_[i].update(now, (pressedMask & (uint32_t(1) << i)) != 0)) {
            rising |= uint32_t(1) << i;
        }
    }
    const uint32_t elapsed = static_cast<uint32_t>(now - phaseStarted_);
    if (mode_ == PanelMode::Random2) {
        if (rising != 0) {
            enter(PanelPhase::Hit, now);
            lampInputMask_ = rising;
            beatStarted_ = now;
            discoStart_ = static_cast<uint8_t>(pick(70));
            hue_ = static_cast<uint8_t>(pick(256));
            feedback_ = PanelFeedback::Melody;
            return true;
        }
        if (phase_ != PanelPhase::Hit) return false;
        if (elapsed >= Random2DurationMs) {
            enter(PanelPhase::Ready, now);
            return true;
        }
        if (static_cast<uint32_t>(now - beatStarted_) >= Random2BeatMs) {
            beatStarted_ = now; // No catch-up burst after a delayed tick.
            discoStart_ = static_cast<uint8_t>(pick(70));
            hue_ = static_cast<uint8_t>(pick(256));
            return true;
        }
        return false;
    }
    if (mode_ == PanelMode::Random) {
        if (rising != 0) {
            enter(PanelPhase::Hit, now);
            litChannel_ = static_cast<uint8_t>(pick(PanelChannelCount));
            hue_ = static_cast<uint8_t>(1U + pick(255));
            frequency_ = static_cast<uint16_t>(220U + pick(1541));
            return true;
        }
        if (phase_ == PanelPhase::Hit && elapsed >= RandomHitMs) {
            enter(PanelPhase::Ready, now);
            return true;
        }
        return false;
    }

    switch (phase_) {
    case PanelPhase::Ready:
        if (!rising) return false;
        appendRound(now); // Start press is consumed, never an answer.
        return true;
    case PanelPhase::Showing:
        if (elapsed < ShowingMs) return false;
        ++showingIndex_;
        enter(PanelPhase::Gap, now);
        return true;
    case PanelPhase::Gap:
        if (elapsed < (showingIndex_ == 0 ? InitialPauseMs : GapMs)) return false;
        if (showingIndex_ < sequenceLength_) {
            show(now);
        } else {
            enter(PanelPhase::Input, now);
            if (mode_ == PanelMode::Flasher || isSearch()) {
                // Keep the current target visible while waiting: no memory task.
                litChannel_ = sequence_[0];
                hue_ = static_cast<uint8_t>(1U + litChannel_ * 20U);
            }
            // Discard both stable and partially debounced playback presses.
            resetButtons(now, pressedMask);
            secondWarning_ = false;
            if (mode_ == PanelMode::Sorrend5) feedback_ = PanelFeedback::Warning;
        }
        return true;
    case PanelPhase::Input:
        // Timeout wins over a press arriving at or after the deadline.
        if (elapsed >= inputTimeoutMs()) {
            lose(now, sequence_[inputIndex_]);
            return true;
        }
        if (!rising) {
            if (mode_ == PanelMode::Sorrend5 && !secondWarning_ && elapsed >= 1000U) {
                secondWarning_ = true;
                feedback_ = PanelFeedback::Warning;
                return true;
            }
            return false;
        }
        if ((pressedMask & (pressedMask - 1U)) != 0 ||
            (rising & (rising - 1U)) != 0 ||
            firstChannel(rising) != sequence_[inputIndex_]) {
            lose(now, firstChannel(rising));
            return true;
        }
        ++inputIndex_;
        enter(inputIndex_ == sequenceLength_ ? PanelPhase::RoundWin : PanelPhase::Hit, now);
        litChannel_ = firstChannel(rising);
        hue_ = 96;
        feedback_ = PanelFeedback::Win;
        return true;
    case PanelPhase::Hit:
        if (elapsed < HitMs) return false;
        enter(PanelPhase::Input, now);
        resetButtons(now, pressedMask);
        return true;
    case PanelPhase::RoundWin:
        if (elapsed < HitMs) return false;
        if (sequenceLength_ == PanelSequenceCapacity) ready(now, pressedMask);
        else appendRound(now);
        return true;
    case PanelPhase::Loss:
        if (elapsed < LossMs) return false;
        ready(now, pressedMask);
        return true;
    default:
        return false;
    }
}

} // namespace game