# Gyerek_jatek

Arduino Mega 2560 LED reaction game with a 16x2 tcMenu LCD interface, win/loss buzzer sounds and live input diagnostics. Open [Gyerek_jatek.code-workspace](Gyerek_jatek.code-workspace); it contains only this project, not the unrelated Csiraztato project.

Magyar nyelvű felhasználói útmutató: [HASZNALATI_UTMUTATO.md](HASZNALATI_UTMUTATO.md).

Workbook-driven configuration design and generated artifacts: [docs/EXCEL_CONFIG_ARCHITECTURE.md](docs/EXCEL_CONFIG_ARCHITECTURE.md).

## Future development sessions

- [AGENTS.md](AGENTS.md): repository-wide guidance for coding assistants.
- [docs/SESSION_HANDOFF.md](docs/SESSION_HANDOFF.md): dated checkpoint, implemented behavior, validation evidence and next-session workflow.
- [docs/HARDWARE_TEST_CHECKLIST.md](docs/HARDWARE_TEST_CHECKLIST.md): device regression checklist and a place to record actual hardware results.

## Firmware structure

| Module | Responsibility |
| --- | --- |
| [src/main.cpp](src/main.cpp) | Arduino setup/loop entry point |
| [src/app/Application.cpp](src/app/Application.cpp) | Hardware initialization and two permanent 5 ms tasks: game/input and audio |
| [src/app/ModeController.cpp](src/app/ModeController.cpp) | Cyclon/Debug selection and one-shot result sounds; no task allocation |
| [src/audio/SoundEffects.cpp](src/audio/SoundEffects.cpp) | Flash-backed, non-blocking win/loss buzzer patterns |
| [src/game/CycloneGame.cpp](src/game/CycloneGame.cpp) | Portable game state machine, movement, hit windows, intro and feedback timers |
| [src/game/DebouncedButton.h](src/game/DebouncedButton.h) | One event per debounced press, with rollover-safe timing |
| [src/hardware/LedDisplay.cpp](src/hardware/LedDisplay.cpp) | One 100-pixel D2 buffer shared by the 75-pixel game and strip test |
| [src/hardware/Diagnostics.cpp](src/hardware/Diagnostics.cpp) | Mega register/ADC adapter and scoped button pull-ups; preserves outputs and PWM |
| [src/debug/InputMonitor.cpp](src/debug/InputMonitor.cpp) | Cached, bounded digital/analog scanning and input labels |
| [src/debug/ButtonTester.cpp](src/debug/ButtonTester.cpp) | Debounced pressed-pin detection, last-press capture and reversible input pull-ups |
| [src/debug/LedStripTester.cpp](src/debug/LedStripTester.cpp) | Manual one-pixel strip stepping, length limits, LCD position and idle timeout |
| [src/debug/SystemChecks.cpp](src/debug/SystemChecks.cpp) | Game/audio task health and LCD I2C error reporting |
| [src/ui/MenuInput.cpp](src/ui/MenuInput.cpp) | Static tcMenu encoder; deterministic A9 input polled by the existing task |
| [src/ui/JoystickRepeat.cpp](src/ui/JoystickRepeat.cpp) | Portable joystick hysteresis, debounce, neutral rearm and fixed repeats |
| [src/ui/MenuCallbacks.cpp](src/ui/MenuCallbacks.cpp) | Application-owned tcMenu callbacks |
| [src/ui/SafeLiquidCrystalRenderer.cpp](src/ui/SafeLiquidCrystalRenderer.cpp) | Project-owned LCD renderer with clipped text and cursor bounds |
| [src/ui/LcdText.cpp](src/ui/LcdText.cpp) | Portable, tested LCD text composition |

## Excel configuration pipeline

