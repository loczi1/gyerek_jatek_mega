#include "Application.h"
#include "ModeController.h"
#include "Gyerek_jatek_arduinomega_menu.h"
#include "config/BoardConfig.h"
#include "config/PanelConfig.h"
#include "debug/ButtonTester.h"
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

uint32_t pickBelow(uint32_t bound) {
    static bool seeded = false;
    if (!seeded) {
        // Mix first-player timing with an optional unconnected analog input.
        // This is game randomness, not a cryptographic entropy source.
        randomSeed(micros() ^ static_cast<unsigned long>(analogRead(board::EntropyInputPin)));
        seeded = true;
    }
    return static_cast<uint32_t>(random(static_cast<long>(bound)));
}

uint8_t pickTarget() {
    return static_cast<uint8_t>(game::MinimumTarget +
        pickBelow(game::MaximumTarget - game::MinimumTarget + 1));
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
game::PanelGames panel(pickBelow);
enum class SelectedGame : uint8_t { None, Cyclone, Random, Simon, Flasher };
SelectedGame selectedGame = SelectedGame::None;
MenuItem* gameScope = nullptr;
uint8_t panelInputs[game::PanelInputCount];
uint8_t panelInputCount = 0;
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

void configurePanelInputs() {
    panelInputCount = 0;
    for (uint8_t i = 0; i < game::PanelChannelCount; ++i)
        panelInputs[panelInputCount++] = board::panelInputPin(i);
    // The remaining real workbook buttons/switch contacts also trigger Random.
    // Never include joystick/select, peripheral pins, or unconfigured floating GPIO.
    for (uint8_t pin = 0; pin < board::DigitalPinCount; ++pin) {
        if (!board::isWorkbookDigitalInput(pin) || !debug::canTestButton(pin)) continue;
        bool duplicate = false;
        for (uint8_t i = 0; i < panelInputCount; ++i)
            if (panelInputs[i] == pin) duplicate = true;
        if (!duplicate && panelInputCount < game::PanelInputCount)
            panelInputs[panelInputCount++] = pin;
    }
}

uint32_t panelPressedMask() {
    uint32_t mask = 0;
    for (uint8_t i = 0; i < panelInputCount; ++i) {
        const uint8_t pin = panelInputs[i];
        if (!debug::canTestButton(pin)) continue;
        const uint8_t port = digitalPinToPort(pin);
        if (port == NOT_A_PIN) continue;
        const uint8_t bit = digitalPinToBitMask(pin);
        // Read without digitalRead's PWM side effects; preserve other owners.
        if ((*portModeRegister(port) & bit) == 0 &&
            (*portInputRegister(port) & bit) == 0)
            mask |= uint32_t(1) << i;
    }
    return mask;
}

void stopGames(uint32_t now) {
    panel.select(game::PanelMode::Disabled, now, 0);
    controller.select(app::Mode::Debug, now, gameButtonPressed());
    soundEffects.stop();
    stopTone(); // Random's timed tone is not owned by SoundEffects.
    selectedGame = SelectedGame::None;
    gameScope = nullptr;
    // Leaving a selected game restores the normal Debug/menu idle timeout.
    renderer.setResetIntervalTimeSeconds(board::MenuIdleTimeoutSeconds);
    renderer.menuAltered();
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
    if (selectedGame == SelectedGame::Random || selectedGame == SelectedGame::Simon ||
        selectedGame == SelectedGame::Flasher)
        leds::drawPanel(panel);
    else if (selectedGame == SelectedGame::Cyclone) leds::draw(cyclone);
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
    // Level submenus are not Debug. Runtime lists have their own current pointer.
    const MenuItem* current = menuMgr.getCurrentMenu();
    const bool viewing = current == &menuBackDebug || current == &menuButtonDebug ||
        current == &menuBackLedStrip || current == &menuBackDigitalOutputs ||
        current == &menuDigitalInputs || current == &menuAnalogInputs ||
        current == &menuSystemChecks;
    if (viewing == debugViewing) return;
    debugViewing = viewing;
    stopGames(now);
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
    if (gameScope && menuMgr.getCurrentMenu() != gameScope) {
        stopGames(now);
        drawMode();
    }
    // Display takeover does not run tcMenu's normal inactivity countdown.
    // Use the same two-minute limit here, renewed only by a step click.
    if (ledTest.hasTimedOut(now)) {
        finishLedTest(true);
        synchronizeDebugView(now);
    }
    diagnostics::setButtonTestEnabled(debugViewing && menuMgr.getCurrentMenu() == &menuButtonDebug, now);
    diagnostics::setDigitalOutputTestEnabled(
        debugViewing && menuMgr.getCurrentMenu() == &menuBackDigitalOutputs);
    if (diagnostics::updateButtons(now)) menuButtonDebug.setChanged(true);
    if (selectedGame == SelectedGame::Cyclone) {
        if (controller.update(now, gameButtonPressed())) leds::draw(cyclone);
    } else if (selectedGame == SelectedGame::Random || selectedGame == SelectedGame::Simon ||
               selectedGame == SelectedGame::Flasher) {
        if (panel.update(now, panelPressedMask())) {
            const game::PanelFeedback feedback = panel.takeFeedback();
            if (feedback != game::PanelFeedback::None)
                soundEffects.play(feedback == game::PanelFeedback::Win
                    ? audio::Effect::Win : audio::Effect::Loss, now);
            if (selectedGame == SelectedGame::Random) {
                if (panel.frequency()) playTone(panel.frequency(), 250);
                else stopTone();
            } else if (feedback == game::PanelFeedback::None) {
                soundEffects.stop();
            }
            leds::drawPanel(panel);
        }
    }
    diagnostics::update();

    if (static_cast<uint32_t>(now - lastDiagnosticRefresh) >= board::DiagnosticRefreshMs) {
        lastDiagnosticRefresh = now;
        refreshDiagnostics();
    }
}

void updateAudio() {
    if (selectedGame == SelectedGame::Simon || selectedGame == SelectedGame::Flasher)
        soundEffects.update(millis());
    else controller.updateAudio(millis());
}

} // namespace

