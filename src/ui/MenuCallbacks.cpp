#include "Gyerek_jatek_arduinomega_menu.h"
#include "app/Application.h"
#include "config/PinConfig.h"
#include "debug/ButtonTester.h"
#include "debug/SystemChecks.h"
#include "hardware/Diagnostics.h"

void CALLBACK_FUNCTION Jatek(int) {
    application::playGame();
}

void CALLBACK_FUNCTION RandomGame(int) {
    application::playRandom();
}

void CALLBACK_FUNCTION RandomGame2(int) {
    application::playRandom2();
}

void CALLBACK_FUNCTION SimonGame(int) {
    application::playSimon();
}

void CALLBACK_FUNCTION SorrendGame3(int) {
    application::playSorrend3();
}

void CALLBACK_FUNCTION SorrendGame4(int) {
    application::playSorrend4();
}

void CALLBACK_FUNCTION SorrendGame5(int) {
    application::playSorrend5();
}

void CALLBACK_FUNCTION FlasherGame(int) {
    application::playFlasher();
}

void CALLBACK_FUNCTION Restart(int) {
    application::restart();
}

void CALLBACK_FUNCTION StartLedTest(int) {
    application::startLedTest();
}

void CALLBACK_FUNCTION NextDigitalOutputPin(int) {
    diagnostics::setDigitalOutputTestEnabled(true);
    diagnostics::selectNextDigitalOutput();
    application::refreshDiagnostics();
}

void CALLBACK_FUNCTION PreviousDigitalOutputPin(int) {
    diagnostics::setDigitalOutputTestEnabled(true);
    diagnostics::selectPreviousDigitalOutput();
    application::refreshDiagnostics();
}

void CALLBACK_FUNCTION SetDigitalOutputLow(int) {
    diagnostics::setDigitalOutputTestEnabled(true);
    diagnostics::setSelectedDigitalOutput(false);
    application::refreshDiagnostics();
}

void CALLBACK_FUNCTION SetDigitalOutputHigh(int) {
    diagnostics::setDigitalOutputTestEnabled(true);
    diagnostics::setSelectedDigitalOutput(true);
    application::refreshDiagnostics();
}

namespace {

using FormatRow = void (*)(uint8_t row, char* buffer, size_t capacity);

int renderReadOnlyList(RuntimeMenuItem* item, uint8_t row, RenderFnMode mode,
        char* buffer, int bufferSize, uint8_t rows, FormatRow formatName, FormatRow formatValue) {
    if (row == LIST_PARENT_ITEM_POS)
        return defaultRtListCallback(item, row, mode, buffer, bufferSize);

    if (row >= rows) {
        if (buffer && bufferSize > 0) buffer[0] = '\0';
        return false;
    }
    switch (mode) {
        case RENDERFN_NAME:
            if (buffer && bufferSize > 0)
                formatName(row, buffer, static_cast<size_t>(bufferSize));
            return true;
        case RENDERFN_VALUE:
            if (buffer && bufferSize > 0)
                formatValue(row, buffer, static_cast<size_t>(bufferSize));
            return true;
        case RENDERFN_INVOKE:
            // Rows are observations, never commands to drive a pin.
            return true;
        default:
            return defaultRtListCallback(item, row, mode, buffer, bufferSize);
    }
}

} // namespace

int CALLBACK_FUNCTION fnButtonDebugRtCall(RuntimeMenuItem* item, uint8_t row,
        RenderFnMode mode, char* buffer, int bufferSize) {
    return renderReadOnlyList(item, row, mode, buffer, bufferSize, debug::ButtonDebugRowCount,
        diagnostics::formatButtonName, diagnostics::formatButtonValue);
}

int CALLBACK_FUNCTION fnDigitalInputsRtCall(RuntimeMenuItem* item, uint8_t row,
        RenderFnMode mode, char* buffer, int bufferSize) {
    return renderReadOnlyList(item, row, mode, buffer, bufferSize, board::DigitalPinCount,
        diagnostics::formatDigitalName, diagnostics::formatDigitalValue);
}

int CALLBACK_FUNCTION fnAnalogInputsRtCall(RuntimeMenuItem* item, uint8_t row,
        RenderFnMode mode, char* buffer, int bufferSize) {
    return renderReadOnlyList(item, row, mode, buffer, bufferSize, board::AnalogChannelCount,
        diagnostics::formatAnalogName, diagnostics::formatAnalogValue);
}

int CALLBACK_FUNCTION fnSystemChecksRtCall(RuntimeMenuItem* item, uint8_t row,
        RenderFnMode mode, char* buffer, int bufferSize) {
    return renderReadOnlyList(item, row, mode, buffer, bufferSize, debug::SystemCheckCount,
        application::formatSystemCheckName, application::formatSystemCheckValue);
}