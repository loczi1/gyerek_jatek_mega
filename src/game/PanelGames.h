#ifndef GYEREK_JATEK_PANEL_GAMES_H
#define GYEREK_JATEK_PANEL_GAMES_H

#include "game/DebouncedButton.h"
#include <stdint.h>

namespace game {

constexpr uint8_t PanelChannelCount = 11;
constexpr uint8_t PanelInputCount = 32;
constexpr uint8_t PanelNoChannel = 255;
constexpr uint8_t PanelSequenceCapacity = 32;

enum class PanelMode : uint8_t { Disabled, Random, Simon, Flasher };
enum class PanelPhase : uint8_t {
    Disabled, Ready, Showing, Gap, Input, Hit, RoundWin, Loss
};
enum class PanelFeedback : uint8_t { None, Win, Loss };

// Logical pressed bits, not GPIO numbers or polarity. Adapter maps bits 0..10
// to BTN01..03, BTN07..14, in that order. Random accepts all 32 bits;
// adapter supplies ONLY safe workbook DigitalIn contacts (unused bits zero).
// No hardware, heap, scheduler, or timing source is used by this class.
class PanelGames {
public:
    explicit PanelGames(uint32_t (*randomBelow)(uint32_t) = nullptr);

    // Always resets, even when selecting the current mode. Held inputs must
    // be released and debounced before they can generate another press.
    void select(PanelMode mode, uint32_t now, uint32_t pressedMask);
    // True for a frame/status change, including a new Random hit with the
    // same output values. At most one phase transition per call; no catch-up.
    bool update(uint32_t now, uint32_t pressedMask);
    PanelFeedback takeFeedback();

    PanelMode mode() const { return mode_; }
    PanelPhase phase() const { return phase_; }
    uint8_t litChannel() const { return litChannel_; }
    // FastLED-compatible hue: red=0, green=96. No light when channel=255.
    uint8_t hue() const { return hue_; }
    // Random tone only: 220..1760 Hz while Hit, otherwise zero. Adapter
    // starts/restarts its 250 ms tone on each Random Hit frame change.
    uint16_t frequency() const { return frequency_; }
    uint8_t sequenceLength() const { return sequenceLength_; }
    uint8_t inputIndex() const { return inputIndex_; }
    uint32_t enabledMask() const;

private:
    uint32_t (*randomBelow_)(uint32_t);
    DebouncedButton buttons_[PanelInputCount];
    uint8_t sequence_[PanelSequenceCapacity] = {};
    PanelMode mode_ = PanelMode::Disabled;
    PanelPhase phase_ = PanelPhase::Disabled;
    PanelFeedback feedback_ = PanelFeedback::None;
    uint32_t phaseStarted_ = 0;
    uint8_t sequenceLength_ = 0;
    uint8_t inputIndex_ = 0;
    uint8_t showingIndex_ = 0;
    uint8_t litChannel_ = PanelNoChannel;
    uint8_t hue_ = 0;
    uint16_t frequency_ = 0;

    uint32_t pick(uint32_t bound);
    void resetButtons(uint32_t now, uint32_t pressedMask);
    void enter(PanelPhase phase, uint32_t now);
    void ready(uint32_t now, uint32_t pressedMask);
    void appendRound(uint32_t now);
    void show(uint32_t now);
    void lose(uint32_t now, uint8_t channel);
};

} // namespace game

#endif