# Excel Configuration Architecture

## Goals

- Keep the workbook as the only editable source of hardware and game-control configuration.
- Generate constexpr C++ tables so the AVR firmware uses flash-friendly static data and no runtime JSON parsing.
- Support multiple games, per-game control remapping and per-level role changes without allocating RAM for configuration objects.
- Keep the generated C++ layout easy to diff, easy to document and deterministic in CI/build hooks.

## Workbook sheets

The current workbook already contains the two minimum sheets the generator consumes today:

| Sheet | Purpose | Required columns |
| --- | --- | --- |
| `PinMap_export` | Flat hardware export used for generated pin metadata | `ID`, `Info`, `ArduinoPin`, `Type` |
| `GameControls` | Control bindings per game and level range | `GameId`, `Level`, `Action`, `ControlId` |

### Implemented GameLevel rules (2026-10-20)

The user-maintained **`GameLevel`** sheet in [../Gyerek_jatek_3.0.xlsx](../Gyerek_jatek_3.0.xlsx) has one row per game/menu-level combination. Its actual columns are **`GameID`, `MenuLevel (szint)`, `Nehézség`, `Time (sec)`, `Hang jelzés`, `Info`**. Corrected Cyclon/Sorrend requirements are now implemented manually in source, superseding the earlier **documentation-only review** and old nonmemory Sorrend4/5 behavior.

**The generator still does not consume `GameLevel`.** Gameplay constants/menus live in handwritten firmware and the synchronized designer definition; sheet edits alone cannot configure them. The singular human-readable sheet is not the proposed normalized `GameLevels` schema below. Blank time/sound cells specify no new value, not automatic behavior. The implementation leaves the workbook, generated headers and user-owned handoff unchanged.

#### Implemented game placement and Sorrend rules

| Game | Menu level | Implemented rules |
| --- | --- | --- |
| Random | 1 | Difficulty 1; no new timing or sound specified |
| Random 2 | 1 | Difficulty 1; no new timing or sound specified |
| Cyclon | 2, 3, 4, 5 | Four internal difficulties per menu level; see table below |
| Sorrend (memory) | 2 | White sequence lights; no distractors; no input time limit; no warning specified |
| Sorrend (memory) | 3 | White sequence lights; 4–6 random saturated-colored distractors; 3 seconds per hit; warnings at 2 and 1 seconds remaining |
| Sorrend (memory) | 4 | White sequence lights; 4–10 random saturated-colored distractors; 2 seconds per hit; warnings at 1 and 0.5 seconds remaining |
| Sorrend (memory) | 5 | White sequence lights; 8–10 random saturated-colored distractors; 1.2 seconds per hit; warnings at 1 and 0.5 seconds remaining |
| Sima villogo | 2 | Difficulty 1; random target color; 10-second input time limit |

All Sorrend variants are **1–32-step memory games with white targets**, including levels4/5. Distractor counts are additional to the target, distinct and excluding it; at most ten plus one target fit the eleven channels. `RANDOM_SATURATED` excludes white. Times are per required correct input, reset at each Input entry (including after540 ms correct feedback), not a whole-sequence or menu deadline. Deadline wins at the boundary. Preserve fresh any-panel start,500 ms intro/show,200 ms gap, playback-release gating,540 ms green/Win per correct answer and740 ms red/Loss for wrong/chord/deadline; max32/failure returns Ready.

**Explicit implementation choices where the workbook is unspecified:** distractors refresh positions/colors on **each shown step**, stay fixed within that Showing, and appear **only in Showing**. Input hides both target and distractors; gaps are dark, preserving memory play. Warnings use the existing80 ms/880 Hz effect, once at each threshold while awaiting input. These choices are source behavior, not values inferred or imported from the sheet.

#### Implemented Cyclon profiles

`MenuLevel` identifies the root submenu; internal difficulties 1–4 are the successive stages within that selected Cyclon profile. The cell's `+9` denotes a total of nine accepted positions, not an offset of nine.

