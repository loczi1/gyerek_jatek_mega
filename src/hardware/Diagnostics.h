#ifndef GYEREK_JATEK_DIAGNOSTICS_H
#define GYEREK_JATEK_DIAGNOSTICS_H

#include <stddef.h>
#include <stdint.h>

namespace diagnostics {

void begin();
void setEnabled(bool enabled);
void update();
void setButtonTestEnabled(bool enabled, uint32_t now);
bool updateButtons(uint32_t now);
void setDigitalOutputTestEnabled(bool enabled);
void selectNextDigitalOutput();
void selectPreviousDigitalOutput();
void setSelectedDigitalOutput(bool high);
void formatDigitalOutputStatus(char* buffer, size_t capacity);
void formatButtonName(uint8_t row, char* buffer, size_t capacity);
void formatButtonValue(uint8_t row, char* buffer, size_t capacity);
void formatDigitalName(uint8_t pin, char* buffer, size_t capacity);
void formatDigitalValue(uint8_t pin, char* buffer, size_t capacity);
void formatAnalogName(uint8_t channel, char* buffer, size_t capacity);
void formatAnalogValue(uint8_t channel, char* buffer, size_t capacity);

} // namespace diagnostics

#endif