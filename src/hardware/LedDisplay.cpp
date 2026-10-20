#include "LedDisplay.h"
#include "config/BoardConfig.h"
#include "config/PanelConfig.h"
#include <FastLED.h>

namespace leds {
namespace {

// One controller owns the workbook strip pin for the game and strip test.
// Every frame clears the tail, so test pixels cannot leak into gameplay.
CRGB pixels[board::LedStripCapacity];
static_assert(board::LedStripCapacity >= game::TotalLedCount, "LED buffer too small for the game");

void drawScore(uint8_t count) {
    for (uint8_t i = 0; i < game::ScoreLedCount && i < count; ++i)
        pixels[game::GameLedCount + i] = CRGB::Blue;
}

} // namespace

void begin() {
    FastLED.addLeds<WS2812B, board::LedDataPin, GRB>(pixels, board::LedStripCapacity);
    FastLED.setMaxPowerInVoltsAndMilliamps(5, board::MaximumLedMilliamps);
    clear();
}

void clear() {
    fill_solid(pixels, board::LedStripCapacity, CRGB::Black);
    FastLED.show();
}

void draw(const game::CycloneGame& currentGame) {
    fill_solid(pixels, board::LedStripCapacity, CRGB::Black);
    switch (currentGame.state()) {
        case game::State::Disabled:
            break;

        case game::State::Idle:
        case game::State::Intro:
            FastLED.setBrightness(board::IdleBrightness);
            fill_rainbow(pixels, game::TotalLedCount, 10, 15);
            if (currentGame.state() == game::State::Intro) {
                for (uint8_t i = 0; i < currentGame.clearedGameLeds(); ++i)
                    pixels[i] = CRGB::Black;
                for (uint8_t i = 0; i < currentGame.clearedScoreLeds(); ++i)
                    pixels[game::GameLedCount + i] = CRGB::Black;
            }
            break;

        case game::State::Playing: {
            FastLED.setBrightness(board::GameBrightness);
            const int radius = game::CycloneGame::targetRadius(currentGame.level());
            for (int offset = -radius; offset <= radius; ++offset) {
                const int index = static_cast<int>(currentGame.target()) + offset;
                if (index >= 0 && index < game::GameLedCount)
                    pixels[index] = offset == 0 ? CRGB::Green : CRGB(255, 140, 0);
            }
            if (currentGame.position() < game::GameLedCount)
                pixels[currentGame.position()] = CRGB::Red;
            drawScore(currentGame.scoreLeds());
            break;
        }

        case game::State::WinFeedback:
        case game::State::LossFeedback:
            FastLED.setBrightness(board::FeedbackBrightness);
            if (currentGame.feedbackOn()) {
                fill_solid(pixels, game::GameLedCount,
                    currentGame.state() == game::State::WinFeedback ? CRGB::Green : CRGB::Red);
            }
            drawScore(currentGame.scoreLeds());
            break;
    }
    FastLED.show();
}

void drawPanel(const game::PanelGames& currentGame) {
    fill_solid(pixels, board::LedStripCapacity, CRGB::Black);
    FastLED.setBrightness(board::GameBrightness);
    for (uint8_t channel = 0; channel < game::PanelChannelCount; ++channel) {
        if (!(currentGame.distractorMask() & (uint16_t(1) << channel))) continue;
        const uint8_t distractorPixel = board::panelStripPixel(channel);
        if (distractorPixel > 0 && distractorPixel <= board::LedStripCapacity)
            pixels[distractorPixel - 1] = CHSV(currentGame.distractorHue(channel), 255, 255);
    }
    const uint8_t pixel = board::panelStripPixel(currentGame.litChannel());
    if (pixel > 0 && pixel <= board::LedStripCapacity)
        pixels[pixel - 1] = currentGame.whiteTarget() ? CRGB(CRGB::White) :
            CRGB(CHSV(currentGame.hue(), 255, 255));
    FastLED.show();
}

void drawRandom2(const game::PanelGames& currentGame, const uint8_t* inputs, uint8_t count) {
    fill_solid(pixels, board::LedStripCapacity, CRGB::Black);
    FastLED.setBrightness(board::GameBrightness);
    if (currentGame.phase() == game::PanelPhase::Hit) {
        // Three changing pixels on the Cyclon track, away from paired lamps.
        for (uint8_t i = 0; i < 3; ++i)
            pixels[currentGame.discoPixel(i)] = CHSV(currentGame.hue() + i * 80U, 255, 160);
        for (uint8_t i = 0; inputs && i < count && i < game::PanelInputCount; ++i) {
            if (!(currentGame.lampInputMask() & (uint32_t(1) << i))) continue;
            const uint8_t pixel = board::pairedStripPixel(inputs[i]);
            if (pixel > 0 && pixel <= board::LedStripCapacity)
                pixels[pixel - 1] = CHSV(currentGame.hue(), 255, 255);
        }
    }
    FastLED.show();
}

void drawTest(const debug::LedStripTester& test) {
    fill_solid(pixels, board::LedStripCapacity, CRGB::Black);
    FastLED.setBrightness(board::GameBrightness);
    if (test.isActive() && test.position() < test.length() &&
        test.position() < board::LedStripCapacity)
        pixels[test.position()] = CRGB::White;
    FastLED.show();
}

} // namespace leds