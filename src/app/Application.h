#ifndef GYEREK_JATEK_APPLICATION_H
#define GYEREK_JATEK_APPLICATION_H

#include <stddef.h>
#include <stdint.h>

namespace application {

void begin();
void run();
void playGame();
void playRandom();
void playRandom2();
void playSimon();
void playSorrend3();
void playSorrend4();
void playSorrend5();
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