- `Gyerek_jatek_3.0.xlsx` is the workbook source for generated configuration artifacts.
- `tools/generate_config.py` parses the workbook with Python's standard library only, so the PlatformIO build does not need JSON parsing or extra Python packages.
- The build regenerates [src/generated/pins.h](src/generated/pins.h) and [src/generated/game_config.h](src/generated/game_config.h) before the normal project checks run.
- The generated headers are constexpr-only data tables intended for flash-friendly firmware consumption. They currently cover the exported pin catalog and per-game control bindings from the workbook.
- The long-term sheet layout for multi-game and per-level gameplay settings is documented in [docs/EXCEL_CONFIG_ARCHITECTURE.md](docs/EXCEL_CONFIG_ARCHITECTURE.md). The current firmware still keeps Cyclon timing constants in source until the workbook gains the dedicated level-definition sheet described there.

## Wiring

The existing hardware assignments are unchanged. Pin ownership is centralized in [src/config/PinConfig.h](src/config/PinConfig.h); hardware timing and power limits are in [src/config/BoardConfig.h](src/config/BoardConfig.h).

| Connection | Mega pin / setting |
| --- | --- |
| WS2812B LED data | D2; GRB. Game uses 70+5 LEDs; Debug tests a selected length of 1-100 LEDs |
| Passive buzzer | D5 |
| Game push button | D51 to GND, internal pull-up, active LOW; derived from workbook `CYCLON` `Start` / `Action` -> `BTN15` |
| Button Debug inputs | Safe D0-D53 and A0-A15 GPIO, excluding A9 joystick; button to GND; temporary pull-ups in Gombok. Existing pull-ups are preserved |
| Menu select button | D53 to GND, configured by tcMenu |
| Menu joystick Y | A9; ADC centre 512, engage distance 140, release distance 80 |
| Analog Debug view | A0-A15, including menu joystick A9 |
| Optional randomness input | A1, preferably unconnected; mixed with first-player timing |
| LCD | PCF8574 at I2C address 0x27; Mega SDA D20 and SCL D21 |
| Status output | On-board LED D13; reserved, not driven by Debug |

LCD constructor pin numbers 0-7 refer to **PCF8574 bits**, not Mega pins, after the I/O abstraction is selected. There is no LCD/FastLED D2 conflict in this configuration.

Use an appropriate external 5 V supply for the strip, a common ground, and suitable data-line protection. FastLED's configured 1000 mA software estimate helps cap brightness; it does not verify or protect the physical power supply. Actual supply, wiring, joystick calibration, LCD backpack mapping and buzzer operation require on-device checks.

## Behavior

- The main menu has exactly **Jatek**, **Ujrainditas** and **Debug**. Jatek is an action for the only available game, **Cyclon**, not a game/music selector. Selecting Jatek while a game is already active does not reset the round.
- Default mode: **Cyclon**, ready at the rainbow screen. Press D51 once to start the non-blocking intro wipe (500 ms pause, 70 x 40 ms game pixels, 5 x 100 ms score pixels).
- A held button is never reused to stop a level, restart after a loss, or start a game on boot. Release it before pressing again.
- Level speeds are 50, 40, 35 and 20 ms per pixel; the task polls every 5 ms rather than imposing the old 50 ms limit. Actual timing can have small scheduling and LED-transmission jitter.
- Level 1 accepts the five-pixel target window, level 2 accepts three pixels, and levels 3/4 accept only the centre. The checked position is the displayed LED, not the next index.
- Wins/losses flash green/red three times, 500 ms on and 500 ms off, without delaying menu or application dispatch. Winning advances to the next level; finishing level 4 or losing returns to idle.
- Each win plays a short rising chime (523, 659, 784 Hz; about 0.54 seconds); each loss plays a descending sound (392, 294, 196 Hz; about 0.74 seconds). Each result starts its sound **once**, alongside the LED flashes, not once per flash or while a button is held.
- Four score LEDs indicate the current difficulty level; the fifth lights during final-victory feedback. All five are cleared on the next start.
- **Ujrainditas** resets Cyclon to its ready screen and cancels unfinished result sounds. Press D51 again to start a new round.
- **Debug** stops the current game and sound while any of its nested views is open, so testing D51 cannot start/stop a round. Returning to the main menu restores Cyclon at its ready screen, not mid-round. A held D51 must be released before the next start.
- Debug and nested lists automatically return to the main menu after **2 minutes without joystick/menu-button interaction**. Change MenuIdleTimeoutSeconds in [src/config/BoardConfig.h](src/config/BoardConfig.h); [src/app/Application.cpp](src/app/Application.cpp) applies it after menu setup so tcMenu's default 30-second interval is overridden. Live input refreshes do not extend this timeout. This is separate from the 25 ms I2C bus timeout.
- JatekValaszto, Input naplo, the old test placeholders and Greensleeves selection are removed from the menu. The standalone [src/music/MusicPlayer.cpp](src/music/MusicPlayer.cpp) and [src/music/Melody.cpp](src/music/Melody.cpp) modules remain for future reuse/tests but are not referenced by the active firmware application.
- Diagnostics are intentionally volatile; no EEPROM addresses are assigned or written.