| Menu level | Internal difficulty | Step interval per LED | Accepted offset from centre | Total accepted LEDs |
| --- | --- | --- | --- | --- |
| 2 | 1 | 50 ms | −4…+4 | 9 |
| 2 | 2 | 40 ms | −2…+2 | 5 |
| 2 | 3 | 40 ms | −2…+2 | 5 |
| 2 | 4 | 40 ms | −1…+1 | 3 |
| 3 | 1 | 40 ms | −4…+4 | 9 |
| 3 | 2 | 35 ms | −2…+2 | 5 |
| 3 | 3 | 30 ms | −2…+2 | 5 |
| 3 | 4 | 30 ms | −1…+1 | 3 |
| 4 | 1 | 50 ms | −2…+2 | 5 |
| 4 | 2 | 40 ms | −1…+1 | 3 |
| 4 | 3 | 35 ms | 0, +1 | 2 |
| 4 | 4 | 20 ms | 0, +1 | 2 |
| 5 | 1 | 40 ms | −2…+2 | 5 |
| 5 | 2 | 35 ms | −1…+1 | 3 |
| 5 | 3 | 30 ms | 0, +1 | 2 |
| 5 | 4 | 20 ms | 0 only | 1 |

The literal **centre +1, total 2** means centre plus its next higher-index LED, not symmetric ±1. Cyclon times are **milliseconds per LED step**, not Sorrend seconds per answer. Selection/reselection resets the chosen profile and waits for fresh D39; the intro, four-stage progression, score/feedback and release gating remain. No Cyclon response deadline is introduced.

#### Added challenge engines and menu bindings

[../src/game/ChallengeGames.cpp](../src/game/ChallengeGames.cpp) provides portable `Lock3`, `Lock4` and `Duel` state machines. Hardware sampling/rendering remains in application/config/LED adapters; no new task, buffer or automatic workbook import is added.

- **Kapcsolozar3/4:** SW01–05 literal GPIO HIGH mask, D40/42/38/36/37; paired strip76–80. Level3 lit→HIGH/off→LOW, unlimited; level4 inverse,2000 ms deadline from round start, deadline wins, warnings1/0.5 seconds remaining. Whole five-level match must be stable30 ms, including LOW; random displayed mask excludes the currently solved vector (inverted on4). Selection/reselection starts immediately; Win540/Loss740 then Ready, fresh D39 for another round. HIGH=physical up remains an unverified assumption, not `ActiveLevel` metadata.
- **Parbaj:** upper left channels0–2 (BTN01–03), right8–10 (BTN12–14), three distinct colors each with exactly one common from hues0/32/64/96/160. Auto-start2-second preview, warnings1/0.5 seconds remaining; afterward common-color cue at BTN09/channel5/pixel73, upper colors stay lit. No response deadline. Preview presses are ignored and require stable release. Correct wins/wrong loses; same-side chord wrong; simultaneous both correct→both green/draw, both wrong→both red/draw, mixed→correct wins. Three90 ms on/off blinks total540 ms, one Win if any answer correct else Loss; Ready requires fresh upper press.

| Menu | Action IDs in order |
| --- | --- |
| Szint1 | Random35, Random2:38 (unchanged) |
| Szint2 | Cyclon10, Sorrend36, Sima villogo37 (unchanged placement) |
| Szint3 | Cyclon42, Sorrend39, Kapcsolozar45, Parbaj47 |
| Szint4 | Cyclon43, Sorrend40, Kapcsolozar46 |
| Szint5 | Cyclon44, Sorrend41 |

All levels retain Back. Defined new callbacks `CyclonGame3/4/5`, `LockGame3/4`, `DuelGame` in [../src/ui/MenuCallbacks.cpp](../src/ui/MenuCallbacks.cpp) call application selectors; declarations/menu metadata/designer IDs must remain synchronized. Random, Random2 and Flasher gameplay is unchanged. All selected games disable menu timeout, including Ready/results; cancellation stops audio/clears100 pixels and restores normal120-second timeout. The same two application tasks serve every engine.

### Optional future normalized schema

For a future migration to machine-readable workbook-only gameplay configuration, the earlier normalized proposal is:

