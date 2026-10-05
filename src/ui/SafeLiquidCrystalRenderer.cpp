/*
 * Copyright (c) 2018 https://www.thecoderscorner.com (Dave Cherry).
 * Original renderer licensed under the Apache License, Version 2.0.
 * Project-owned bounds-safe renderer derived from the tcMenu LCD plugin.
 */
#include "SafeLiquidCrystalRenderer.h"
#include "LcdText.h"
#include <string.h>

extern const ConnectorLocalInfo applicationInfo;

SafeLiquidCrystalRenderer::SafeLiquidCrystalRenderer(LiquidCrystal& lcd, int columns, int rows)
    : BaseGraphicalRenderer(columns, columns, rows, true, applicationInfo.name), lcd_(&lcd) {}

SafeLiquidCrystalRenderer::~SafeLiquidCrystalRenderer() {
    // BaseMenuRenderer also deletes buffer (using scalar delete). Release the
    // actual array correctly here and null it so the base destructor is safe.
    delete[] buffer;
    buffer = nullptr;
    delete dialog;
    dialog = nullptr;
}

void SafeLiquidCrystalRenderer::initialise() {
    uint8_t charNo = 0;
    for (TitleWidget* widget = firstWidget; widget && charNo < 8; widget = widget->getNext()) {
        for (int i = 0; i < widget->getMaxValue() && charNo < 8; ++i)
            lcd_->createCharPgm(charNo++, widget->getIcon(i));
    }
    lcd_->clear();
    BaseGraphicalRenderer::initialise();
}

void SafeLiquidCrystalRenderer::setEditorChars(char back, char forward, char edit) {
    backChar_ = back;
    forwardChar_ = forward;
    editChar_ = edit;
}

void SafeLiquidCrystalRenderer::drawWidget(tcgfx::Coord where, TitleWidget* widget, tcgfx::color_t, tcgfx::color_t) {
    if (!widget || where.x < 0 || where.y < 0 ||
        where.x >= static_cast<int>(width) || where.y >= static_cast<int>(height))
        return;
    lcd_->setCursor(where.x, where.y);
    lcd_->write(static_cast<uint8_t>(widget->getHeight() + widget->getCurrentState()));
    widget->setChanged(false);
}

void SafeLiquidCrystalRenderer::drawMenuItem(tcgfx::GridPositionRowCacheEntry* entry,
        tcgfx::Coord where, tcgfx::Coord areaSize, const tcgfx::DrawingFlags& flags) {
    if (!entry || where.x < 0 || where.y < 0 ||
        where.x >= static_cast<int>(width) || where.y >= static_cast<int>(height) ||
        areaSize.x <= 0) return;

    MenuItem* item = entry->getMenuItem();
    if (!item) return;
    item->setChanged(displayNumber, false);
    size_t columns = static_cast<size_t>(areaSize.x);
    const size_t remainingColumns = width - static_cast<size_t>(where.x);
    if (columns > remainingColumns) columns = remainingColumns;
    if (columns > bufferSize) columns = bufferSize;
    if (columns == 0) return;

    char name[21] = {};
    char value[21] = {};
    item->copyNameToBuffer(name, sizeof(name));
    name[sizeof(name) - 1] = '\0';
    copyMenuItemValue(item, value, sizeof(value), flags.isActive());
    value[sizeof(value) - 1] = '\0';

    char marker = flags.isEditing() ? editChar_ : (flags.isActive() ? forwardChar_ : ' ');
    if (flags.isActive() && item->getMenuType() == MENUTYPE_BACK_VALUE) marker = backChar_;
    const size_t capacity = static_cast<size_t>(bufferSize) + 1;
    const auto justification = entry->getPosition().getJustification();
    size_t valueStart = 1;

    if (justification == tcgfx::GridPosition::JUSTIFY_TITLE_LEFT_VALUE_RIGHT) {
        valueStart = ui::composeNameValue(buffer, capacity, columns, marker, name, value);
    } else {
        ui::blankRow(buffer, capacity, columns, marker);
        char text[41] = {};
        size_t used = 0;
        if (tcgfx::itemNeedsName(justification))
            used = ui::copyClipped(text, sizeof(text), 0, name, sizeof(text) - 1);
        size_t valueOffset = 0;
        if (tcgfx::itemNeedsValue(justification)) {
            if (used != 0 && used < sizeof(text) - 1) text[used++] = ' ';
            valueOffset = used;
            used += ui::copyClipped(text, sizeof(text), used, value, sizeof(text) - 1);
        }
        text[used] = '\0';
        const auto core = tcgfx::coreJustification(justification);
        const ui::Alignment alignment = core == tcgfx::GridPosition::CORE_JUSTIFY_RIGHT
            ? ui::Alignment::Right : (core == tcgfx::GridPosition::CORE_JUSTIFY_LEFT
                ? ui::Alignment::Left : ui::Alignment::Centre);
        const size_t textStart = ui::writeAligned(buffer, capacity, 1, columns - 1, text, alignment);
        valueStart = textStart + valueOffset;
    }

    if (item == menuMgr.getCurrentEditor() &&
        menuMgr.getEditorHints().getEditorRenderingType() != CurrentEditorRenderingHints::EDITOR_REGULAR) {
        size_t cursor = valueStart + menuMgr.getEditorHints().getStartIndex();
        if (cursor >= columns) cursor = columns - 1;
        setupEditorPlacement(where.x + static_cast<int32_t>(cursor), where.y);
    }
    lcd_->setCursor(where.x, where.y);
    lcd_->print(buffer);
}

void SafeLiquidCrystalRenderer::setupEditorPlacement(int32_t x, int32_t y) {
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x >= width) x = width - 1;
    if (y >= height) y = height - 1;
    editorCursorX_ = static_cast<uint8_t>(x);
    editorCursorY_ = static_cast<uint8_t>(y);
}

void SafeLiquidCrystalRenderer::drawingCommand(RenderDrawingCommand command) {
    switch (command) {
        case DRAW_COMMAND_CLEAR:
            lcd_->clear();
            break;
        case DRAW_COMMAND_START:
            if (editorCursorX_ != 0xff) {
                if (!menuMgr.getCurrentEditor()) {
                    editorCursorX_ = 0xff;
                    editorCursorY_ = 0xff;
                }
                lcd_->noCursor();
            }
            break;
        case DRAW_COMMAND_ENDED:
            if (editorCursorX_ != 0xff) {
                lcd_->setCursor(editorCursorX_, editorCursorY_);
                lcd_->cursor();
            }
            break;
    }
}

void SafeLiquidCrystalRenderer::fillWithBackgroundTo(int endPoint) {
    if (endPoint < 0) endPoint = 0;
    for (uint16_t y = static_cast<uint16_t>(endPoint); y < height; ++y) {
        lcd_->setCursor(0, y);
        for (uint16_t x = 0; x < width; ++x) lcd_->print(' ');
    }
}

BaseDialog* SafeLiquidCrystalRenderer::getDialog() {
    if (!dialog) dialog = new MenuBasedDialog();
    return dialog;
}