# Gyerek_jatek

Arduino Mega 2560 LED reaction game with a 16x2 tcMenu LCD interface, win/loss buzzer sounds and live input diagnostics. Open [Gyerek_jatek.code-workspace](Gyerek_jatek.code-workspace); it contains only this project, not the unrelated Csiraztato project.

Magyar nyelvű felhasználói útmutató: [HASZNALATI_UTMUTATO.md](HASZNALATI_UTMUTATO.md).

## Firmware structure

| Module | Responsibility |
| --- | --- |
| [src/main.cpp](src/main.cpp) | Arduino setup/loop entry point |
| [src/app/Application.cpp](src/app/Application.cpp) | Hardware initialization and two permanent 5 ms tasks: game/input and audio |
| [src/app/ModeController.cpp](src/app/ModeController.cpp) | Cyclon/Debug selection and one-shot result sounds; no task allocation |
| [src/audio/SoundEffects.cpp](src/audio/SoundEffects.cpp) | Flash-backed, non-blocking win/loss buzzer patterns |
| [src/game/CycloneGame.cpp](src/game/CycloneGame.cpp) | Portable game state machine, movement, hit windows, intro and feedback timers |
| [src/game/DebouncedButton.h](src/game/DebouncedButton.h) | One event per debounced press, with rollover-safe timing |
| [src/hardware/LedDisplay.cpp](src/hardware/LedDisplay.cpp) | Bounded FastLED drawing; redraw from a clean frame to avoid stale pixels |
| [src/hardware/Diagnostics.cpp](src/hardware/Diagnostics.cpp) | Mega register/ADC adapter and scoped button pull-ups; preserves outputs and PWM |
| [src/debug/InputMonitor.cpp](src/debug/InputMonitor.cpp) | Cached, bounded digital/analog scanning and input labels |
| [src/debug/ButtonTester.cpp](src/debug/ButtonTester.cpp) | Debounced pressed-pin detection, last-press capture and reversible input pull-ups |
| [src/debug/SystemChecks.cpp](src/debug/SystemChecks.cpp) | Game/audio task health and LCD I2C error reporting |
| [src/ui/MenuInput.cpp](src/ui/MenuInput.cpp) | Static tcMenu encoder; deterministic A9 input polled by the existing task |
| [src/ui/JoystickRepeat.cpp](src/ui/JoystickRepeat.cpp) | Portable joystick hysteresis, debounce, neutral rearm and fixed repeats |
| [src/ui/MenuCallbacks.cpp](src/ui/MenuCallbacks.cpp) | Application-owned tcMenu callbacks |
| [src/ui/SafeLiquidCrystalRenderer.cpp](src/ui/SafeLiquidCrystalRenderer.cpp) | Project-owned LCD renderer with clipped text and cursor bounds |
| [src/ui/LcdText.cpp](src/ui/LcdText.cpp) | Portable, tested LCD text composition |

## Wiring

The existing hardware assignments are unchanged. Pin ownership is centralized in [src/config/PinConfig.h](src/config/PinConfig.h); hardware timing and power limits are in [src/config/BoardConfig.h](src/config/BoardConfig.h).

