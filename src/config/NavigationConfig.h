#ifndef GYEREK_JATEK_NAVIGATION_CONFIG_H
#define GYEREK_JATEK_NAVIGATION_CONFIG_H

#include <stdint.h>

namespace board {

constexpr uint16_t MenuJoystickCentre = 512;
constexpr uint16_t MenuJoystickEngageDistance = 140;
constexpr uint16_t MenuJoystickReleaseDistance = 80;
constexpr uint32_t MenuJoystickPollMs = 10;
constexpr uint32_t MenuJoystickDebounceMs = 30;
constexpr uint32_t MenuJoystickHoldMs = 500;
constexpr uint32_t MenuJoystickRepeatMs = 300;
constexpr uint8_t MenuDisplayUpdatesPerSecond = 4;

static_assert(MenuJoystickReleaseDistance < MenuJoystickEngageDistance,
              "Joystick hysteresis requires a smaller release threshold");
static_assert(MenuJoystickCentre >= MenuJoystickEngageDistance &&
              MenuJoystickCentre + MenuJoystickEngageDistance <= 1023,
              "Joystick thresholds must fit the Mega ADC range");
static_assert(MenuJoystickRepeatMs >= 1000UL / MenuDisplayUpdatesPerSecond,
              "Display must update at least as fast as held navigation");

} // namespace board

#endif