| Sheet | Purpose | Required columns |
| --- | --- | --- |
| `GameCatalog` | One row per game, stable identity and display metadata | `GameId`, `DisplayName`, `Engine`, `Enabled`, `LevelCount` |
| `GameLevels` | One row per level, pure gameplay tuning | `GameId`, `Level`, `MoveIntervalMs`, `HitRadius`, `TargetMin`, `TargetMax`, `IntroPauseMs`, `IntroGameStepMs`, `IntroScoreStepMs`, `FeedbackPhaseMs`, `FeedbackPhaseCount` |
| `BoardRoles` | Explicit board-owned semantic roles that are not derivable from raw IDs alone | `Role`, `ControlId` |

### Sheet conventions

- `ControlId` is the stable physical identifier, for example `BTN01`, `JOY01`, `LED01`.
- `GameId` is the stable firmware identifier, for example `CYCLON`.
- `Level` accepts `*`, `1`, `1-4`, `1,3,5`, or combined expressions such as `1-2,4`.
- `Action` is a symbolic role, not a pin number. Examples: `Start`, `Action`, `Pause`, `Confirm`, `Left`, `Right`.
- `BoardRoles` should hold singleton runtime roles such as `GameButton`, `MenuButton`, `MenuJoystickY`, `LedData`, `Buzzer`, `LcdI2cAddress`. This avoids inferring semantic ownership from free-form descriptions.
- A multi-contact digital input may contain a comma-separated GPIO list. `25,24 (3 állású)` exports D25 and D24, not a third pin numbered 3. The three positions are not interpreted by the current runtime.
- Ikarus illuminated switches use paired identifiers `BTNnn_ikarus_N` (`DigitalIn`) and `DIOnn_ikarus_N` (`DigitalOut`). The shared numeric suffix identifies the physical unit; row order and the BTN/DIO prefix number do not determine the pairing.
- A switch contact and its built-in LED remain separate electrical controls. Pairing does not implement automatic matching Ikarus lamp switching. Random accepts safe configured contacts, including Ikarus, but chooses one of the 11 strip panel lamps independently. An Ikarus LED must not overlap a board-owned pin, a digital input or another Ikarus LED.

### ActiveLevel and general lamp pairs (2026-10-15)

`PinMap_export` now also supplies `ActiveLevel`: HIGH/LOW for transistor-driven GPIO lamps, WS2812B for the shared strip, blank for unspecified. Invalid nonblank values and active levels on non-output rows are rejected. Only HIGH/LOW outputs can be driven by Digitalis ki or temporarily held inactive by Gombok. All six current GPIO lamps are HIGH-active; LOW therefore means OFF.

Hyphenated BTN/SW inputs and LED/DIO outputs pair by normalized suffix, independently of prefix numbers. `BTN01-left-blue_1` pairs with `LED01-left-blue_1`; `BTN07-down-5_1` pairs with `LED03-strip-down5_1`. Strip-prefix removal and punctuation normalization cover that workbook spelling. Missing, duplicate and incorrectly typed pairs fail generation. The current export has 22 pairs. Legacy `_ikarus_N` IDs remain supported.

Generated `PinDefinition` includes `ActiveLevel activeLevel` and `uint8_t stripPixel`; `LampPairDefinition { inputControl, lampControl }`, `LampPairs` and `LampPairCount` extend the model below. Existing `IkarusSwitches` remains available. Pair identity and strip position are separate: runtime panel channels resolve a button through `LampPairs` to its lamp's pixel; pairing alone does not trigger lighting.

### Panel strip positions from existing Info values

The panel implementation leaves the Excel workbook **unchanged**. Numeric values in column B (`Info`) of `PinMap_export`, on WS2812B lamp rows, supply **1-based** positions. The generated `PinDefinition::stripPixel` is **0 when unspecified or not a strip lamp**. Descriptive text and range strings are skipped, not expanded or interpreted as GPIO/pixel numbers. Numeric positions are validated as **1–100 and unique**; invalid/duplicate numeric values fail generation.

The exact existing layout, resolved by lamp name through `LampPairs`, is:

