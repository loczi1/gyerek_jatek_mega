# Gyerek_jatek — session handoff

**Checkpoint date:** 2026-10-15

**Purpose:** Resume development without repeating the previous project review and refactor.

## Status and evidence

- The user reported on **2026-10-11**: "ok so far. I have tested." Treat this as a user-confirmed hardware checkpoint, not proof that every regression scenario was exercised. Board identity, uploaded revision, physical strip length and detailed test results were not supplied.
- The project now includes a workbook-driven configuration pipeline: `tools/generate_config.py` regenerates `src/generated/pins.h` and `src/generated/game_config.h` from `Gyerek_jatek_3.0.xlsx` before project checks and host tests. The current firmware consumes the generated pin metadata where the workbook is already unambiguous, while Cyclon gameplay timings remain in source until the workbook adds the dedicated level-definition sheet documented in `docs/EXCEL_CONFIG_ARCHITECTURE.md`.
- The **2026-10-14** workbook updates move strip data to D4, the Cyclon button to D39, `POT1` to A15 and both `SW06` contacts to D25/D24. Six Ikarus units now have separate, generated input/LED associations. Safe workbook digital inputs receive startup pull-ups; LED testing remains manual, with no automatic switch-to-light behavior. The new wiring still needs hardware acceptance.
- The **2026-10-15** workbook adds `ActiveLevel` (six GPIO lamps HIGH-active, strip entries WS2812B) and hyphenated control IDs. The generator exports 22 suffix-based `LampPairs`, including normalized `down-5_N` / `strip-down5_N`, and retains the six legacy-compatible `IkarusSwitches`. Pairing does not automatically light lamps.
- Gombok temporarily drives free, polarity-defined GPIO lamps inactive: current D2/D3/D16/D17/D18/D19 LOW. The fixed 65 rows remain; Tesztelt is normally 55. Back, mode change and timeout restore test-owned lamp input/pull-up settings. Pre-existing outputs and pins whose mode/level changed under another owner are not overwritten. Strip D4 remains exclusively FastLED-owned.
- Latest automated verification, **2026-10-15**: Mega build passed; **104 native regression tests and 28 Python project/config checks passed**. The first implementation build ran before new tests were generated, following the user's lower-token/build-first preference. Previous checkpoint's sanitizer evidence is not evidence for this new revision.
- Recorded build usage: **3,190 / 8,192 bytes static SRAM** and **55,986 / 253,952 bytes flash**. Static SRAM excludes runtime heap and stack. These figures belong to that build, not a future build.
- No firmware was uploaded in this session, and no current hardware acceptance tests were performed. Existing staged work and the user's workbook edits were preserved; new source/documentation changes were not staged or committed. After later code changes, rerun validation and record the new results rather than carrying these numbers forward.
- At handoff creation, branch `main` pointed to `f9ea894` and the worktree contained modified and untracked feature files. The tested development state was not wholly represented by HEAD. Inspect current Git status; do not discard or automatically commit that work.

## Documentation map

- [../AGENTS.md](../AGENTS.md): concise repository-wide coding guidance.
- [../README.md](../README.md): architecture, wiring, configuration, safety and regeneration details.
- [../HASZNALATI_UTMUTATO.md](../HASZNALATI_UTMUTATO.md): Hungarian end-user instructions.
- [HARDWARE_TEST_CHECKLIST.md](HARDWARE_TEST_CHECKLIST.md): repeatable device acceptance checklist with an evidence template.

## Current user-facing behavior

Root menu has exactly **Jatek**, **Ujrainditas**, **Debug**. No empty JatekValaszto/Input naplo entries or selectable Greensleeves music remain.

| Feature | Current behavior |
| --- | --- |
| Cyclon | Four levels, 70 game LEDs plus five score LEDs; movement intervals 50/40/35/20 ms. Debounced press edges prevent held-button reuse. |
| Result sounds | Win: rising chime, 540 ms. Loss: descending pattern, 740 ms. Each plays once beside three timed LED flashes. |
| Joystick | A9; short movement gives one step, hold waits 500 ms then repeats once per 300 ms. No acceleration or missed-step replay. Neutral is required after menu/select changes. |
| Normal menu timeout | 120 seconds without joystick/menu-button interaction; live diagnostics and tested buttons do not renew it. |
| Debug entry/exit | Stops the game and sounds while Debug or its children are open. Returning to root restores Cyclon idle; a held game button must be released before starting. |
| Gombok | 65 fixed rows: four summaries plus 61 candidates; six free GPIO lamp pins are held LOW and show OUT, normally leaving 55 tested inputs. Current/last presses use D/A names. Scoped lamps and added pull-ups are restored on exit. |
| LED szalag | LED db selects 1–100 pixels, default 100. Inditas starts one white pixel. Short joystick-button click advances; long press/release exits and clears. LCD positions are 1-based. |
| LED test timeout | Own 120-second inactivity timer, renewed by step clicks. Clears the strip and returns to root. Directional joystick movement is ignored on the test screen. |
| Digitalis ki | Six Ikarus LED outputs, in GPIO order: D2, D3, D16, D17, D18, D19. HIGH/LOW drives the selected pin; changing selection or exiting restores the driven pin to LOW input mode. |
| Digitalis / Analog | Cached D0–D53 digital states and A0–A15 raw ADC values. Reserved/output pins are identified rather than repurposed. |
| Allapot / Hibak | Application state, game/audio task scheduling versus fallback, and LCD ACK/I2C error/timeout status. Not an exhaustive hardware self-test. |

## Configuration and implementation authorities

