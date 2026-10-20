#ifndef GYEREK_JATEK_PAIRED_LAMP_OUTPUTS_H
#define GYEREK_JATEK_PAIRED_LAMP_OUTPUTS_H

#include "debug/ButtonTester.h"

namespace game {

// Portable scoped GPIO ownership. Existing outputs are never borrowed;
// cancellation restores only unchanged, still-owned lamp pins.
class PairedLampOutputs {
public:
    using ReadMode = debug::ButtonTester::ReadMode;
    using ReadHigh = bool (*)(uint8_t);
    using Write = void (*)(uint8_t, bool);
    using Restore = void (*)(uint8_t, bool);
    PairedLampOutputs(ReadMode mode, ReadHigh high, Write write, Restore restore)
        : mode_(mode), high_(high), write_(write), restore_(restore) {}
    void setEnabled(bool enabled);
    void draw(uint32_t inputMask, const uint8_t* inputs, uint8_t count);

private:
    ReadMode mode_;
    ReadHigh high_;
    Write write_;
    Restore restore_;
    debug::PinSet owned_;
    debug::PinSet pullups_;
    debug::PinSet lastHigh_;
    bool enabled_ = false;
    bool stillOwned(uint8_t pin);
};

} // namespace game
#endif