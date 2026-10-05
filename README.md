# Gyerek_jatek

Arduino Mega 2560 LED reaction game with a 16x2 tcMenu LCD interface and a buzzer music mode. Open [Gyerek_jatek.code-workspace](Gyerek_jatek.code-workspace); it contains only this project, not the unrelated Csiraztato project.

Magyar nyelvű felhasználói útmutató: [HASZNALATI_UTMUTATO.md](HASZNALATI_UTMUTATO.md).

## Firmware structure

| Module | Responsibility |
| --- | --- |
| [src/main.cpp](src/main.cpp) | Arduino setup/loop entry point |
| [src/app/Application.cpp](src/app/Application.cpp) | Hardware initialization and two permanent 5 ms tasks: game/input and audio |
| [src/app/ModeController.cpp](src/app/ModeController.cpp) | Game/music selection, one-shot result events and shared-buzzer ownership; no task allocation |
| [src/audio/SoundEffects.cpp](src/audio/SoundEffects.cpp) | Flash-backed, non-blocking win/loss buzzer patterns |
| [src/game/CycloneGame.cpp](src/game/CycloneGame.cpp) | Portable game state machine, movement, hit windows, intro and feedback timers |
| [src/game/DebouncedButton.h](src/game/DebouncedButton.h) | One event per debounced press, with rollover-safe timing |
| [src/hardware/LedDisplay.cpp](src/hardware/LedDisplay.cpp) | Bounded FastLED drawing; redraw from a clean frame to avoid stale pixels |
| [src/hardware/Diagnostics.cpp](src/hardware/Diagnostics.cpp) | Fixed-buffer digital/analog diagnostics and D13 LED output |
| [src/music/Melody.cpp](src/music/Melody.cpp) | Greensleeves notes stored once in AVR flash and read twice |
| [src/music/MusicPlayer.cpp](src/music/MusicPlayer.cpp) | Non-blocking tone playback, stop and restart; independent of game state |
| [src/ui/MenuCallbacks.cpp](src/ui/MenuCallbacks.cpp) | Application-owned tcMenu callbacks |
| [src/ui/SafeLiquidCrystalRenderer.cpp](src/ui/SafeLiquidCrystalRenderer.cpp) | Project-owned LCD renderer with clipped text and cursor bounds |
| [src/ui/LcdText.cpp](src/ui/LcdText.cpp) | Portable, tested LCD text composition |

## Wiring

The existing hardware assignments are unchanged. Application pins and power limits are centralized in [src/config/BoardConfig.h](src/config/BoardConfig.h).

| Connection | Mega pin / setting |
| --- | --- |
| WS2812B LED data | D2; 70 game LEDs followed by 5 score LEDs; GRB |
| Passive buzzer | D5 |
| Game push button | D46 to GND, internal pull-up, active LOW |
| Diagnostic buttons | D46 through D52 inclusive, to GND, internal pull-ups |
| Menu select button | D53 to GND, configured by tcMenu |
| Menu joystick Y | A9; centre 0.5, tolerance 0.1 |
| Diagnostic analog input | A0 |
| Optional randomness input | A1, preferably unconnected; mixed with first-player timing |
| LCD | PCF8574 at I2C address 0x27; Mega SDA D20 and SCL D21 |
| Diagnostic output | On-board LED D13; OFF/ON only |

LCD constructor pin numbers 0-7 refer to **PCF8574 bits**, not Mega pins, after the I/O abstraction is selected. There is no LCD/FastLED D2 conflict in this configuration.

Use an appropriate external 5 V supply for the strip, a common ground, and suitable data-line protection. FastLED's configured 1000 mA software estimate helps cap brightness; it does not verify or protect the physical power supply. Actual supply, wiring, joystick calibration, LCD backpack mapping and buzzer operation require on-device checks.

## Behavior

- Default mode: **Cyclon**. Press D46 once to start the non-blocking intro wipe (500 ms pause, 70 x 40 ms game pixels, 5 x 100 ms score pixels).
- A held button is never reused to stop a level, restart after a loss, or start a game on boot. Release it before pressing again.
- Level speeds are 50, 40, 35 and 20 ms per pixel; the task polls every 5 ms rather than imposing the old 50 ms limit. Actual timing can have small scheduling and LED-transmission jitter.
- Level 1 accepts the five-pixel target window, level 2 accepts three pixels, and levels 3/4 accept only the centre. The checked position is the displayed LED, not the next index.
- Wins/losses flash green/red three times, 500 ms on and 500 ms off, without delaying menu or application dispatch. Winning advances to the next level; finishing level 4 or losing returns to idle.
- Each win plays a short rising chime (523, 659, 784 Hz; about 0.54 seconds); each loss plays a descending sound (392, 294, 196 Hz; about 0.74 seconds). Each result starts its sound **once**, alongside the LED flashes, not once per flash or while a button is held.
- Four score LEDs indicate the current difficulty level; the fifth lights during final-victory feedback. All five are cleared on the next start.
- **Zene** replaces the unfinished Item2 option: it stops the game, clears the strip, and plays Greensleeves once (including the original repeated phrase). Music completion never resets a game. Switching to Cyclon stops the buzzer.
- **Ujrainditas** resets the selected game/music mode and cancels any unfinished result sound. Changing modes also cancels result audio before starting music. Selecting the already selected mode does not create tasks or restart it.
- **Bemenetek** lists live D46-D52 states; **Input naplo** logs digital changes rather than printing every frame. Digital/analog diagnostic fields are read-only and refresh every 250 ms. **D13 LED** controls only the on-board LED.
- Selection and diagnostics are intentionally volatile; no EEPROM addresses are assigned or written.

