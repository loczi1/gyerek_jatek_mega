#include "test_challenge_games.h"
#include "game/ChallengeGames.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

namespace {
#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition); \
        exit(EXIT_FAILURE); \
    } \
} while (false)

uint32_t chosen = 0;
uint32_t calls = 0;
uint32_t bounds[8] = {};

uint32_t randomBelow(uint32_t bound) {
    CHECK(bound != 0);
    bounds[calls % 8U] = bound;
    ++calls;
    return chosen;
}

uint16_t answer(const game::ChallengeGames& engine, uint8_t side, bool correct) {
    const uint8_t base = side == game::DuelLeft ? 0 : 8;
    for (uint8_t i = 0; i < 3; ++i)
        if ((engine.colorHue(side, i) == engine.cueHue()) == correct)
            return static_cast<uint16_t>(uint16_t(1) << (base + i));
    CHECK(false);
    return 0;
}

uint32_t input(game::ChallengeGames& engine, uint32_t now = 0, uint16_t held = 0) {
    engine.select(game::ChallengeMode::Duel, now, 0, false, held);
    CHECK(engine.phase() == game::PanelPhase::Showing);
    CHECK(!engine.cueVisible());
    CHECK(engine.update(now + 2000U, 0, false, held));
    CHECK(engine.phase() == game::PanelPhase::Input);
    CHECK(engine.cueVisible());
    CHECK(engine.takeFeedback() == game::PanelFeedback::None);
    return now + 2000U;
}

uint32_t pressUpper(game::ChallengeGames& engine, uint32_t now, uint16_t mask) {
    CHECK(!engine.update(now + 1U, 0, false, mask));
    CHECK(!engine.update(now + 30U, 0, false, mask));
    CHECK(engine.update(now + 31U, 0, false, mask));
    return now + 31U;
}

void testDisabledAndImmediateSelection() {
    game::ChallengeGames engine;
    CHECK(engine.mode() == game::ChallengeMode::Disabled);
    CHECK(engine.phase() == game::PanelPhase::Disabled);
    CHECK(!engine.update(100, 31, true, game::DuelButtonMask));
    CHECK(!engine.resultVisible(100));
    CHECK(!engine.cueVisible());
    CHECK(engine.takeFeedback() == game::PanelFeedback::None);
    engine.select(game::ChallengeMode::Lock3, 100, 0, true);
    CHECK(engine.phase() == game::PanelPhase::Input);
    CHECK(engine.targetMask() == 1);
    CHECK(!engine.update(130, 0, true));
    engine.select(game::ChallengeMode::Lock4, 200, 31);
    CHECK(engine.phase() == game::PanelPhase::Input);
    CHECK(engine.requiredSwitchMask() != 31);
    engine.select(game::ChallengeMode::Duel, 300);
    CHECK(engine.phase() == game::PanelPhase::Showing);
    engine.select(static_cast<game::ChallengeMode>(255), 400);
    CHECK(engine.mode() == game::ChallengeMode::Disabled);
    CHECK(engine.phase() == game::PanelPhase::Disabled);
    CHECK(engine.targetMask() == 0 && engine.wonSides() == 0 && engine.lostSides() == 0);
    CHECK(!engine.update(100000, 31, true, game::DuelButtonMask));
}

void testLockEveryPatternExcludesInitial() {
    for (uint8_t mode = 0; mode < 2; ++mode) {
        for (uint8_t initial = 0; initial < 32; ++initial) {
            uint32_t seen = 0;
            for (uint8_t rank = 0; rank < 31; ++rank) {
                chosen = rank;
                calls = 0;
                game::ChallengeGames engine(randomBelow);
                engine.select(mode == 0 ? game::ChallengeMode::Lock3 : game::ChallengeMode::Lock4,
                              0, initial);
                CHECK(calls == 1 && bounds[0] == 31);
                CHECK(engine.targetMask() < 32);
                CHECK(engine.requiredSwitchMask() != initial);
                CHECK(engine.requiredSwitchMask() == (mode == 0 ? engine.targetMask() :
                      static_cast<uint8_t>(engine.targetMask() ^ 31U)));
                const uint32_t bit = uint32_t(1) << engine.targetMask();
                CHECK((seen & bit) == 0);
                seen |= bit;
                CHECK(!engine.update(30, initial, false));
                const uint8_t required = engine.requiredSwitchMask();
                CHECK(!engine.update(31, required, false));
                CHECK(!engine.update(60, required, false));
                CHECK(engine.update(61, required, false));
                CHECK(engine.phase() == game::PanelPhase::RoundWin);
                CHECK(engine.takeFeedback() == game::PanelFeedback::Win);
                CHECK(engine.takeFeedback() == game::PanelFeedback::None);
            }
            const uint8_t excluded = mode == 0 ? initial : static_cast<uint8_t>(initial ^ 31U);
            CHECK(seen == (UINT32_MAX ^ (uint32_t(1) << excluded)));
        }
    }
}

