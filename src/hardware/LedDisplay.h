#ifndef GYEREK_JATEK_LED_DISPLAY_H
#define GYEREK_JATEK_LED_DISPLAY_H

#include "game/CycloneGame.h"

namespace leds {

void begin();
void clear();
void draw(const game::CycloneGame& game);

} // namespace leds

#endif