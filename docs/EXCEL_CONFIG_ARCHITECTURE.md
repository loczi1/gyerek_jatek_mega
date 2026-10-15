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

To complete the migration to a true workbook-only gameplay configuration, add these normalized sheets:

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
- The current firmware still keeps Cyclon and Random/Simon timing constants in source because the workbook does not yet provide a dedicated `GameLevels` sheet. Root Szint1–5 are menu groups, not workbook level masks or Cyclon's four internal difficulties.
- Once `GameLevels` and `BoardRoles` exist, `CycloneGame` can move to generated `LevelConfig` rows and the last handwritten semantic board roles can disappear.

## Current runtime consumers and evidence (2026-10-15)

- Root is **Szint1–Szint5, Ujrainditas, Debug**. Szint1 offers Random; Szint2 offers Cyclon and Simon mondja; Szint3–5 are intentionally Back-only by user request.
- Boot selects no game and leaves the strip dark. Cyclon selection waits for fresh D39; its four internal difficulties are unchanged. Random re-selection resets it; each fresh debounced safe configured input/switch press chooses one of 11 lamps, a random hue and a 220–1760 Hz/250 ms tone. Menu/peripheral inputs are excluded; matching Ikarus GPIO lamps are not switched automatically.
- Simon selection/re-selection resets to Ready. A fresh press of any of its 11 buttons starts and is consumed; random sequences grow from 1 to 32. Initial pause/light are 500 ms, gaps 200 ms. Playback-held input is ignored until release; answers repeat the same buttons. Correct presses give 540 ms green plus Cyclon Win; wrong/chord or 10-second input timeout gives 740 ms red plus Cyclon Loss. Failure/max-32 completion waits for a fresh start press.
- Selecting any game (Cyclon, Random or Simon) disables tcMenu automatic return entirely via `renderer.turnOffResetLogic()`, including ready, gameplay and results. After completion/failure the game remains selected with no menu timeout. Manually leaving the owning level submenu (Back/root or another menu), Debug and restart cancel the game, tone and all 100 LEDs and restore the normal 120-second menu/Debug timeout. Simon's 10-second answer timeout and the LED Debug screen's own 120-second timer are unchanged. Debug exit does not resume a game. All modes reuse the existing two application tasks.
- The latest full suite passed **115 native tests and 32 Python checks**. The final firmware build is pending; no final build result or size is claimed here. The new behavior remains **hardware untested**; **no firmware was uploaded** for this update.
