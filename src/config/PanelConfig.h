#ifndef GYEREK_JATEK_PANEL_CONFIG_H
#define GYEREK_JATEK_PANEL_CONFIG_H

#include "config/PinConfig.h"
#include "game/PanelGames.h"

namespace board {

constexpr generated::pins::ControlId MetalControls[] = {
    generated::pins::ControlId::Sw01Metal1,
    generated::pins::ControlId::Sw02Metal2,
    generated::pins::ControlId::Sw03Metal3,
    generated::pins::ControlId::Sw04Metal4,
    generated::pins::ControlId::Sw05Metal5,
};

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

// Random2 resolves every safe input through workbook pairs. Keep the physical
// lower-row correction above authoritative for the eleven panel buttons.
inline uint8_t pairedStripPixel(uint8_t inputPin) {
    for (uint8_t i = 0; i < game::PanelChannelCount; ++i)
        if (panelInputPin(i) == inputPin) return panelStripPixel(i);
    for (const auto& pair : generated::pins::LampPairs) {
        const auto input = generated::pins::definition(pair.inputControl);
        const auto lamp = generated::pins::definition(pair.lampControl);
        if ((input.primaryPin == inputPin || input.secondaryPin == inputPin ||
             input.tertiaryPin == inputPin) && inputPin != generated::pins::InvalidPin &&
            lamp.activeLevel == generated::pins::ActiveLevel::Ws2812b)
            return lamp.stripPixel;
    }
    return 0;
}

inline uint8_t pairedGpioLamp(uint8_t inputPin) {
    if (inputPin == generated::pins::InvalidPin) return generated::pins::InvalidPin;
    for (const auto& pair : generated::pins::LampPairs) {
        const auto input = generated::pins::definition(pair.inputControl);
        const auto lamp = generated::pins::definition(pair.lampControl);
        if ((input.primaryPin == inputPin || input.secondaryPin == inputPin ||
             input.tertiaryPin == inputPin) &&
            lamp.type == generated::pins::PinType::DigitalOut &&
            (lamp.activeLevel == generated::pins::ActiveLevel::High ||
             lamp.activeLevel == generated::pins::ActiveLevel::Low))
            return lamp.primaryPin;
    }
    return generated::pins::InvalidPin;
}

} // namespace board
#endif