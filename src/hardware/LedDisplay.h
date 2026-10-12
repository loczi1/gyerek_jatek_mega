#ifndef GYEREK_JATEK_LED_DISPLAY_H
#define GYEREK_JATEK_LED_DISPLAY_H

#include "game/CycloneGame.h"
#include "debug/LedStripTester.h"

namespace leds {

void begin();
void clear();
void draw(const game::CycloneGame& game);
void drawTest(const debug::LedStripTester& test);

} // namespace leds

#endif