void testLockWholeLevelDebounceAndBounce() {
    game::ChallengeGames engine;
    engine.select(game::ChallengeMode::Lock3, 0, 31);
    CHECK(engine.requiredSwitchMask() == 0); // All five LOW is a real answer.
    CHECK(!engine.update(10, 0, false));
    CHECK(!engine.update(39, 0, false));
    CHECK(!engine.update(40, 16, false)); // One wrong switch invalidates all-five match.
    CHECK(!engine.update(41, 0, false));
    CHECK(!engine.update(70, 0, false));
    CHECK(engine.update(71, 0, false));
    CHECK(engine.phase() == game::PanelPhase::RoundWin);

    engine.select(game::ChallengeMode::Lock3, 100, 0xff);
    CHECK(engine.requiredSwitchMask() == 0);
    CHECK(!engine.update(110, 0xe0, false)); // Ignore high unused bits.
    CHECK(engine.update(140, 0xa0, false));
    CHECK(engine.phase() == game::PanelPhase::RoundWin);
}

void testLockUnlimitedAndDeadlinePriority() {
    const uint32_t boots[] = {0, UINT32_MAX - 1000U};
    for (uint32_t boot : boots) {
        game::ChallengeGames engine;
        engine.select(game::ChallengeMode::Lock3, boot, 0);
        CHECK(!engine.update(boot + 100000U, 0, false));
        CHECK(engine.takeFeedback() == game::PanelFeedback::None);
        CHECK(!engine.update(boot + 100001U, engine.requiredSwitchMask(), false));
        CHECK(engine.update(boot + 100031U, engine.requiredSwitchMask(), false));
        CHECK(engine.phase() == game::PanelPhase::RoundWin);

        for (uint32_t finish = 1999; finish <= 2001; ++finish) {
            engine.select(game::ChallengeMode::Lock4, boot, 0);
            const uint8_t required = engine.requiredSwitchMask();
            engine.update(boot + finish - 30U, required, false);
            engine.takeFeedback();
            CHECK(engine.update(boot + finish, required, false));
            CHECK(engine.phase() == (finish < 2000 ? game::PanelPhase::RoundWin : game::PanelPhase::Loss));
            CHECK(engine.takeFeedback() == (finish < 2000 ? game::PanelFeedback::Win : game::PanelFeedback::Loss));
        }
        engine.select(game::ChallengeMode::Lock4, boot, 0);
        CHECK(engine.update(boot + 100000U, 0, false));
        CHECK(engine.phase() == game::PanelPhase::Loss);
        CHECK(engine.takeFeedback() == game::PanelFeedback::Loss);
        CHECK(engine.update(boot + 100740U, 0, false));
        CHECK(engine.phase() == game::PanelPhase::Ready); // No automatic next pattern.
    }
}

