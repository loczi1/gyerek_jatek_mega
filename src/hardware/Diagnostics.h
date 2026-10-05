#ifndef GYEREK_JATEK_DIAGNOSTICS_H
#define GYEREK_JATEK_DIAGNOSTICS_H

#include <stddef.h>
#include <stdint.h>

namespace diagnostics {

void begin();
uint8_t pressedMask();
void formatDigitalInputs(uint8_t mask, char* buffer, size_t capacity);
void formatAnalogInput(char* buffer, size_t capacity);
void setOutput(bool enabled);

} // namespace diagnostics

#endif