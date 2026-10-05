/*
 * Copyright (c) 2018 https://www.thecoderscorner.com (Dave Cherry).
 * Original renderer licensed under the Apache License, Version 2.0.
 * Project-owned bounds-safe renderer derived from the tcMenu LCD plugin.
 */
#ifndef GYEREK_JATEK_SAFE_LIQUID_CRYSTAL_RENDERER_H
#define GYEREK_JATEK_SAFE_LIQUID_CRYSTAL_RENDERER_H

#include <tcMenu.h>
#include <LiquidCrystalIO.h>
#include <BaseDialog.h>
#include <graphics/BaseGraphicalRenderer.h>

class SafeLiquidCrystalRenderer : public tcgfx::BaseGraphicalRenderer {
public:
    SafeLiquidCrystalRenderer(LiquidCrystal& lcd, int columns, int rows);
    ~SafeLiquidCrystalRenderer() override;
    void initialise() override;
    void setTitleRequired(bool required) {
        titleMode = required ? TITLE_FIRST_ROW : NO_TITLE;
    }
    void setEditorChars(char back, char forward, char edit);
    uint8_t getRows() { return height; }
    LiquidCrystal* getLCD() { return lcd_; }
    BaseDialog* getDialog() override;

    void drawingCommand(RenderDrawingCommand command) override;
    void drawWidget(tcgfx::Coord where, TitleWidget* widget, tcgfx::color_t foreground, tcgfx::color_t background) override;
    void drawMenuItem(tcgfx::GridPositionRowCacheEntry* entry, tcgfx::Coord where,
                      tcgfx::Coord areaSize, const tcgfx::DrawingFlags& flags) override;
    void fillWithBackgroundTo(int endPoint) override;
    tcgfx::ItemDisplayPropertiesFactory& getDisplayPropertiesFactory() override {
        return propertiesFactory_;
    }
    tcgfx::NullItemDisplayPropertiesFactory& getLcdDisplayPropertiesFactory() {
        return propertiesFactory_;
    }
    void setupEditorPlacement(int32_t x, int32_t y);

private:
    LiquidCrystal* lcd_;
    tcgfx::NullItemDisplayPropertiesFactory propertiesFactory_;
    char backChar_ = '<';
    char forwardChar_ = '>';
    char editChar_ = '=';
    uint8_t editorCursorX_ = 0xff;
    uint8_t editorCursorY_ = 0xff;
};

#endif