void testWarningsExactAndCoalesced() {
    const game::ChallengeMode modes[] = {game::ChallengeMode::Lock4, game::ChallengeMode::Duel};
    const uint32_t boots[] = {0, UINT32_MAX - 1200U};
    for (game::ChallengeMode mode : modes) {
        for (uint32_t boot : boots) {
            game::ChallengeGames engine;
            engine.select(mode, boot);
            CHECK(!engine.update(boot + 999U, 0, false));
            CHECK(engine.update(boot + 1000U, 0, false));
            CHECK(engine.takeFeedback() == game::PanelFeedback::Warning);
            CHECK(engine.takeFeedback() == game::PanelFeedback::None);
            CHECK(!engine.update(boot + 1000U, 0, false));
            CHECK(!engine.update(boot + 1499U, 0, false));
            CHECK(engine.update(boot + 1500U, 0, false));
            CHECK(engine.takeFeedback() == game::PanelFeedback::Warning);
            CHECK(!engine.update(boot + 1999U, 0, false));
            CHECK(engine.update(boot + 2000U, 0, false));
            CHECK(engine.takeFeedback() == (mode == game::ChallengeMode::Lock4 ?
                  game::PanelFeedback::Loss : game::PanelFeedback::None));
            engine.select(mode, boot);
            CHECK(engine.update(boot + 1800U, 0, false));
            CHECK(engine.takeFeedback() == game::PanelFeedback::Warning);
            CHECK(!engine.update(boot + 1800U, 0, false));
            CHECK(!engine.update(boot + 1801U, 0, false));
            CHECK(engine.takeFeedback() == game::PanelFeedback::None);
        }
    }
    game::ChallengeGames engine;
    engine.select(game::ChallengeMode::Lock4, 0);
    const uint8_t required = engine.requiredSwitchMask();
    CHECK(!engine.update(970, required, false));
    CHECK(engine.update(1000, required, false)); // Match beats warning.
    CHECK(engine.takeFeedback() == game::PanelFeedback::Win);
}

void testLockFreshStartAndFeedbackIsolation() {
    game::ChallengeGames engine;
    engine.select(game::ChallengeMode::Lock3, 0, 0, true);
    CHECK(!engine.update(1, 1, true));
    CHECK(engine.update(31, 1, true));
    CHECK(engine.update(571, 1, true));
    CHECK(engine.phase() == game::PanelPhase::Ready);
    CHECK(engine.takeFeedback() == game::PanelFeedback::None);
    CHECK(!engine.update(1000, 1, true));
    CHECK(!engine.update(1001, 1, false));
    CHECK(!engine.update(1031, 1, false));
    CHECK(!engine.update(1032, 1, true));
    CHECK(!engine.update(1061, 1, true));
    CHECK(engine.update(1062, 1, true));
    CHECK(engine.phase() == game::PanelPhase::Input);
    CHECK(engine.requiredSwitchMask() != 1);
    CHECK(!engine.update(1063, 1, true));

    engine.select(game::ChallengeMode::Lock4, 2000);
    CHECK(engine.update(4000, 0, false));
    engine.update(4730, 0, true); // Partial feedback press is discarded.
    CHECK(engine.update(4740, 0, true));
    CHECK(!engine.update(4770, 0, true));
    CHECK(engine.phase() == game::PanelPhase::Ready);
    CHECK(!engine.update(4771, 0, false));
    CHECK(!engine.update(4801, 0, false));
    CHECK(!engine.update(4802, 0, true));
    CHECK(engine.update(4832, 0, true));
    CHECK(engine.phase() == game::PanelPhase::Input);
}

void checkPalette(const game::ChallengeGames& engine) {
    uint8_t common = 0;
    uint8_t cueLeft = 0;
    uint8_t cueRight = 0;
    for (uint8_t i = 0; i < 3; ++i) {
        CHECK(engine.colorHue(game::DuelLeft, i) == engine.duelHue(i));
        CHECK(engine.colorHue(game::DuelRight, i) == engine.duelHue(8U + i));
        if (engine.duelHue(i) == engine.cueHue()) ++cueLeft;
        if (engine.duelHue(8U + i) == engine.cueHue()) ++cueRight;
        for (uint8_t j = 0; j < 3; ++j) {
            if (engine.duelHue(i) == engine.duelHue(8U + j)) ++common;
            if (i != j) {
                CHECK(engine.duelHue(i) != engine.duelHue(j));
                CHECK(engine.duelHue(8U + i) != engine.duelHue(8U + j));
            }
        }
    }
    CHECK(common == 1 && cueLeft == 1 && cueRight == 1);
    CHECK(engine.duelHue(3) == 0 && engine.duelHue(11) == 0 && engine.duelHue(255) == 0);
    CHECK(engine.colorHue(0, 0) == 0 && engine.colorHue(3, 0) == 0);
    CHECK(engine.colorHue(game::DuelLeft, 3) == 0);
}

