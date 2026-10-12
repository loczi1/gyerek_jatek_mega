# Project guidance for future coding sessions

## Start here

- Read [docs/SESSION_HANDOFF.md](docs/SESSION_HANDOFF.md) for the latest checkpoint, then [README.md](README.md) for architecture and wiring. The current source and Git diff take precedence over dated notes.
- Work in [Gyerek_jatek.code-workspace](Gyerek_jatek.code-workspace), which contains only this project. Inspect Git status before editing; preserve existing staged, unstaged and untracked work.
- Target Arduino Mega 2560, AVR C++11 and the pinned PlatformIO environment `megaatmega2560`. Do not introduce a new framework, RTOS or dependency upgrade without a requirement.

## Firmware boundaries and invariants

- Keep [src/main.cpp](src/main.cpp) as a thin Arduino entry point, including Arduino.h before defining `setup()` and `loop()`.
- Keep portable state machines separate from Arduino hardware adapters. Add host regression tests for changed state, timing, bounds and cancellation behavior.
- Keep the two permanent cooperative application tasks; never schedule another recurring task per menu selection, button press or result. Do not add blocking waits or Arduino `String` allocation to runtime paths. Use unsigned elapsed-time subtraction for timer rollover.
- Preserve the three root options **Jatek**, **Ujrainditas**, **Debug**, unless the user requests a redesign. Cyclon is the only selectable game; standalone melody/player modules are not active firmware features.
- Use [src/config/PinConfig.h](src/config/PinConfig.h), [src/config/BoardConfig.h](src/config/BoardConfig.h), [src/config/NavigationConfig.h](src/config/NavigationConfig.h) and [src/config/LedConfig.h](src/config/LedConfig.h) as configuration authorities. Do not silently change wiring.
- Keep D2 under one FastLED controller with a 100-pixel buffer. Game frames use 75 pixels and clear the remaining tail. The strip test must clear all pixels when it ends.
- Keep joystick navigation single-step, fixed-repeat and neutral-rearmed. tcMenu owns the absolute encoder position; do not restore its old accelerated joystick task or introduce a second cursor.
- Button tests may temporarily enable pull-ups only on eligible INPUT pins. Preserve reserved pins, A9 navigation, existing pull-ups and output owners. Restore only the test's own changes on Back, mode change and timeout.
- Keep menu metadata and [Gyerek_jatek_arduinomega.emf](Gyerek_jatek_arduinomega.emf) synchronized, with stable runtime-list sizes and non-overlapping ID ranges. Preserve the project-owned safe renderer and input bindings after designer regeneration; do not edit installed libraries or generated build caches.

## Validation and handoff

- For firmware/menu changes, use the VS Code tasks **Build Gyerek_jatek (Mega)** and **Test Gyerek_jatek (no hardware)**. Equivalent commands are `platformio run -e megaatmega2560` and `python tools/run_tests.py`; the native runner also accepts `--sanitize` when supported.
- Preserve strict project warnings and the documented unused-parameter exception for upstream inline headers. Read full build errors before changing flags or dependencies.
- Do not upload firmware, stage/commit changes or modify unrelated workbooks/backups unless requested. Report what was actually built, tested and uploaded.
- Hardware acceptance is separate from native tests. Use [docs/HARDWARE_TEST_CHECKLIST.md](docs/HARDWARE_TEST_CHECKLIST.md); distinguish user-reported tests from tests performed in the current session.
- When behavior changes, update the technical README, the Hungarian [HASZNALATI_UTMUTATO.md](HASZNALATI_UTMUTATO.md), relevant tests and the dated handoff. For documentation-only work, validate links/content without unnecessarily rebuilding firmware.
