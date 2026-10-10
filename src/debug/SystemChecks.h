#ifndef GYEREK_JATEK_SYSTEM_CHECKS_H
#define GYEREK_JATEK_SYSTEM_CHECKS_H

#include <stddef.h>
#include <stdint.h>

namespace debug {

constexpr uint8_t SystemCheckCount = 3;
enum class TaskHealth : uint8_t { Pending, Scheduled, Fallback };
enum class LcdHealth : uint8_t { Pending, Ready, Missing, BusError, Timeout };

class SystemChecks {
public:
    void setTaskHealth(bool gameScheduled, bool audioScheduled);
    void setLcdResult(uint8_t transmissionResult, bool timedOut);
    void recordI2cTimeout() { lcdHealth_ = LcdHealth::Timeout; }
    static void formatName(uint8_t row, char* buffer, size_t capacity);
    void formatValue(uint8_t row, char* buffer, size_t capacity) const;

private:
    TaskHealth gameTask_ = TaskHealth::Pending;
    TaskHealth audioTask_ = TaskHealth::Pending;
    LcdHealth lcdHealth_ = LcdHealth::Pending;
};

} // namespace debug

#endif