void testDuelPaletteBoundedConstantFaultyAndNullRng() {
    for (uint32_t seed = 0; seed <= 255; ++seed) {
        chosen = seed;
        calls = 0;
        game::ChallengeGames engine(randomBelow);
        engine.select(game::ChallengeMode::Duel, 0);
        CHECK(calls == 8);
        const uint32_t expected[] = {5, 4, 3, 2, 3, 2, 3, 2};
        for (uint8_t i = 0; i < 8; ++i) CHECK(bounds[i] == expected[i]);
        checkPalette(engine);
        uint8_t hues[6];
        for (uint8_t i = 0; i < 3; ++i) {
            hues[i] = engine.duelHue(i);
            hues[3U + i] = engine.duelHue(8U + i);
        }
        engine.update(1000, 0, false);
        engine.update(1500, 0, false);
        CHECK(engine.update(2000, 0, false));
        for (uint8_t i = 0; i < 3; ++i) {
            CHECK(hues[i] == engine.duelHue(i));
            CHECK(hues[3U + i] == engine.duelHue(8U + i));
        }
        CHECK(calls == 8); // Never reroll within a round.
    }
    chosen = UINT32_MAX;
    game::ChallengeGames faulty(randomBelow);
    faulty.select(game::ChallengeMode::Duel, 0);
    checkPalette(faulty);
    faulty.select(game::ChallengeMode::Lock3, 0, 0);
    CHECK(faulty.requiredSwitchMask() != 0 && faulty.targetMask() < 32);
    game::ChallengeGames fallback;
    fallback.select(game::ChallengeMode::Duel, 0);
    checkPalette(fallback);
}

void testDuelEitherSideCorrectOrWrong() {
    const uint8_t sides[] = {game::DuelLeft, game::DuelRight};
    for (uint8_t side : sides) {
        for (uint8_t correct = 0; correct < 2; ++correct) {
            game::ChallengeGames engine;
            uint32_t now = input(engine);
            CHECK(!engine.update(now + 100000U, 0, true)); // D39 never answers; no deadline.
            CHECK(engine.takeFeedback() == game::PanelFeedback::None);
            const uint16_t mask = answer(engine, side, correct != 0);
            now = pressUpper(engine, now + 100000U, mask);
            CHECK(engine.wonSides() == (correct != 0 ? side : (3U ^ side)));
            CHECK(engine.lostSides() == (3U ^ engine.wonSides()));
            CHECK(!engine.cueVisible());
            CHECK(engine.phase() == (correct != 0 ? game::PanelPhase::RoundWin : game::PanelPhase::Loss));
            CHECK(engine.takeFeedback() == (correct != 0 ? game::PanelFeedback::Win : game::PanelFeedback::Loss));
            CHECK(engine.takeFeedback() == game::PanelFeedback::None);
            CHECK(engine.update(now + 540U, 0, false, mask));
            CHECK(engine.phase() == game::PanelPhase::Ready); // Duel Loss also only 540ms.
            CHECK(engine.wonSides() == 0 && engine.lostSides() == 0);
            CHECK(!engine.update(now + 1000U, 0, false, mask));
        }
    }
}

void testDuelSimultaneousDrawsAndMixed() {
    for (uint8_t combination = 0; combination < 4; ++combination) {
        game::ChallengeGames engine;
        const uint32_t now = input(engine);
        const bool left = (combination & 1U) != 0;
        const bool right = (combination & 2U) != 0;
        const uint16_t mask = answer(engine, game::DuelLeft, left) |
                              answer(engine, game::DuelRight, right);
        pressUpper(engine, now, mask);
        CHECK(engine.wonSides() == combination);
        CHECK(engine.lostSides() == (3U ^ combination));
        CHECK(engine.takeFeedback() == (combination == 0 ? game::PanelFeedback::Loss : game::PanelFeedback::Win));
    }
}

