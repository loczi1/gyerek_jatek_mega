#ifndef GYEREK_JATEK_APPLICATION_H
#define GYEREK_JATEK_APPLICATION_H

#include <stddef.h>
#include <stdint.h>

namespace application {

void begin();
void run();
void playGame();
void playRandom();
void playSimon();
void playFlasher();
void restart();
void refreshDiagnostics();
void startLedTest();
bool isLedTestActive();
// Returns true when the test consumes a joystick-button click/hold.
bool handleLedTestButton(bool held);
void formatSystemCheckName(uint8_t row, char* buffer, size_t capacity);
void formatSystemCheckValue(uint8_t row, char* buffer, size_t capacity);

} // namespace application

#endif