All application timers use unsigned elapsed-time subtraction for millis rollover. Button sampling, music notes, diagnostic formatting and LED rendering do not allocate Arduino String objects. The frameworks may still allocate their own menu/task storage.

## Predictable menu navigation

The old analog-joystick acceleration is disabled. A short movement produces **one step** after 30 ms of stable input. Holding a direction starts single-step repeats after **500 ms**, then repeats **once every 300 ms**, independent of how far the joystick is pushed. Returning to centre stops repeats; a delayed task never replays missed steps.

The LCD refreshes at **4 Hz**, faster than held repeats, rather than hiding several changes between frames. A9 is sampled every 10 ms with a discarded first ADC conversion, plus hysteresis around the centre. Entering/leaving a menu, auto-return, and pressing the menu/select button rearm navigation only after the joystick has returned to neutral. tcMenu remains the owner of the absolute cursor and its bounds; there is no separate stale cursor or new recurring joystick task.

Adjust the centre, thresholds, debounce, hold delay, repeat interval and display rate in [src/config/NavigationConfig.h](src/config/NavigationConfig.h). Hardware joystick centre/voltage still needs on-device verification if it does not settle near 512.

## Debug menu

- **Gombok:** identifies buttons wired between a safe GPIO input and GND. It covers **61 candidates: 46 digital pins plus 15 analog pins used as digital inputs**. UART D0/D1, LED D2, buzzer D5, status output D13, I2C D20/D21, menu/select D53 and analog joystick A9 are excluded. Any additional output pin is skipped. Missing internal pull-ups are temporarily enabled; a pulled-down pin is detected after 30 ms debounce. Analog button results use **A0-A15** labels, not the underlying Mega GPIO numbers.
	- **Utolso:** most recently detected press, retained after release so short presses remain visible until the next press or test entry.
	- **Lenyomva:** currently pressed pins. **Darab:** current pressed count. **Tesztelt:** number of safe inputs participating.
	- Summaries show up to three IDs plus a **+N** overflow count; subsequent fixed rows show every candidate individually as HIGH/LOW/OUT. The list never changes size while buttons are pressed, so results do not move the cursor.
	- Leaving Gombok (Back, another view, or the two-minute auto-return) restores only pull-ups added by this test, including analog GPIO pull-ups. Existing digital/analog pull-ups remain, and a pin changed to an output by another owner is never overwritten. A9 is never modified, so navigation remains available.
- **LED szalag:** test the addressable strip on **D2**. Set **LED db** to the physical strip length (**1-100**, default 100), confirm it, then choose **Inditas**. The first LED lights white; each short **joystick/select-button click** advances the single white pixel, wrapping from the last selected LED back to the first. The LCD shows **LED 1/100**, **LED 2/100**, etc., using one-based positions.
	- While the test screen is active, up/down movements are ignored; the click is the stepper and there is no automatic movement or held-button repeat.
	- **Hold and release** the joystick/select button to exit to the strip-test menu. Two minutes without a step click clears the test and returns to the main menu. Restart, leaving the view and normal game frames also clear all test pixels.
	- The dedicated screen owns display rendering while active, so it uses its own rollover-safe inactivity timer with the same MenuIdleTimeoutSeconds setting. Clicks are handled by the input callback rather than waiting for an LCD refresh, preventing fast clicks from being lost or consumed as normal menu selections.
	- D2 has only one FastLED controller and a fixed 100-pixel buffer. Game output remains 75 pixels with the remaining tail black. Test output lights only one white pixel at moderate brightness; no new recurring task or dynamic LED-buffer allocation is created. Maximum/default count is defined in [src/config/LedConfig.h](src/config/LedConfig.h).