| Connection | Mega pin / setting |
| --- | --- |
| WS2812B LED data | D2; 70 game LEDs followed by 5 score LEDs; GRB |
| Passive buzzer | D5 |
| Game push button | D46 to GND, internal pull-up, active LOW |
| Button Debug inputs | All safe D0-D53 input candidates, button to GND; temporary internal pull-ups in Gombok. D46-D52 keep their existing pull-ups |
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
- Default mode: **Cyclon**, ready at the rainbow screen. Press D46 once to start the non-blocking intro wipe (500 ms pause, 70 x 40 ms game pixels, 5 x 100 ms score pixels).
- A held button is never reused to stop a level, restart after a loss, or start a game on boot. Release it before pressing again.
- Level speeds are 50, 40, 35 and 20 ms per pixel; the task polls every 5 ms rather than imposing the old 50 ms limit. Actual timing can have small scheduling and LED-transmission jitter.
- Level 1 accepts the five-pixel target window, level 2 accepts three pixels, and levels 3/4 accept only the centre. The checked position is the displayed LED, not the next index.
- Wins/losses flash green/red three times, 500 ms on and 500 ms off, without delaying menu or application dispatch. Winning advances to the next level; finishing level 4 or losing returns to idle.
- Each win plays a short rising chime (523, 659, 784 Hz; about 0.54 seconds); each loss plays a descending sound (392, 294, 196 Hz; about 0.74 seconds). Each result starts its sound **once**, alongside the LED flashes, not once per flash or while a button is held.
- Four score LEDs indicate the current difficulty level; the fifth lights during final-victory feedback. All five are cleared on the next start.
- **Ujrainditas** resets Cyclon to its ready screen and cancels unfinished result sounds. Press D46 again to start a new round.
- **Debug** stops the current game and sound while any of its nested views is open, so testing D46 cannot start/stop a round. Returning to the main menu restores Cyclon at its ready screen, not mid-round. A held D46 must be released before the next start.
- Debug and nested lists automatically return to the main menu after **2 minutes without joystick/menu-button interaction**. Change MenuIdleTimeoutSeconds in [src/config/BoardConfig.h](src/config/BoardConfig.h); [src/app/Application.cpp](src/app/Application.cpp) applies it after menu setup so tcMenu's default 30-second interval is overridden. Live input refreshes do not extend this timeout. This is separate from the 25 ms I2C bus timeout.
- JatekValaszto, Input naplo, the old test placeholders and Greensleeves selection are removed from the menu. The standalone [src/music/MusicPlayer.cpp](src/music/MusicPlayer.cpp) and [src/music/Melody.cpp](src/music/Melody.cpp) modules remain for future reuse/tests but are not referenced by the active firmware application.
- Diagnostics are intentionally volatile; no EEPROM addresses are assigned or written.

All application timers use unsigned elapsed-time subtraction for millis rollover. Button sampling, music notes, diagnostic formatting and LED rendering do not allocate Arduino String objects. The frameworks may still allocate their own menu/task storage.

## Predictable menu navigation

The old analog-joystick acceleration is disabled. A short movement produces **one step** after 30 ms of stable input. Holding a direction starts single-step repeats after **500 ms**, then repeats **once every 300 ms**, independent of how far the joystick is pushed. Returning to centre stops repeats; a delayed task never replays missed steps.

The LCD refreshes at **4 Hz**, faster than held repeats, rather than hiding several changes between frames. A9 is sampled every 10 ms with a discarded first ADC conversion, plus hysteresis around the centre. Entering/leaving a menu, auto-return, and pressing the menu/select button rearm navigation only after the joystick has returned to neutral. tcMenu remains the owner of the absolute cursor and its bounds; there is no separate stale cursor or new recurring joystick task.

Adjust the centre, thresholds, debounce, hold delay, repeat interval and display rate in [src/config/NavigationConfig.h](src/config/NavigationConfig.h). Hardware joystick centre/voltage still needs on-device verification if it does not settle near 512.

## Debug menu

- **Gombok:** identifies buttons wired between a safe digital input and GND. On entry it enables missing internal pull-ups for all **46 candidates**, excluding UART D0/D1, LED D2, buzzer D5, status output D13, I2C D20/D21 and menu/select D53. Any additional pin already configured as an output is skipped. After 30 ms debounce, a pulled-down pin is shown as pressed.
	- **Utolso:** most recently detected press, retained after release so short presses remain visible until the next press or test entry.
	- **Lenyomva:** currently pressed pins. **Darab:** current pressed count. **Tesztelt:** number of safe inputs participating.
	- Summaries show up to three IDs plus a **+N** overflow count; subsequent fixed rows show every candidate individually as HIGH/LOW/OUT. The list never changes size while buttons are pressed, so results do not move the cursor.
	- Leaving Gombok (Back, another view, or the two-minute auto-return) restores only pull-ups added by this test. Existing D46-D52 pull-ups remain, and a pin changed to an output by another owner is never overwritten. Analog A0-A15 pins are not given button pull-ups.
