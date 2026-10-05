#include "MusicPlayer.h"
#include "Melody.h"

namespace music {

void MusicPlayer::stop() {
    if (enabled_ && stopTone_) stopTone_();
    enabled_ = false;
    noteDuration_ = 0;
}

void MusicPlayer::setEnabled(bool enabled, uint32_t now) {
    if (enabled == enabled_) return;
    if (enabled) restart(now);
    else stop();
}

void MusicPlayer::restart(uint32_t now) {
    stop();
    currentNote_ = 0;
    enabled_ = true;
    startNote(now);
}

void MusicPlayer::startNote(uint32_t now) {
    const Note note = melodyNote(currentNote_);
    noteDuration_ = noteDurationMs(note.divider);
    noteStartedAt_ = now;
    if (note.frequency != 0 && noteDuration_ != 0) {
        if (playTone_) playTone_(note.frequency, noteDuration_ * 9UL / 10UL);
    } else if (stopTone_) {
        stopTone_();
    }
}

void MusicPlayer::update(uint32_t now) {
    if (!enabled_ || static_cast<uint32_t>(now - noteStartedAt_) < noteDuration_)
        return;

    if (stopTone_) stopTone_();
    ++currentNote_;
    if (currentNote_ >= melodyNoteCount()) {
        enabled_ = false;
        noteDuration_ = 0;
        return; // Finishing music never changes the game state.
    }
    startNote(now);
}

} // namespace music