- **Digitalis:** scroll through D0-D53. General input pins show **LOW/HIGH**; D51 is labelled Jatek, D53 Menu. Configured outputs show **OUT**. UART pins D0/D1, LED data D2, buzzer D5, status output D13 and I2C D20/D21 are reserved and display their role instead of being converted to inputs or sampled.
- **Analog:** scroll through all **A0-A15**, showing raw 10-bit readings **0-1023**. A9 is labelled Joy. An analog pin configured as a digital output shows OUT and is not read as an ADC input. **---** means not yet sampled; ERR denotes an invalid internal sample, not an automatic diagnosis of a disconnected sensor.
- **Allapot:** current application state; Debug indicates that the game is stopped for input testing.
- **Hibak:** separate rows for **Jatek task**, **Hang task** and **LCD 0x27**. Tasks show OK when scheduled or FALLBACK when their main-loop fallback is in use. The LCD address-only probe reports OK, NINCS (no ACK), I2C HIBA or TIMEOUT. It runs on Debug entry with a 25 ms Wire timeout; subsequent Wire timeouts remain visible until the next entry/probe.

Digitalis/Analog scanning is read-only: it never switches pin modes, drives a test output, changes pull-ups or disables PWM. Gombok deliberately changes only the pull-up latch of existing safe digital/analog GPIO inputs while that test is open. Outside Gombok, the original pull-ups are restored; other unused inputs may float. A LOW reading can also mean a permanent ground connection or an external pull-down, not necessarily a mechanically pressed button. Analog voltages can be biased by the temporary pull-up during Gombok, so use the separate Analog view for voltage measurements. Never apply a voltage outside the Mega's 0-5 V input range or connect a test switch to a reserved/output pin.

The existing 5 ms task samples at most four digital pins and one analog channel per tick in Debug, plus all active button-test inputs when Gombok is open. Cached digital/analog channels refresh in about 80 ms; Gombok checks every task tick and retains short presses. The LCD redraws at 4 Hz. The first ADC conversion after switching channels is discarded to reduce cross-talk. The full analog sweep runs only in Debug; normal A9 navigation is polled in other modes except the dedicated LED-test screen, where directions are ignored. These are input observations plus task/LCD communication checks, not a full automatic hardware self-test (power, LED wiring and buzzer operation still require physical testing).

## Cooperative multitasking and the buzzer

The Mega uses **TaskManagerIO cooperative scheduling**, not OS threads or a new RTOS. The game/input task and audio task are registered once at startup; both have separate 5 ms update intervals in [src/config/BoardConfig.h](src/config/BoardConfig.h). The LCD/input framework has its own scheduled work. A task callback does a small amount of work and returns; no task waits for a note or an animation to finish. Arduino's timer-backed tone function generates the actual buzzer waveform asynchronously.

The game task detects entry into win/loss feedback and requests a sound. The audio task advances that effect independently of LED animation. Debug and restart cancel unfinished effects; no music source is selectable. If the task queue is full, each failed task has its own main-loop fallback rather than retrying allocation from menu callbacks.

Customize pitches, note lengths and silent gaps in the WinPattern and LossPattern tables in [src/audio/SoundEffects.cpp](src/audio/SoundEffects.cpp). They live in AVR flash and use no dynamic allocation. Late audio updates skip expired notes; feedback completion also cancels stale audio so it cannot carry into the next level.

This design can be extended with additional short scheduled callbacks. Keep these state machines on the cooperative loop; they are not thread-safe or intended to be called from an ISR. A future true multithreaded port would need queued commands or synchronization around shared state and buzzer ownership. WS2812B transmission can briefly mask timer interrupts on AVR, so small audio/timing jitter still needs checking on the device; adding a cooperative task does not remove that hardware limitation.

