#include "Application.h"
#include "ModeController.h"
#include "Gyerek_jatek_arduinomega_menu.h"
#include "config/BoardConfig.h"
#include "debug/LedStripTester.h"
#include "debug/SystemChecks.h"
#include "hardware/Diagnostics.h"
#include "hardware/LedDisplay.h"
#include "ui/MenuInput.h"
#include "ui/LcdText.h"
#include <TaskManagerIO.h>
#include <stdio.h>

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
audio::SoundEffects soundEffects(playTone, stopTone);
app::ModeController controller(cyclone, soundEffects);
debug::SystemChecks systemChecks;
debug::LedStripTester ledTest(board::MenuIdleTimeoutSeconds * 1000UL);
bool ledTestScreenDirty = false;
taskid_t updateTask = TASKMGR_INVALIDID;
taskid_t audioTask = TASKMGR_INVALIDID;
uint32_t lastDiagnosticRefresh = 0;
bool initialized = false;
bool debugViewing = false;

bool gameButtonPressed() {
    return digitalRead(board::GameButtonPin) == LOW;
}

void finishLedTest(bool returnToRoot) {
    if (!ledTest.isActive()) return;
    ledTest.stop();
    ledTestScreenDirty = false;
    leds::clear(); // Clear the complete 100-pixel buffer, including the tail.
    renderer.giveBackDisplay();
    if (returnToRoot) menuMgr.resetMenu(true);
}

void printLedTestRow(uint8_t row, const char* text) {
    char line[17];
    ui::blankRow(line, sizeof(line), 16, ' ');
    ui::copyClipped(line, sizeof(line), 0, text, 16);
    lcd.setCursor(0, row);
    lcd.print(line);
}

void renderLedTest(unsigned int, RenderPressMode) {
    if (!ledTest.isActive() || !ledTestScreenDirty) return;
    // Mark clean before LCD writes: they can yield to an input task. A new
    // click during the draw marks it dirty again for the next renderer tick.
    ledTestScreenDirty = false;
    char position[17];
    ledTest.formatPosition(position, sizeof(position));
    lcd.noCursor();
    printLedTestRow(0, position);
    printLedTestRow(1, "Gomb=lep Tart=ki");
}

void drawMode() {
    if (controller.mode() == app::Mode::Cyclone) leds::draw(cyclone);
    else leds::clear();
}

void checkLcd() {
    // Address-only probe, once on Debug entry. The configured Wire timeout
    // bounds a stuck bus; no LCD contents, other devices or input modes change.
    Wire.clearWireTimeoutFlag();
    Wire.beginTransmission(board::LcdI2cAddress);
    const uint8_t result = Wire.endTransmission();
    systemChecks.setLcdResult(result, Wire.getWireTimeoutFlag());
}

void synchronizeDebugView(uint32_t now) {
    // Debug is the only submenu at root, so all nested lists belong to it.
    // Poll navigation rather than installing another tcMenu navigation listener.
    const bool viewing = !menuMgr.getNavigationStore().isShowingRoot();
    if (viewing == debugViewing) return;
    debugViewing = viewing;
    controller.select(viewing ? app::Mode::Debug : app::Mode::Cyclone,
                      now, gameButtonPressed());
    diagnostics::setEnabled(viewing);
    drawMode();
    if (viewing) checkLcd();
    refreshDiagnostics();
}

