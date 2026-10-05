#include "Melody.h"

#ifdef ARDUINO_ARCH_AVR
#include <avr/pgmspace.h>
#define MELODY_STORAGE PROGMEM
#else
#define MELODY_STORAGE
#endif

namespace music {
namespace {

enum Pitch : uint16_t {
    D4 = 294, E4 = 330, F4 = 349, FS4 = 370, G4 = 392,
    A4 = 440, AS4 = 466, C5 = 523, D5 = 587, E5 = 659, F5 = 698, DS5 = 622
};

// Greensleeves arrangement retained from the original project.
// Source: https://github.com/robsoncouto/arduino-songs/blob/master/greensleeves/greensleeves.ino
// Score: https://musescore.com/user/168402/scores/1396946 (Alexander Trompoukis).
// Store the phrase once in flash; the player reads it twice, as before.
const Note Melody[] MELODY_STORAGE = {
    {G4, 8},
    {AS4, 4}, {C5, 8}, {D5, -8}, {DS5, 16}, {D5, 8},
    {C5, 4}, {A4, 8}, {F4, -8}, {G4, 16}, {A4, 8},
    {AS4, 4}, {G4, 8}, {G4, -8}, {FS4, 16}, {G4, 8},
    {A4, 4}, {FS4, 8}, {D4, 4}, {G4, 8},

    {AS4, 4}, {C5, 8}, {D5, -8}, {DS5, 16}, {D5, 8},
    {C5, 4}, {A4, 8}, {F4, -8}, {G4, 16}, {A4, 8},
    {AS4, -8}, {A4, 16}, {G4, 8}, {FS4, -8}, {E4, 16}, {FS4, 8},
    {G4, -2}, {F5, 2}, {E5, 16}, {D5, 8},

    {C5, 4}, {A4, 8}, {F4, -8}, {G4, 16}, {A4, 8},
    {AS4, 4}, {G4, 8}, {G4, -8}, {FS4, 16}, {G4, 8},
    {A4, 4}, {FS4, 8}, {D4, 4},
    {F5, 2}, {E5, 16}, {D5, 8},
    {C5, 4}, {A4, 8}, {F4, -8}, {G4, 16}, {A4, 8},
    {AS4, -8}, {A4, 16}, {G4, 8}, {FS4, -8}, {E4, 16}, {FS4, 8},
    {G4, -2}
};

constexpr uint16_t PhraseNoteCount = sizeof(Melody) / sizeof(Melody[0]);
constexpr uint8_t Repetitions = 2;

} // namespace

uint16_t melodyNoteCount() {
    return PhraseNoteCount * Repetitions;
}

Note melodyNote(uint16_t index) {
    if (index >= melodyNoteCount()) return {0, 0};
    const uint16_t phraseIndex = index % PhraseNoteCount;
#ifdef ARDUINO_ARCH_AVR
    return {
        pgm_read_word(&Melody[phraseIndex].frequency),
        static_cast<int8_t>(pgm_read_byte(&Melody[phraseIndex].divider))
    };
#else
    return Melody[phraseIndex];
#endif
}

uint32_t noteDurationMs(int8_t divider, uint16_t tempo) {
    if (divider == 0 || tempo == 0) return 0;
    const uint32_t wholeNote = (60000UL * 4UL) / tempo;
    const int16_t signedDivider = divider;
    const uint16_t divisor = static_cast<uint16_t>(
        signedDivider < 0 ? -signedDivider : signedDivider);
    const uint32_t duration = wholeNote / divisor;
    return divider < 0 ? duration + duration / 2 : duration;
}

} // namespace music