## Build and regression tests

[platformio.ini](platformio.ini) pins the tested AVR platform, toolchain and eight libraries. The unused LiquidCrystal_I2C dependency has been removed. Project sources are compiled with Wall, Wextra and Werror, with an unused-parameter exception for tcMenu/IoAbstraction's inline callback headers. Upstream library compilation warnings are not promoted to errors, and the native portable modules use strict warnings without that exception.

Use **Terminal > Run Build Task** for **Build Gyerek_jatek (Mega)**. The VS Code test task **Test Gyerek_jatek (no hardware)** invokes [tools/run_tests.py](tools/run_tests.py).

The runner uses Python's standard unittest for project/configuration checks and compiles the actual portable C++ modules for native regression tests. No Mega, serial port or upload is involved. It locates g++/clang++/Zig on PATH, or the optional pinned Zig package in the current Python or PlatformIO Python environment. If no native compiler is installed, install the package specified in [tools/requirements-test.txt](tools/requirements-test.txt) into a Python environment, or provide a C++ compiler. The runner also accepts a sanitizer flag when the native compiler supports undefined-behavior instrumentation. Every PlatformIO build also runs the read-only project/configuration checks through [tools/check_build.py](tools/check_build.py); this preflight requires no native compiler.

Tests cover bouncing/held/boot buttons, initial position and repeated wraparound, every target and win-window boundary, all four speeds, score pixels, intro/feedback timing and rollover, invalid target clamping, repeated Debug entry/exit, and LCD clipping with guard bytes. Dedicated audio tests cover both result patterns and gaps, one-shot triggering on all four levels, cancellation, independent audio updates, timer rollover and delayed scheduling. Debug tests cover all input rows, reserved/output protection, bounded scan work, cache lifecycle, formatting, task fallback and I2C error status. Additional input tests exercise fixed-repeat joystick timing, debounced digital/analog button capture, mixed pin names, analog output/joystick protection and exact pull-up restoration. LED-state tests cover all lengths 1-100, exactly one lit pixel, manual stepping and wraparound, cancellation, one-based LCD text and the two-minute timeout/rollover. Standalone melody/player tests remain for future reuse. Project checks enforce the three-option root, matching designer definition, LED length/step/exit wiring, one D2 controller, fixed diagnostic rows, scoped pull-ups and exactly two permanent application tasks.

Native tests cannot verify real FastLED interrupt behavior, I2C communication, supply capacity or buzzer timing. Run those checks on the actual hardware before deployment.

## tcMenu designer and regeneration

[Gyerek_jatek_arduinomega.emf](Gyerek_jatek_arduinomega.emf) remains the menu definition. [src/Gyerek_jatek_arduinomega_menu.cpp](src/Gyerek_jatek_arduinomega_menu.cpp) and [src/Gyerek_jatek_arduinomega_menu.h](src/Gyerek_jatek_arduinomega_menu.h) were originally produced by Designer 4.4.0; the current firmware is verified against pinned tcMenu 4.5.9. The generated header date is provenance, not a claim that the newer designer was run.

The unsafe renderer plugin was replaced with a project-owned **SafeLiquidCrystalRenderer**, not edited in place under a generated filename. When regenerating the menu:

1. Preserve the SafeLiquidCrystalRenderer include, extern declaration and global renderer binding in the generated menu pair.
2. Do not compile a regenerated legacy renderer or another Arduino main/sketch alongside the existing entry point. Keep application implementations in their dedicated modules.
3. Preserve read-only diagnostic flags, callbacks and labels from the updated designer definition. Keep the menuInput::begin binding and configured display rate; do not regenerate the old accelerated setupAnalogJoystickEncoder input alongside it.
4. Run the project checks and native tests, then build the Mega environment. The project checks deliberately fail if unsafe/generated duplicates are reintroduced; PlatformIO also enforces these checks before compilation.

Greensleeves arrangement and score attribution are retained in [src/music/Melody.cpp](src/music/Melody.cpp). The custom LCD renderer retains the original tcMenu Apache license attribution.