void testDuelRawChordsAndFirstAnswerWins() {
    const uint8_t sides[] = {game::DuelLeft, game::DuelRight};
    for (uint8_t side : sides) {
        game::ChallengeGames engine;
        uint32_t now = input(engine);
        const uint16_t correct = answer(engine, side, true);
        const uint16_t wrong = answer(engine, side, false);
        CHECK(!engine.update(now + 1U, 0, false, correct));
        CHECK(!engine.update(now + 30U, 0, false, correct | wrong));
        CHECK(engine.update(now + 31U, 0, false, correct | wrong));
        CHECK(engine.lostSides() == side); // Extra raw button not yet debounced.
        CHECK(engine.takeFeedback() == game::PanelFeedback::Loss);
        const uint8_t won = engine.wonSides();
        engine.update(now + 32U, 0, false, game::DuelButtonMask);
        engine.update(now + 62U, 0, false, game::DuelButtonMask);
        CHECK(engine.wonSides() == won);
        CHECK(engine.takeFeedback() == game::PanelFeedback::None);

        now = input(engine, 5000);
        const uint16_t first = answer(engine, side, true);
        const uint16_t second = answer(engine, static_cast<uint8_t>(3U ^ side), true);
        CHECK(!engine.update(now + 1U, 0, false, first));
        CHECK(!engine.update(now + 2U, 0, false, first | second));
        CHECK(engine.update(now + 31U, 0, false, first | second));
        CHECK(engine.wonSides() == side); // Other side is still below 30ms.
    }
}

void testDuelPreviewHeldAndPartialPresses() {
    for (uint8_t partial = 0; partial < 2; ++partial) {
        game::ChallengeGames engine;
        engine.select(game::ChallengeMode::Duel, 0);
        const uint16_t correct = answer(engine, game::DuelLeft, true);
        const uint32_t pressedAt = partial != 0 ? 1990U : 100U;
        if (partial != 0) {
            CHECK(engine.update(1000, 0, false));
            CHECK(engine.update(1500, 0, false));
            engine.takeFeedback();
        }
        CHECK(!engine.update(pressedAt, 0, false, correct));
        CHECK(engine.update(2000, 0, false, correct));
        CHECK(!engine.update(2100, 0, false, correct));
        CHECK(engine.phase() == game::PanelPhase::Input);
        CHECK(!engine.update(2101, 0, false, 0));
        CHECK(!engine.update(2130, 0, false, 0));
        CHECK(!engine.update(2131, 0, false, 0));
        pressUpper(engine, 2131, correct);
        CHECK(engine.wonSides() == game::DuelLeft);
    }
    game::ChallengeGames engine;
    uint32_t now = input(engine, 0, game::DuelButtonMask);
    CHECK(!engine.update(now + 100U, 0, false, game::DuelButtonMask));
    const uint16_t right = answer(engine, game::DuelRight, true);
    CHECK(!engine.update(now + 101U, 0, false, right)); // Releasing only left is insufficient.
    CHECK(!engine.update(now + 131U, 0, false, right));
    CHECK(!engine.update(now + 132U, 0, false, 0));
    CHECK(!engine.update(now + 161U, 0, false, right)); // Release bounce never arms.
    CHECK(!engine.update(now + 191U, 0, false, right));
    CHECK(!engine.update(now + 192U, 0, false, 0));
    CHECK(!engine.update(now + 222U, 0, false, 0));
    pressUpper(engine, now + 222U, right);
    CHECK(engine.wonSides() == game::DuelRight);
}

void testDuelPrematureReleaseAndUnusedInputs() {
    game::ChallengeGames engine;
    engine.select(game::ChallengeMode::Duel, 0);
    CHECK(!engine.update(1, 31, true, game::DuelButtonMask));
    CHECK(!engine.update(31, 0, false, game::DuelButtonMask));
    CHECK(!engine.update(32, 31, true, 0));
    CHECK(!engine.update(62, 0, false, 0));
    CHECK(engine.update(2000, 31, true, 0));
    CHECK(!engine.update(2001, 0, false, 0xf8f8)); // Bottom/out-of-range inputs ignored.
    CHECK(!engine.update(2031, 31, true, 0xf8f8));
    pressUpper(engine, 2031, answer(engine, game::DuelRight, true));
    CHECK(engine.wonSides() == game::DuelRight);
}

