#include "SystemChecks.h"
#include <stdio.h>

namespace debug {

void SystemChecks::setTaskHealth(bool gameScheduled, bool audioScheduled) {
    gameTask_ = gameScheduled ? TaskHealth::Scheduled : TaskHealth::Fallback;
    audioTask_ = audioScheduled ? TaskHealth::Scheduled : TaskHealth::Fallback;
}

void SystemChecks::setLcdResult(uint8_t transmissionResult, bool timedOut) {
    if (timedOut || transmissionResult == 5) lcdHealth_ = LcdHealth::Timeout;
    else if (transmissionResult == 0) lcdHealth_ = LcdHealth::Ready;
    else if (transmissionResult == 2 || transmissionResult == 3) lcdHealth_ = LcdHealth::Missing;
    else lcdHealth_ = LcdHealth::BusError;
}

void SystemChecks::formatName(uint8_t row, char* buffer, size_t capacity) {
    if (!buffer || capacity == 0) return;
    const char* name = row == 0 ? "Jatek task" : (row == 1 ? "Hang task" :
        (row == 2 ? "LCD 0x27" : ""));
    snprintf(buffer, capacity, "%s", name);
}

void SystemChecks::formatValue(uint8_t row, char* buffer, size_t capacity) const {
    if (!buffer || capacity == 0) return;
    const char* value = "";
    if (row < 2) {
        const TaskHealth health = row == 0 ? gameTask_ : audioTask_;
        value = health == TaskHealth::Scheduled ? "OK" :
            (health == TaskHealth::Fallback ? "FALLBACK" : "---");
    } else if (row == 2) {
        switch (lcdHealth_) {
            case LcdHealth::Pending: value = "---"; break;
            case LcdHealth::Ready: value = "OK"; break;
            case LcdHealth::Missing: value = "NINCS"; break;
            case LcdHealth::BusError: value = "I2C HIBA"; break;
            case LcdHealth::Timeout: value = "TIMEOUT"; break;
        }
    }
    snprintf(buffer, capacity, "%s", value);
}

} // namespace debug