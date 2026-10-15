#ifndef GYEREK_JATEK_PANEL_CONFIG_H
#define GYEREK_JATEK_PANEL_CONFIG_H

#include "config/PinConfig.h"
#include "game/PanelGames.h"

namespace board {

// Input order: left three, bottom five BTN07..11, right three.
constexpr generated::pins::ControlId PanelControls[] = {
    generated::pins::ControlId::Btn01LeftBlue1,
    generated::pins::ControlId::Btn02LeftBlue2,
    generated::pins::ControlId::Btn03LeftBlue3,
    generated::pins::ControlId::Btn07Down51,
    generated::pins::ControlId::Btn08Down52,
    generated::pins::ControlId::Btn09Down53,
    generated::pins::ControlId::Btn10Down54,
    generated::pins::ControlId::Btn11Down55,
    generated::pins::ControlId::Btn12RightColor1,
    generated::pins::ControlId::Btn13RightColor2,
    generated::pins::ControlId::Btn14RightColor3,
};
static_assert(sizeof(PanelControls) / sizeof(PanelControls[0]) == game::PanelChannelCount,
              "Simon channel count mismatch");

inline uint8_t panelInputPin(uint8_t channel) {
    return channel < game::PanelChannelCount
        ? generated::pins::pinNumber(PanelControls[channel]) : generated::pins::InvalidPin;
}

// Workbook lamp identity and 1-based pixel are separate from the D4 data pin.
inline uint8_t panelStripPixel(uint8_t channel) {
    if (channel >= game::PanelChannelCount) return 0;
    // Physical report: the bottom strip runs opposite to its button row.
    // Reverse only its lamp association, not inputs or the workbook export:
    // BTN07..11 -> 75,74,73,72,71. Upper groups stay unchanged.
    const uint8_t lampChannel = channel >= 3 && channel <= 7 ? 10 - channel : channel;
    for (uint8_t i = 0; i < generated::pins::LampPairCount; ++i) {
        const auto pair = generated::pins::LampPairs[i];
        if (pair.inputControl == PanelControls[lampChannel])
            return generated::pins::definition(pair.lampControl).stripPixel;
    }
    return 0;
}

} // namespace board
#endif