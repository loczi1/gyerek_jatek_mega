#include "Gyerek_jatek_arduinomega_menu.h"
#include "app/Application.h"
#include "config/BoardConfig.h"
#include <stdio.h>

void CALLBACK_FUNCTION Gomb1(int) {
    application::setInputLogging(menuGomb1.getCurrentValue() != 0);
}

void CALLBACK_FUNCTION DigIn(int) {
    application::refreshDiagnostics();
}

void CALLBACK_FUNCTION AnIn(int) {
    application::refreshDiagnostics();
}

void CALLBACK_FUNCTION Tesztelo(int) {
    application::refreshDiagnostics();
}

void CALLBACK_FUNCTION Jatek(int) {
    application::selectMode(menuJatek.getCurrentValue());
}

void CALLBACK_FUNCTION DigOut(int) {
    application::setDiagnosticOutput(menuDigOut.getCurrentValue() != 0);
}

void CALLBACK_FUNCTION Restart(int) {
    application::restart();
}

int CALLBACK_FUNCTION fnNewRuntimeListRtCall(RuntimeMenuItem* item, uint8_t row,
        RenderFnMode mode, char* buffer, int bufferSize) {
    if (row == LIST_PARENT_ITEM_POS)
        return defaultRtListCallback(item, row, mode, buffer, bufferSize);

    const uint8_t inputCount = board::LastDiagnosticPin - board::FirstDiagnosticPin + 1;
    if (row >= inputCount)
        return defaultRtListCallback(item, row, mode, buffer, bufferSize);

    const uint8_t pin = board::FirstDiagnosticPin + row;
    switch (mode) {
        case RENDERFN_NAME:
            if (buffer && bufferSize > 0)
                snprintf(buffer, static_cast<size_t>(bufferSize), "D%u", static_cast<unsigned int>(pin));
            return true;
        case RENDERFN_VALUE:
            if (buffer && bufferSize > 0)
                snprintf(buffer, static_cast<size_t>(bufferSize), "%s", digitalRead(pin) == LOW ? "LOW" : "HIGH");
            return true;
        default:
            return defaultRtListCallback(item, row, mode, buffer, bufferSize);
    }
}