All application timers use unsigned elapsed-time subtraction for millis rollover. Button sampling, music notes, diagnostic formatting and LED rendering do not allocate Arduino String objects. The frameworks may still allocate their own menu/task storage.

## Cooperative multitasking and the buzzer

The Mega uses **TaskManagerIO cooperative scheduling**, not OS threads or a new RTOS. The game/input task and audio task are registered once at startup; both have separate 5 ms update intervals in [src/config/BoardConfig.h](src/config/BoardConfig.h). The LCD/input framework has its own scheduled work. A task callback does a small amount of work and returns; no task waits for a note or an animation to finish. Arduino's timer-backed tone function generates the actual buzzer waveform asynchronously.

The game task detects entry into win/loss feedback and requests a sound. The audio task advances the existing sound or the selected music. One physical buzzer cannot mix both sources: the mode controller cancels effects before starting music, and switching back stops music. If the task queue is full, each failed task has its own main-loop fallback rather than retrying allocation from menu callbacks.

Customize pitches, note lengths and silent gaps in the WinPattern and LossPattern tables in [src/audio/SoundEffects.cpp](src/audio/SoundEffects.cpp). They live in AVR flash and use no dynamic allocation. Late audio updates skip expired notes; feedback completion also cancels stale audio so it cannot carry into the next level.

This design can be extended with additional short scheduled callbacks. Keep these state machines on the cooperative loop; they are not thread-safe or intended to be called from an ISR. A future true multithreaded port would need queued commands or synchronization around shared state and buzzer ownership. WS2812B transmission can briefly mask timer interrupts on AVR, so small audio/timing jitter still needs checking on the device; adding a cooperative task does not remove that hardware limitation.

## Build and regression tests

[platformio.ini](platformio.ini) pins the tested AVR platform, toolchain and eight libraries. The unused LiquidCrystal_I2C dependency has been removed. Project sources are compiled with Wall, Wextra and Werror, with an unused-parameter exception for tcMenu/IoAbstraction's inline callback headers. Upstream library compilation warnings are not promoted to errors, and the native portable modules use strict warnings without that exception.

Use **Terminal > Run Build Task** for **Build Gyerek_jatek (Mega)**. The VS Code test task **Test Gyerek_jatek (no hardware)** invokes [tools/run_tests.py](tools/run_tests.py).

The runner uses Python's standard unittest for project/configuration checks and compiles the actual portable C++ modules for native regression tests. No Mega, serial port or upload is involved. It locates g++/clang++/Zig on PATH, or the optional pinned Zig package in the current Python or PlatformIO Python environment. If no native compiler is installed, install the package specified in [tools/requirements-test.txt](tools/requirements-test.txt) into a Python environment, or provide a C++ compiler. The runner also accepts a sanitizer flag when the native compiler supports undefined-behavior instrumentation. Every PlatformIO build also runs the five read-only project/configuration checks through [tools/check_build.py](tools/check_build.py); this preflight requires no native compiler.

Tests cover bouncing/held/boot buttons, initial position and repeated wraparound, every target and win-window boundary, all four speeds, score pixels, intro/feedback timing and rollover, invalid target clamping, music completion/cancellation/rollover, repeated selection, and LCD clipping with guard bytes. Dedicated audio tests cover both result patterns and gaps, one-shot triggering on all four levels, cancellation, mode switching, independent music updates, timer rollover and delayed scheduling. The project checks enforce exactly two permanent application tasks and detect task allocation from callbacks, duplicate entry points, accidental String/delay reintroduction, incorrect workspace folders and mismatches between the designer and checked-in menu.

Native tests cannot verify real FastLED interrupt behavior, I2C communication, supply capacity or buzzer timing. Run those checks on the actual hardware before deployment.

## tcMenu designer and regeneration

[Gyerek_jatek_arduinomega.emf](Gyerek_jatek_arduinomega.emf) remains the menu definition. [src/Gyerek_jatek_arduinomega_menu.cpp](src/Gyerek_jatek_arduinomega_menu.cpp) and [src/Gyerek_jatek_arduinomega_menu.h](src/Gyerek_jatek_arduinomega_menu.h) were originally produced by Designer 4.4.0; the current firmware is verified against pinned tcMenu 4.5.9. The generated header date is provenance, not a claim that the newer designer was run.

The unsafe renderer plugin was replaced with a project-owned **SafeLiquidCrystalRenderer**, not edited in place under a generated filename. When regenerating the menu:

1. Preserve the SafeLiquidCrystalRenderer include, extern declaration and global renderer binding in the generated menu pair.
2. Do not compile a regenerated legacy renderer or another Arduino main/sketch alongside the existing entry point. Keep application implementations in their dedicated modules.
3. Preserve read-only diagnostic flags, callbacks and labels from the updated designer definition.
4. Run the project checks and native tests, then build the Mega environment. The project checks deliberately fail if unsafe/generated duplicates are reintroduced; PlatformIO also enforces these checks before compilation.

Greensleeves arrangement and score attribution are retained in [src/music/Melody.cpp](src/music/Melody.cpp). The custom LCD renderer retains the original tcMenu Apache license attribution.