| Input controls in logical order | Lamp strip positions, 1-based |
| --- | --- |
| BTN01, BTN02, BTN03 | 84, 85, 86 |
| BTN07, BTN08, BTN09, BTN10, BTN11 | 71, 72, 73, 74, 75 |
| BTN12, BTN13, BTN14 | 83, 82, 81 |

The table above describes the **raw workbook pairs**, not corrected physical gameplay output. The user reported the bottom row appeared reversed. The [panel adapter](../src/config/PanelConfig.h) therefore reverses only the five lower lamp associations: **BTN07–11 → 75, 74, 73, 72, 71**. It preserves input order, raw generated positions and both upper groups (BTN01–03 → 84/85/86; BTN12–14 → 83/82/81). Do not sort or rewrite the workbook to implement this correction. All 11 lamps require **at least 86 physical strip pixels**. D4 remains the one shared data pin, not an individual lamp position; the fixed buffer remains 100 pixels. Cyclon still uses 75 pixels and clears its tail.

## Generated C++ model

The generator emits two headers into `src/generated/`.

### `pins.h`

Raw hardware inventory and workbook-derived board constants.

```cpp
namespace generated {
namespace pins {

enum class ControlId : uint8_t;
enum class PinType : uint8_t;
enum class ActiveLevel : uint8_t;

struct PinDefinition {
    uint8_t primaryPin;
    uint8_t secondaryPin;
    uint8_t tertiaryPin;
    PinType type;
    ActiveLevel activeLevel;
    uint8_t stripPixel; // 1-based; 0 unspecified or not a strip lamp.
};

struct IlluminatedSwitchDefinition {
    uint8_t number;
    ControlId inputControl;
    ControlId ledControl;
};

struct LampPairDefinition {
    ControlId inputControl;
    ControlId lampControl;
};

constexpr PinDefinition PinTable[] = { ... };
constexpr IlluminatedSwitchDefinition IkarusSwitches[] = { ... };
constexpr uint8_t IkarusSwitchCount = ...;
constexpr LampPairDefinition LampPairs[] = { ... };
constexpr uint8_t LampPairCount = ...;
constexpr uint8_t LedDataPin = ...;
constexpr uint8_t BuzzerPin = ...;
constexpr uint8_t MenuButtonPin = ...;
constexpr uint8_t MenuJoystickXChannel = ...;
constexpr uint8_t MenuJoystickYChannel = ...;
constexpr uint8_t LcdI2cSdaPin = ...;
constexpr uint8_t LcdI2cSclPin = ...;
constexpr uint8_t LcdI2cAddress = ...;

}
}
```

The emitted table is intentionally numeric and compact. Human-readable descriptions stay in the workbook or generated docs, not in AVR RAM.

Digital multi-contact rows use `secondaryPin`/`tertiaryPin` for additional GPIO contacts. Composite joystick rows instead use these fields for the X/Y ADC channels; LCD rows use the primary/secondary fields for SDA/SCL. Analog-only rows contain an ADC channel, not a digital GPIO number.

The current workbook maps the strip to D4, the Cyclon button to D39, `POT1` to A15 and `SW06` to D25/D24. Six Ikarus input/LED pairs are listed in [../README.md](../README.md). Boot-time pull-ups follow safe exported digital inputs instead of a fixed GPIO range; reserved roles still take precedence over the workbook's raw `Type` label (for example, `BUZ01` remains the buzzer output).

### `game_config.h`

Game identity, per-level bindings and optional per-level gameplay specs.

```cpp
namespace generated {
namespace games {

enum class GameId : uint8_t;
enum class ControlRole : uint8_t;

struct ControlBinding {
    GameId game;
    uint32_t levelMask;
    pins::ControlId control;
    ControlRole role;
};

struct LevelConfig {
    GameId game;
    uint8_t level;
    uint16_t moveIntervalMs;
    uint8_t hitRadius;
    uint8_t targetMin;
    uint8_t targetMax;
    uint16_t introPauseMs;
    uint16_t introGameStepMs;
    uint16_t introScoreStepMs;
    uint16_t feedbackPhaseMs;
    uint8_t feedbackPhaseCount;
};

struct GameDefinition {
    GameId game;
    uint8_t bindingOffset;
    uint8_t bindingCount;
    uint8_t levelOffset;
    uint8_t levelCount;
};

constexpr ControlBinding ControlBindings[] = { ... };
constexpr LevelConfig LevelConfigs[] = { ... };
constexpr GameDefinition GameDefinitions[] = { ... };

}
}
```

