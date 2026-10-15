#ifndef GYEREK_JATEK_PIN_CONFIG_H
#define GYEREK_JATEK_PIN_CONFIG_H

#include <stdint.h>
#include "generated/pins.h"

// Portable Mega pin policy, shared by hardware setup and native diagnostics.
namespace board {

constexpr uint8_t LedDataPin = generated::pins::LedDataPin;
constexpr uint8_t BuzzerPin = generated::pins::BuzzerPin;
constexpr uint8_t GameButtonPin = generated::pins::GameButtonPin;
constexpr uint8_t MenuButtonPin = generated::pins::MenuButtonPin;
constexpr uint8_t StatusLedPin = 13;
constexpr uint8_t DigitalPinCount = 54;
constexpr uint8_t AnalogChannelCount = 16;
constexpr uint8_t FirstAnalogGpioPin = DigitalPinCount;
constexpr uint8_t GpioPinCount = DigitalPinCount + AnalogChannelCount;
constexpr uint8_t MenuJoystickChannel = generated::pins::MenuJoystickYChannel;
constexpr uint8_t LcdI2cAddress = generated::pins::LcdI2cAddress;
constexpr uint8_t PotentiometerChannel =
	generated::pins::pinNumber(generated::pins::ControlId::Pot1);

inline bool isWorkbookDigitalInput(uint8_t pin) {
	if (pin >= DigitalPinCount) return false;
	for (uint8_t index = 0;
		 index < static_cast<uint8_t>(generated::pins::ControlId::Count); ++index) {
		const generated::pins::PinDefinition definition = generated::pins::PinTable[index];
		if (definition.type == generated::pins::PinType::DigitalIn &&
			(definition.primaryPin == pin || definition.secondaryPin == pin ||
			 definition.tertiaryPin == pin))
			return true;
	}
	return false;
}

static_assert(GameButtonPin < DigitalPinCount, "Cyclon requires a Mega digital button pin");
static_assert(generated::pins::definition(generated::pins::GameButtonControlId).type ==
			  generated::pins::PinType::DigitalIn, "Cyclon requires a digital input control");
static_assert(PotentiometerChannel < AnalogChannelCount, "Invalid potentiometer ADC channel");

static_assert(generated::pins::LcdI2cSdaPin == 20 && generated::pins::LcdI2cSclPin == 21,
			  "Workbook LCD wiring must stay on Mega I2C pins");

} // namespace board

#endif