- **Digitalis:** scroll through D0-D53. General input pins show **LOW/HIGH**; D46 is labelled Jatek, D53 Menu. Configured outputs show **OUT**. UART pins D0/D1, LED data D2, buzzer D5, status output D13 and I2C D20/D21 are reserved and display their role instead of being converted to inputs or sampled.
- **Analog:** scroll through all **A0-A15**, showing raw 10-bit readings **0-1023**. A9 is labelled Joy. An analog pin configured as a digital output shows OUT and is not read as an ADC input. **---** means not yet sampled; ERR denotes an invalid internal sample, not an automatic diagnosis of a disconnected sensor.
- **Allapot:** current application state; Debug indicates that the game is stopped for input testing.
- **Hibak:** separate rows for **Jatek task**, **Hang task** and **LCD 0x27**. Tasks show OK when scheduled or FALLBACK when their main-loop fallback is in use. The LCD address-only probe reports OK, NINCS (no ACK), I2C HIBA or TIMEOUT. It runs on Debug entry with a 25 ms Wire timeout; subsequent Wire timeouts remain visible until the next entry/probe.

Digitalis/Analog scanning is read-only: it never switches pin modes, drives a test output, changes pull-ups or disables PWM. Gombok deliberately changes only the pull-up latch of existing safe inputs while that test is open. Outside Gombok, only the existing D46-D52 buttons and menu D53 have configured internal pull-ups; other unused inputs may float. A LOW reading can also mean a permanent ground connection or an external pull-down, not necessarily a mechanically pressed button. For a meaningful check, connect a button or known safe signal with a common ground. Never apply a voltage outside the Mega's 0-5 V input range or connect a test switch to a reserved/output pin.

The existing 5 ms task samples at most four digital pins and one analog channel per tick in Debug, plus all active button-test inputs when Gombok is open. Cached digital/analog channels refresh in about 80 ms; Gombok checks every task tick and retains short presses. The LCD redraws at 4 Hz. The first ADC conversion after switching channels is discarded to reduce cross-talk. The full analog sweep runs only in Debug (A9 navigation is sampled in all modes). These are input observations plus task/LCD communication checks, not a full automatic hardware self-test (power, LED wiring and buzzer operation still require physical testing).

## Cooperative multitasking and the buzzer

The Mega uses **TaskManagerIO cooperative scheduling**, not OS threads or a new RTOS. The game/input task and audio task are registered once at startup; both have separate 5 ms update intervals in [src/config/BoardConfig.h](src/config/BoardConfig.h). The LCD/input framework has its own scheduled work. A task callback does a small amount of work and returns; no task waits for a note or an animation to finish. Arduino's timer-backed tone function generates the actual buzzer waveform asynchronously.

The game task detects entry into win/loss feedback and requests a sound. The audio task advances that effect independently of LED animation. Debug and restart cancel unfinished effects; no music source is selectable. If the task queue is full, each failed task has its own main-loop fallback rather than retrying allocation from menu callbacks.

Customize pitches, note lengths and silent gaps in the WinPattern and LossPattern tables in [src/audio/SoundEffects.cpp](src/audio/SoundEffects.cpp). They live in AVR flash and use no dynamic allocation. Late audio updates skip expired notes; feedback completion also cancels stale audio so it cannot carry into the next level.

This design can be extended with additional short scheduled callbacks. Keep these state machines on the cooperative loop; they are not thread-safe or intended to be called from an ISR. A future true multithreaded port would need queued commands or synchronization around shared state and buzzer ownership. WS2812B transmission can briefly mask timer interrupts on AVR, so small audio/timing jitter still needs checking on the device; adding a cooperative task does not remove that hardware limitation.