void testDuelReadyFreshStartConsumedAndFeedbackIsolation() {
    game::ChallengeGames engine;
    uint32_t now = input(engine);
    const uint16_t held = answer(engine, game::DuelLeft, true);
    now = pressUpper(engine, now, held);
    engine.update(now + 530U, 0, false, game::DuelButtonMask);
    CHECK(engine.update(now + 540U, 0, false, game::DuelButtonMask));
    CHECK(!engine.update(now + 570U, 0, true, game::DuelButtonMask));
    CHECK(engine.phase() == game::PanelPhase::Ready);
    CHECK(!engine.update(now + 571U, 0, false, 0));
    CHECK(!engine.update(now + 601U, 0, false, 0));
    const uint32_t restarted = pressUpper(engine, now + 601U, 1);
    CHECK(engine.phase() == game::PanelPhase::Showing);
    CHECK(engine.takeFeedback() == game::PanelFeedback::None);
    CHECK(engine.update(restarted + 2000U, 0, false, 1));
    CHECK(!engine.update(restarted + 2100U, 0, false, 1));
    CHECK(engine.phase() == game::PanelPhase::Input);
}

void testResultBlinkChangesAndRollover() {
    const uint32_t boots[] = {0, UINT32_MAX - 2010U};
    for (uint32_t boot : boots) {
        for (uint8_t kind = 0; kind < 3; ++kind) {
            game::ChallengeGames engine;
            uint32_t now = boot;
            if (kind == 2) {
                now = input(engine, boot);
                now = pressUpper(engine, now, answer(engine, game::DuelLeft, false));
            } else {
                engine.select(kind == 0 ? game::ChallengeMode::Lock3 : game::ChallengeMode::Lock4, boot);
                if (kind == 0) {
                    CHECK(!engine.update(boot + 1U, engine.requiredSwitchMask(), false));
                    CHECK(engine.update(boot + 31U, engine.requiredSwitchMask(), false));
                    now += 31U;
                } else {
                    now += 2000U;
                    CHECK(engine.update(now, 0, false));
                }
            }
            engine.takeFeedback();
            const uint32_t duration = kind == 1 ? 740U : 540U;
            const uint32_t beat = kind == 1 ? 100U : 90U;
            CHECK(engine.resultVisible(now));
            bool previous = true;
            for (uint32_t elapsed = 1; elapsed < duration; ++elapsed) {
                const bool visible = (elapsed / beat) % 2U == 0;
                CHECK(engine.resultVisible(now + elapsed) == visible);
                CHECK(engine.update(now + elapsed, 0, false) == (visible != previous));
                CHECK(engine.takeFeedback() == game::PanelFeedback::None);
                previous = visible;
            }
            CHECK(!engine.resultVisible(now + duration));
            CHECK(engine.update(now + duration, 0, false));
            CHECK(engine.phase() == game::PanelPhase::Ready);
            CHECK(!engine.update(now + duration, 0, false));
        }
    }
}

void testDuelRolloverAndDelayedTransitions() {
    game::ChallengeGames engine;
    const uint32_t boot = UINT32_MAX - 1000U;
    const uint32_t now = input(engine, boot);
    pressUpper(engine, now, answer(engine, game::DuelRight, true));
    CHECK(engine.wonSides() == game::DuelRight);
    engine.select(game::ChallengeMode::Duel, boot);
    CHECK(engine.update(boot + 100000U, 0, false));
    CHECK(engine.phase() == game::PanelPhase::Input);
    CHECK(engine.takeFeedback() == game::PanelFeedback::None); // No stale preview warnings.
    CHECK(!engine.update(boot + 100000U, 0, false));
    CHECK(!engine.update(boot + 200000U, 0, false));
    pressUpper(engine, boot + 200000U, answer(engine, game::DuelLeft, true));
    CHECK(engine.update(boot + 300000U, 0, false));
    CHECK(engine.phase() == game::PanelPhase::Ready);
    CHECK(!engine.update(boot + 300000U, 0, false));
}

