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

## Generated C++ model

The generator emits two headers into `src/generated/`.

### `pins.h`

Raw hardware inventory and workbook-derived board constants.

```cpp
namespace generated {
namespace pins {

enum class ControlId : uint8_t;
enum class PinType : uint8_t;

struct PinDefinition {
    uint8_t primaryPin;
    uint8_t secondaryPin;
    uint8_t tertiaryPin;
    PinType type;
};

constexpr PinDefinition PinTable[] = { ... };
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
2. validating required columns and duplicate IDs
3. parsing composite pin cells such as `d53, x=a8, y=a9`
4. converting level expressions to compact bitmasks
5. rendering deterministic C++ headers with stable enum ordering
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
- The current firmware still keeps Cyclon timing constants in source because the workbook does not yet provide a dedicated `GameLevels` sheet.
- Once `GameLevels` and `BoardRoles` exist, `CycloneGame` can move to generated `LevelConfig` rows and the last handwritten semantic board roles can disappear.
