#ifndef GYEREK_JATEK_GAME_LAMPS_H
#define GYEREK_JATEK_GAME_LAMPS_H
#include <stdint.h>

namespace gameLamps {
void setEnabled(bool enabled);
void draw(uint32_t inputMask, const uint8_t* inputs, uint8_t count);
}
#endif