This shape keeps runtime access simple:

- pick a `GameDefinition`
- scan only that game's bindings or level rows
- resolve behavior with `constexpr` helpers

No heap allocation, no string-key lookup and no parser state are needed on the microcontroller.

## Namespace layout

- `generated::pins`: raw workbook-derived physical inventory
- `generated::games`: raw workbook-derived gameplay/control catalog
- `board`: stable firmware-facing aliases and assertions for board-wide constants
- `game`: runtime logic and state machines that consume resolved configuration

This keeps generated files isolated from handwritten application code while still allowing selective compatibility wrappers during migration.

## Python generator

`tools/generate_config.py` is responsible for:

1. reading `.xlsx` directly through ZIP/XML parsing from the Python standard library
2. validating required columns, duplicate IDs, complete/typed lamp pairs, active levels, safe GPIO lamp assignments and unique numeric strip positions in 1–100
3. parsing composite pin cells such as `d53, x=a8, y=a9` and multi-contact cells such as `25,24 (3 állású)`
4. converting level expressions to compact bitmasks
5. rendering deterministic C++ headers with stable enum ordering, suffix-based lamp associations and 1-based strip positions from numeric `Info`
6. optionally rendering Markdown documentation from the same workbook data

The generator is designed so that the build, the host tests and a manual CLI invocation all go through the same code path.

## Project structure

```text
Gyerek_jatek_3.0.xlsx
tools/
  generate_config.py
src/
  generated/
    pins.h
    game_config.h
  config/
    PinConfig.h
docs/
  EXCEL_CONFIG_ARCHITECTURE.md
```

## Migration notes

- Today the workbook is already authoritative for exported pin metadata and control bindings.
- Current Cyclon, panel and challenge rules remain source constants; corrected human-readable `GameLevel` requirements were manually implemented, not imported. A normalized machine-readable `GameLevels` migration is still future work. Root Szint1–5 are menu groups, not workbook level masks or Cyclon's four internal difficulties.
- Once `GameLevels` and `BoardRoles` exist, `CycloneGame` can move to generated `LevelConfig` rows and the last handwritten semantic board roles can disappear.

## Current runtime consumers and evidence (implementation session 2026-10-20)

- [../src/config/PanelConfig.h](../src/config/PanelConfig.h) resolves corrected panel lamps and metal-switch identities from generated pin/pair metadata. Cyclon, panel and challenge engines consume source profiles. Random2 uses paired strip/GPIO lamps; original Random does not drive matching Ikarus lamps.
- Boot selects no game and leaves the strip dark. Cyclon waits for fresh D39; Sorrend/Flasher consume a fresh any-panel start. Only new Kapcsolozar/Parbaj first selection/reselection autostarts. Manual owning-level exit, another game/menu, Debug and restart cancel old game/audio/100 pixels; no Debug autoresume. Selection disables tcMenu reset logic throughout gameplay/Ready/results; cancellation restores normal120-second menu/Debug timeout. Gameplay deadlines and LED Debug's timer are separate.
- **Current full normal and undefined-behavior-sanitized suites: 157 native tests / 36 Python checks passed each; Mega build successful, static SRAM 4,356 / 8,192 bytes, flash 67,936 / 253,952 bytes.** No upload or hardware verification. Workbook, generated headers and user-owned handoff unchanged.
- **Historical evidence only:** 2026-10-15:115 native/32 Python passed (build was pending at that checkpoint); 2026-10-19 Random2:126/33 normal/sanitized passed, SRAM4123/flash62336; earlier Sorrend:135/34 normal/sanitized passed, SRAM4168/flash63386. None is sanitizer/SRAM evidence for the current revision. Device acceptance remains open in [HARDWARE_TEST_CHECKLIST.md](HARDWARE_TEST_CHECKLIST.md).
