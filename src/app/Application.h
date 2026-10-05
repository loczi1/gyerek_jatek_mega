#ifndef GYEREK_JATEK_APPLICATION_H
#define GYEREK_JATEK_APPLICATION_H

#include <stdint.h>

namespace application {

void begin();
void run();
void selectMode(uint16_t selection);
void restart();
void refreshDiagnostics();
void setInputLogging(bool enabled);
void setDiagnosticOutput(bool enabled);

} // namespace application

#endif