| Topic | Source |
| --- | --- |
| Pin assignments / Mega GPIO mapping | [../src/config/PinConfig.h](../src/config/PinConfig.h) |
| Task intervals, menu timeout, I2C timeout, power estimate | [../src/config/BoardConfig.h](../src/config/BoardConfig.h) |
| Joystick thresholds/repeat and 4 Hz LCD refresh | [../src/config/NavigationConfig.h](../src/config/NavigationConfig.h) |
| 100-pixel strip capacity/default | [../src/config/LedConfig.h](../src/config/LedConfig.h) |
| Task orchestration, Debug scope and LED-screen ownership | [../src/app/Application.cpp](../src/app/Application.cpp) |
| Pure game/Debug selection and result-sound triggering | [../src/app/ModeController.cpp](../src/app/ModeController.cpp) |
| Game timing and win boundaries | [../src/game/CycloneGame.cpp](../src/game/CycloneGame.cpp) |
| GPIO button testing / pull-up lifecycle | [../src/debug/ButtonTester.cpp](../src/debug/ButtonTester.cpp), [../src/hardware/Diagnostics.cpp](../src/hardware/Diagnostics.cpp) |
| Manual LED stepping / shared strip buffer | [../src/debug/LedStripTester.cpp](../src/debug/LedStripTester.cpp), [../src/hardware/LedDisplay.cpp](../src/hardware/LedDisplay.cpp) |
| Safe display and deterministic input | [../src/ui/SafeLiquidCrystalRenderer.cpp](../src/ui/SafeLiquidCrystalRenderer.cpp), [../src/ui/MenuInput.cpp](../src/ui/MenuInput.cpp) |
| Menu definition / callback implementations | [../Gyerek_jatek_arduinomega.emf](../Gyerek_jatek_arduinomega.emf), [../src/ui/MenuCallbacks.cpp](../src/ui/MenuCallbacks.cpp) |
| Build and test configuration | [../platformio.ini](../platformio.ini), [../.vscode/tasks.json](../.vscode/tasks.json), [../tools/run_tests.py](../tools/run_tests.py) |

## Important constraints and lessons

- Hardware: D4 strip data, D5 passive buzzer, D39 game button, D53 menu button, A9 joystick, A15 potentiometer, LCD 0x27 on Mega D20/D21. The game button follows the workbook `GameControls` binding (`CYCLON` -> `BTN15`). `SW06` retains both D25 and D24, not the annotation's position count. LCD constructor numbers 0–7 are **PCF8574 bits**, not conflicting Mega pins.
- Gombok excludes D0/D1, strip D4, D5, D13, D20/D21, D53, A9 and any configured output. Mega analog GPIO maps A0 to 54 through A15 to 69; sets/debounce storage cover all 70 pins. Safe workbook input contacts have permanent startup pull-ups; extra pull-ups are scoped to Gombok and restored on exit. The fixed list remains 65 rows.
- Digitalis ki consumes workbook `PinType::DigitalOut` plus HIGH/LOW `ActiveLevel` metadata from [../src/generated/pins.h](../src/generated/pins.h), exposing only normal-role `Input` GPIO. WS2812B/unspecified entries are excluded. LOW/HIGH remain literal GPIO commands; inactive level follows polarity. LampPairs associates all 22 named input/lamp units; IkarusSwitches retains six numeric pairs. Generator validation rejects incomplete/incorrect pairs and reserved/shared transistor output assignments.
- There are **two permanent 5 ms application tasks**, not OS threads. Menu/input framework work is additional; new feature callbacks must not accumulate recurring tasks.
- The old joystick's acceleration plus a 2 Hz LCD made successive steps look like jumps. The current deterministic input and 4 Hz display must stay aligned. ADC readers discard the first conversion after switching channels.
- One FastLED controller owns the workbook strip pin, now D4, with a fixed **100 CRGB** buffer. The game stays at 75 pixels; the extra 25 remain black in game frames. D2 is a separate Ikarus LED output, not another strip controller.
- tcMenu display takeover **bypasses its normal inactivity countdown**. The LED test supplies its own timeout and explicitly gives back the display. Giving back the display alone does not navigate to root.
- LED-screen clicks are intercepted at input delivery, not sampled as persistent render-button flags; this avoids lost or repeated steps. LCD writes can yield, so screen dirty flags are cleared before writing and may be set again by new input.
- Setting tcMenu display updates per second resets its normal interval to 30 seconds. The 120-second override is applied after menu setup, outside generated source.
- Designer regeneration can replace the menu pair. Preserve safe-renderer and project-owned input bindings, matching item IDs/callbacks/counts and the LED count offset (+1). Build preflight checks detect these regressions.
- Floating inputs, permanent ground connections and external pull-downs cannot automatically be distinguished from sensor/button faults. The 1000 mA software estimate does not certify a power supply. AVR WS2812B transmission can briefly affect timer-driven sound.
- Standalone Greensleeves/player code remains for future reuse and host tests; it is not referenced by the active application and was absent from the linked firmware at the last verification.

## Next-session workflow

1. Read this checkpoint and repository guidance, then inspect current Git status and the source relevant to the new request.
2. For firmware/menu changes, prefer implementation → **Build Gyerek_jatek (Mega)** → focused new tests → **Test Gyerek_jatek (no hardware)** when it reduces token cost. The runner compiles actual portable modules, not a copy of their logic. Keep explanations/output concise; still test changed behavior and rebuild after later source edits.
3. Native tests use g++/clang++/Zig; the optional portable compiler is pinned in [../tools/requirements-test.txt](../tools/requirements-test.txt). Do not confuse host tests with a serial upload.
4. If hardware behavior changes, run the relevant device checklist cases and record the actual firmware revision, configuration and evidence. Upload only when requested.
5. Update this dated checkpoint and the user guide when facts change. No new feature backlog was requested at this checkpoint; wait for the next user requirement rather than inventing work.