## Build and regression tests

[platformio.ini](platformio.ini) pins the tested AVR platform, toolchain and eight libraries. The unused LiquidCrystal_I2C dependency has been removed. Project sources are compiled with Wall, Wextra and Werror, with an unused-parameter exception for tcMenu/IoAbstraction's inline callback headers. Upstream library compilation warnings are not promoted to errors, and the native portable modules use strict warnings without that exception.

Use **Terminal > Run Build Task** for **Build Gyerek_jatek (Mega)**. The VS Code test task **Test Gyerek_jatek (no hardware)** invokes [tools/run_tests.py](tools/run_tests.py).

The runner uses Python's standard unittest for project/configuration checks and compiles the actual portable C++ modules for native regression tests. No Mega, serial port or upload is involved. It locates g++/clang++/Zig on PATH, or the optional pinned Zig package in the current Python or PlatformIO Python environment. If no native compiler is installed, install the package specified in [tools/requirements-test.txt](tools/requirements-test.txt) into a Python environment, or provide a C++ compiler. The runner also accepts a sanitizer flag when the native compiler supports undefined-behavior instrumentation. Every PlatformIO build also runs the read-only project/configuration checks through [tools/check_build.py](tools/check_build.py); this preflight requires no native compiler.

Tests cover bouncing/held/boot buttons, initial position and repeated wraparound, every target and win-window boundary, all four speeds, score pixels, intro/feedback timing and rollover, invalid target clamping, repeated Debug entry/exit, and LCD clipping with guard bytes. Dedicated audio tests cover both result patterns and gaps, one-shot triggering on all four levels, cancellation, independent audio updates, timer rollover and delayed scheduling. Debug tests cover all input rows, reserved/output protection, bounded scan work, cache lifecycle, formatting, task fallback and I2C error status. Additional input tests exercise fixed-repeat joystick timing, force/noise/hysteresis, neutral rearming, stalled polling, debounced button capture, multi-press reports and exact pull-up restoration. Standalone melody/player tests remain for future reuse. Project checks enforce the three-option root and matching designer definition, fixed-size diagnostic lists, deterministic input wiring, scoped pull-ups, exactly two permanent application tasks, and no accidental callback task allocation, duplicate entry points or String/delay reintroduction.

Native tests cannot verify real FastLED interrupt behavior, I2C communication, supply capacity or buzzer timing. Run those checks on the actual hardware before deployment.

## tcMenu designer and regeneration

[Gyerek_jatek_arduinomega.emf](Gyerek_jatek_arduinomega.emf) remains the menu definition. [src/Gyerek_jatek_arduinomega_menu.cpp](src/Gyerek_jatek_arduinomega_menu.cpp) and [src/Gyerek_jatek_arduinomega_menu.h](src/Gyerek_jatek_arduinomega_menu.h) were originally produced by Designer 4.4.0; the current firmware is verified against pinned tcMenu 4.5.9. The generated header date is provenance, not a claim that the newer designer was run.

The unsafe renderer plugin was replaced with a project-owned **SafeLiquidCrystalRenderer**, not edited in place under a generated filename. When regenerating the menu:

1. Preserve the SafeLiquidCrystalRenderer include, extern declaration and global renderer binding in the generated menu pair.
2. Do not compile a regenerated legacy renderer or another Arduino main/sketch alongside the existing entry point. Keep application implementations in their dedicated modules.
3. Preserve read-only diagnostic flags, callbacks and labels from the updated designer definition. Keep the menuInput::begin binding and configured display rate; do not regenerate the old accelerated setupAnalogJoystickEncoder input alongside it.
4. Run the project checks and native tests, then build the Mega environment. The project checks deliberately fail if unsafe/generated duplicates are reintroduced; PlatformIO also enforces these checks before compilation.

Greensleeves arrangement and score attribution are retained in [src/music/Melody.cpp](src/music/Melody.cpp). The custom LCD renderer retains the original tcMenu Apache license attribution.