void testCancellationAndReselectionEveryPhase() {
    for (uint8_t state = 0; state < 6; ++state) {
        game::ChallengeGames engine;
        if (state <= 3) {
            const uint32_t now = input(engine);
            if (state >= 1) {
                const uint32_t resultAt = pressUpper(engine, now,
                    answer(engine, game::DuelLeft, state != 2));
                if (state == 3) engine.update(resultAt + 540U, 0, false);
            }
        } else if (state == 4) {
            engine.select(game::ChallengeMode::Duel, 0);
            engine.update(1000, 0, false); // Pending warning must also clear.
        } else {
            engine.select(game::ChallengeMode::Lock4, 0);
            engine.update(2000, 0, false);
        }
        engine.select(game::ChallengeMode::Disabled, 10000, 31, true, game::DuelButtonMask);
        CHECK(engine.phase() == game::PanelPhase::Disabled);
        CHECK(engine.takeFeedback() == game::PanelFeedback::None);
        CHECK(engine.wonSides() == 0 && engine.lostSides() == 0);
        CHECK(!engine.cueVisible() && !engine.resultVisible(10000));
        CHECK(engine.targetMask() == 0 && engine.cueHue() == 0);
        CHECK(!engine.update(100000, 0, false));
        engine.select(game::ChallengeMode::Duel, 100001, 0, false, game::DuelButtonMask);
        CHECK(engine.phase() == game::PanelPhase::Showing);
        checkPalette(engine);
        engine.select(game::ChallengeMode::Duel, 100002);
        CHECK(engine.update(101002, 0, false));
        CHECK(engine.update(101502, 0, false));
        CHECK(!engine.update(102001, 0, false)); // Reselection restarts preview.
        CHECK(engine.update(102002, 0, false));
        CHECK(engine.phase() == game::PanelPhase::Input);
        engine.select(game::ChallengeMode::Lock3, 103000, 31);
        CHECK(engine.phase() == game::PanelPhase::Input);
        CHECK(engine.requiredSwitchMask() != 31);
        CHECK(engine.takeFeedback() == game::PanelFeedback::None);
    }
}

struct Test { const char* name; void (*run)(); };
const Test Tests[] = {
    {"challenge disabled and immediate selection", testDisabledAndImmediateSelection},
    {"lock all patterns, literal levels and initial-match exclusion", testLockEveryPatternExcludesInitial},
    {"lock all-five level debounce, LOW answers and bounce", testLockWholeLevelDebounceAndBounce},
    {"lock unlimited L3, L4 deadline priority and rollover", testLockUnlimitedAndDeadlinePriority},
    {"challenge warning boundaries, priority and delayed coalescing", testWarningsExactAndCoalesced},
    {"lock fresh D39 restart and feedback isolation", testLockFreshStartAndFeedbackIsolation},
    {"duel unique palette, exact overlap and bounded RNG", testDuelPaletteBoundedConstantFaultyAndNullRng},
    {"duel either side correct/wrong and unlimited input", testDuelEitherSideCorrectOrWrong},
    {"duel simultaneous green/red draws and mixed answers", testDuelSimultaneousDrawsAndMixed},
    {"duel raw chords and first debounced side wins", testDuelRawChordsAndFirstAnswerWins},
    {"duel preview/partial holds and all-upper release gating", testDuelPreviewHeldAndPartialPresses},
    {"duel premature release and unused inputs", testDuelPrematureReleaseAndUnusedInputs},
    {"duel consumed fresh restart and feedback isolation", testDuelReadyFreshStartConsumedAndFeedbackIsolation},
    {"challenge result blink frame changes and rollover", testResultBlinkChangesAndRollover},
    {"duel rollover and delayed single transitions", testDuelRolloverAndDelayedTransitions},
    {"challenge cancellation and reselection in every phase", testCancellationAndReselectionEveryPhase}
};
} // namespace

size_t runChallengeGameTests() {
    for (const Test& test : Tests) {
        chosen = calls = 0;
        test.run();
        printf("PASS: %s\n", test.name);
    }
    return sizeof(Tests) / sizeof(Tests[0]);
}