void update() {
    const uint32_t now = millis();
    menuInput::update(now);
    if (ledTest.isActive() && menuMgr.getCurrentMenu() != &menuBackLedStrip)
        finishLedTest(false);
    synchronizeDebugView(now);
    // Display takeover does not run tcMenu's normal inactivity countdown.
    // Use the same two-minute limit here, renewed only by a step click.
    if (ledTest.hasTimedOut(now)) {
        finishLedTest(true);
        synchronizeDebugView(now);
    }
    diagnostics::setButtonTestEnabled(debugViewing && menuMgr.getCurrentMenu() == &menuButtonDebug, now);
    if (diagnostics::updateButtons(now)) menuButtonDebug.setChanged(true);
    if (controller.update(now, gameButtonPressed())) leds::draw(cyclone);
    diagnostics::update();

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
    // Apply after setupMenu(): setUpdatesPerSecond() resets tcMenu's timeout
    // to 30 seconds. Keep this override outside the generated menu source.
    renderer.setResetIntervalTimeSeconds(board::MenuIdleTimeoutSeconds);
    renderer.menuAltered();
    initialized = true;
    playGame();
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
    systemChecks.setTaskHealth(updateTask != TASKMGR_INVALIDID, audioTask != TASKMGR_INVALIDID);
    Serial.println(F("Gyerek_jatek ready"));
}

void run() {
    taskManager.runLoop();
    if (initialized && updateTask == TASKMGR_INVALIDID) update();
    if (initialized && audioTask == TASKMGR_INVALIDID) updateAudio();
}

void playGame() {
    if (!initialized) return;
    finishLedTest(false);
    if (controller.select(app::Mode::Cyclone, millis(), gameButtonPressed()))
        drawMode();
}

void restart() {
    finishLedTest(false);
    if (initialized && controller.restart(millis(), gameButtonPressed())) drawMode();
}

void startLedTest() {
    if (!initialized || menuMgr.getCurrentMenu() != &menuBackLedStrip) return;
    const uint32_t now = millis();
    synchronizeDebugView(now);
    diagnostics::setButtonTestEnabled(false, now);
    // The menu offset maps raw 0..99 to an actual count of 1..100.
    ledTest.start(static_cast<uint16_t>(menuLedCount.getIntValueIncludingOffset()), now);
    ledTestScreenDirty = true;
    renderer.takeOverDisplay(renderLedTest);
    leds::drawTest(ledTest);
}

bool isLedTestActive() {
    return ledTest.isActive();
}

bool handleLedTestButton(bool held) {
    if (!ledTest.isActive()) return false;
    if (held) {
        finishLedTest(false);
    } else {
        ledTest.step(millis());
        ledTestScreenDirty = true;
        renderer.menuAltered();
        leds::drawTest(ledTest);
    }
    return true; // A step/exit click must not also select a normal menu item.
}

void refreshDiagnostics() {
    char status[16];
    if (ledTest.isActive()) {
        snprintf(status, sizeof(status), "LED teszt");
    } else if (controller.mode() == app::Mode::Debug) {
        snprintf(status, sizeof(status), "Debug");
    } else {
        switch (cyclone.state()) {
            case game::State::Playing:
                snprintf(status, sizeof(status), "Cyclon %u", static_cast<unsigned int>(cyclone.level()));
                break;
            case game::State::Intro: snprintf(status, sizeof(status), "Inditas"); break;
            case game::State::WinFeedback: snprintf(status, sizeof(status), "Talalat"); break;
            case game::State::LossFeedback: snprintf(status, sizeof(status), "Hibas talalat"); break;
            default: snprintf(status, sizeof(status), "Keszenlet"); break;
        }
    }
    menuStatus.setTextValue(status, true);
    if (Wire.getWireTimeoutFlag()) systemChecks.recordI2cTimeout();

    // Render from cached values, not by reading hardware for every LCD row.
    // Only invalidate the visible list, at most four times per second.
    MenuItem* current = menuMgr.getCurrentMenu();
    if (current == &menuButtonDebug) menuButtonDebug.setChanged(true);
    else if (current == &menuDigitalInputs) menuDigitalInputs.setChanged(true);
    else if (current == &menuAnalogInputs) menuAnalogInputs.setChanged(true);
    else if (current == &menuSystemChecks) menuSystemChecks.setChanged(true);
}

void formatSystemCheckName(uint8_t row, char* buffer, size_t capacity) {
    debug::SystemChecks::formatName(row, buffer, capacity);
}

void formatSystemCheckValue(uint8_t row, char* buffer, size_t capacity) {
    systemChecks.formatValue(row, buffer, capacity);
}

} // namespace application