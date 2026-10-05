#include "Application.h"
#include "ModeController.h"
#include "Gyerek_jatek_arduinomega_menu.h"
#include "config/BoardConfig.h"
#include "hardware/Diagnostics.h"
#include "hardware/LedDisplay.h"
#include <TaskManagerIO.h>

namespace application {
namespace {

uint8_t pickTarget() {
    static bool seeded = false;
    if (!seeded) {
        // Mix first-player timing with an optional unconnected analog input.
        // This is game randomness, not a cryptographic entropy source.
        randomSeed(micros() ^ static_cast<unsigned long>(analogRead(board::EntropyInputPin)));
        seeded = true;
    }
    return static_cast<uint8_t>(random(game::MinimumTarget, game::MaximumTarget + 1));
}

void playTone(uint16_t frequency, uint32_t durationMs) {
    tone(board::BuzzerPin, frequency, durationMs);
}

void stopTone() {
    noTone(board::BuzzerPin);
}

game::CycloneGame cyclone(pickTarget);
music::MusicPlayer musicPlayer(playTone, stopTone);
audio::SoundEffects soundEffects(playTone, stopTone);
app::ModeController controller(cyclone, musicPlayer, soundEffects);
taskid_t updateTask = TASKMGR_INVALIDID;
taskid_t audioTask = TASKMGR_INVALIDID;
uint32_t lastDiagnosticRefresh = 0;
uint8_t lastInputMask = 0xff;
bool initialized = false;
bool logInputs = false;

bool gameButtonPressed() {
    return digitalRead(board::GameButtonPin) == LOW;
}

void drawMode() {
    if (controller.mode() == app::Mode::Cyclone) leds::draw(cyclone);
    else leds::clear();
}

void update() {
    const uint32_t now = millis();
    if (controller.update(now, gameButtonPressed())) leds::draw(cyclone);

    if (static_cast<uint32_t>(now - lastDiagnosticRefresh) >= board::DiagnosticRefreshMs) {
        lastDiagnosticRefresh = now;
        refreshDiagnostics();
    }
}

void updateAudio() {
    controller.updateAudio(millis());
}

} // namespace

void begin() {
    if (initialized) return;
    Serial.begin(board::SerialBaud);
    diagnostics::begin();
    leds::begin();
    setupMenu();
    initialized = true;
    selectMode(menuJatek.getCurrentValue());
    setDiagnosticOutput(menuDigOut.getCurrentValue() != 0);
    lastDiagnosticRefresh = millis();
    refreshDiagnostics();

    // Both cooperative tasks are created once, never per selection/result.
    // All callbacks run on taskManager's thread, not from an ISR. Audio keeps
    // its own update cadence without delaying input, LEDs or menu dispatch.
    updateTask = taskManager.scheduleFixedRate(board::AppTickMs, update);
    audioTask = taskManager.scheduleFixedRate(board::AudioTickMs, updateAudio);
    if (updateTask == TASKMGR_INVALIDID)
        Serial.println(F("Game task queue full: using loop fallback"));
    if (audioTask == TASKMGR_INVALIDID)
        Serial.println(F("Audio task queue full: using loop fallback"));
    Serial.println(F("Gyerek_jatek ready"));
}

void run() {
    taskManager.runLoop();
    if (initialized && updateTask == TASKMGR_INVALIDID) update();
    if (initialized && audioTask == TASKMGR_INVALIDID) updateAudio();
}

void selectMode(uint16_t selection) {
    if (!initialized || selection > static_cast<uint16_t>(app::Mode::Music)) return;
    if (controller.select(static_cast<app::Mode>(selection), millis(), gameButtonPressed()))
        drawMode();
}

void restart() {
    if (initialized && controller.restart(millis(), gameButtonPressed())) drawMode();
}

void refreshDiagnostics() {
    char digitalText[12];
    char analogText[10];
    const uint8_t mask = diagnostics::pressedMask();
    diagnostics::formatDigitalInputs(mask, digitalText, sizeof(digitalText));
    diagnostics::formatAnalogInput(analogText, sizeof(analogText));
    // Silent copies avoid recursively invoking these items' own callbacks.
    menuDigIn.setTextValue(digitalText, true);
    menuAnIn.setTextValue(analogText, true);
    if (mask != lastInputMask) {
        menuNewRuntimeList.setChanged(true);
        if (logInputs) {
            Serial.print(F("Inputs LOW: "));
            Serial.println(digitalText);
        }
        lastInputMask = mask;
    }
}

void setInputLogging(bool enabled) {
    logInputs = enabled;
    lastInputMask = 0xff;
    refreshDiagnostics();
}

void setDiagnosticOutput(bool enabled) {
    diagnostics::setOutput(enabled);
}

} // namespace application