void begin() {
    if (initialized) return;
    Serial.begin(board::SerialBaud);
    diagnostics::begin();
    configurePanelInputs();
    leds::begin();
    setupMenu();
    // Apply after setupMenu(): setUpdatesPerSecond() resets tcMenu's timeout
    // to 30 seconds. Keep this override outside the generated menu source.
    renderer.setResetIntervalTimeSeconds(board::MenuIdleTimeoutSeconds);
    renderer.menuAltered();
    initialized = true;
    stopGames(millis());
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
    diagnostics::setButtonTestEnabled(false, millis());
    diagnostics::setDigitalOutputTestEnabled(false);
    panel.select(game::PanelMode::Disabled, millis(), 0);
    selectedGame = SelectedGame::Cyclone;
    gameScope = &menuBackLevel2;
    renderer.turnOffResetLogic();
    renderer.menuAltered();
    if (controller.select(app::Mode::Cyclone, millis(), gameButtonPressed()))
        drawMode();
}

void selectPanelGame(SelectedGame selection, game::PanelMode mode, MenuItem* scope) {
    if (!initialized) return;
    finishLedTest(false);
    const uint32_t now = millis();
    diagnostics::setButtonTestEnabled(false, now);
    diagnostics::setDigitalOutputTestEnabled(false);
    stopGames(now);
    selectedGame = selection;
    gameScope = scope;
    renderer.turnOffResetLogic();
    renderer.menuAltered();
    panel.select(mode, now, panelPressedMask());
    drawMode();
}

void playRandom() {
    selectPanelGame(SelectedGame::Random, game::PanelMode::Random, &menuBackLevel1);
}

void playSimon() {
    selectPanelGame(SelectedGame::Simon, game::PanelMode::Simon, &menuBackLevel2);
}

void playFlasher() {
    selectPanelGame(SelectedGame::Flasher, game::PanelMode::Flasher, &menuBackLevel2);
}

void restart() {
    finishLedTest(false);
    if (!initialized) return;
    stopGames(millis());
    drawMode();
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
    } else if (debugViewing) {
        snprintf(status, sizeof(status), "Debug");
    } else if (selectedGame == SelectedGame::Random) {
        snprintf(status, sizeof(status), "Random");
    } else if (selectedGame == SelectedGame::Flasher) {
        snprintf(status, sizeof(status), "Sima villogo");
    } else if (selectedGame == SelectedGame::Simon) {
        snprintf(status, sizeof(status), "Simon %u/%u",
            static_cast<unsigned int>(panel.inputIndex()),
            static_cast<unsigned int>(panel.sequenceLength()));
    } else if (selectedGame == SelectedGame::None) {
        snprintf(status, sizeof(status), "Valassz jatekot");
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
    char digitalOutputStatus[16];
    diagnostics::formatDigitalOutputStatus(digitalOutputStatus, sizeof(digitalOutputStatus));
    menuDigitalOutputStatus.setTextValue(digitalOutputStatus, true);
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