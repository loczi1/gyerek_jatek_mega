#ifndef GYEREK_JATEK_MELODY_H
#define GYEREK_JATEK_MELODY_H

#include <stdint.h>

namespace music {

struct Note {
    uint16_t frequency;
    int8_t divider;
};

uint16_t melodyNoteCount();
Note melodyNote(uint16_t index);
uint32_t noteDurationMs(int8_t divider, uint16_t tempo = 70);

} // namespace music

#endif