# Gyerek_jatek

Arduino Mega 2560 LED games (Cyclon, Random, Random 2, Sorrend, Sima villogó / Flasher, Kapcsolozar and Parbaj) with a 16x2 tcMenu LCD interface, buzzer feedback and live input diagnostics. Open [Gyerek_jatek.code-workspace](Gyerek_jatek.code-workspace); it contains only this project, not the unrelated Csiraztato project.

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
| [src/app/ModeController.cpp](src/app/ModeController.cpp) | Cyclon/Debug engine control and one-shot result sounds; application owns game selection and menu scope |
| [src/audio/SoundEffects.cpp](src/audio/SoundEffects.cpp) | Flash-backed, non-blocking win/loss, warning and Random 2 melody patterns |
| [src/game/CycloneGame.cpp](src/game/CycloneGame.cpp) | Portable game state machine, movement, hit windows, intro and feedback timers |
| [src/game/PanelGames.cpp](src/game/PanelGames.cpp) | Portable Random/Random 2/Sorrend/Flasher state machines, debounce, timing and feedback; basic Sorrend retains internal `Simon` naming |
| [src/game/ChallengeGames.cpp](src/game/ChallengeGames.cpp) | Portable Kapcsolozar3/4 and Parbaj state machines; whole-vector debounce, preview, arbitration and feedback |
| [src/game/PairedLampOutputs.cpp](src/game/PairedLampOutputs.cpp) | Portable scoped ownership and polarity-aware paired GPIO lamp output |
| [src/hardware/GameLamps.cpp](src/hardware/GameLamps.cpp) | Mega GPIO adapter for Random 2 lamps; preserves other owners and restores prior input modes |
| [src/config/PanelConfig.h](src/config/PanelConfig.h) | Eleven panel channels resolved through workbook lamp pairs and strip positions |
| [src/game/DebouncedButton.h](src/game/DebouncedButton.h) | One event per debounced press, with rollover-safe timing |
| [src/hardware/LedDisplay.cpp](src/hardware/LedDisplay.cpp) | One 100-pixel workbook-selected strip buffer shared by Cyclon, panel games and strip test |
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

- [Gyerek_jatek_3.0.xlsx](Gyerek_jatek_3.0.xlsx) is the workbook source for generated configuration artifacts.
- [tools/generate_config.py](tools/generate_config.py) parses the workbook with Python's standard library only, so the PlatformIO build does not need JSON parsing or extra Python packages.
- The build regenerates [src/generated/pins.h](src/generated/pins.h) and [src/generated/game_config.h](src/generated/game_config.h) before the normal project checks run.
- The generated headers are constexpr-only data tables intended for flash-friendly firmware consumption. They cover the exported pin catalog, per-game bindings, lamp pairs and 1-based strip positions (`PinDefinition::stripPixel`, 0 unspecified). Numeric `PinMap_export` column B (`Info`) values on WS2812B rows must be unique and in 1–100; descriptive/range text is skipped. Multi-contact GPIO cells retain each contact; the annotation in `25,24 (3 állású)` is not a third pin. The panel feature leaves the workbook unchanged.
- Startup pull-ups follow safe workbook `DigitalIn` contacts, including the remapped game button and both contacts of a three-position switch. Unrelated GPIO and board-owned peripheral pins are not initialized as buttons.
- The long-term sheet layout is documented in [docs/EXCEL_CONFIG_ARCHITECTURE.md](docs/EXCEL_CONFIG_ARCHITECTURE.md). Corrected `GameLevel` rules are implemented manually in source; **the generator still ignores that sheet**. Editing it alone does not change gameplay. The proposed normalized `GameLevels` schema remains a future migration.

## Wiring

Assignments follow the workbook export through [src/generated/pins.h](src/generated/pins.h) and [src/config/PinConfig.h](src/config/PinConfig.h). The table below reflects the **2026-10-14** workbook; regenerate and verify the physical wiring after further edits. Hardware timing and power limits are in [src/config/BoardConfig.h](src/config/BoardConfig.h).

| Connection | Mega pin / setting |
| --- | --- |
| WS2812B LED data | D4; GRB. Cyclon uses 70+5 LEDs; panel lamps require at least 86 physical LEDs; Debug tests a selected length of 1-100 LEDs |
| Passive buzzer | D5 |
| Game push button | D39 to GND, internal pull-up, active LOW; derived from workbook `CYCLON` `Start` / `Action` -> `BTN15` |
| Potentiometer | `POT1` on A15; the workbook value 15 is an ADC channel, not D15 |
| Three-position switch | `SW06` contacts on D25 and D24; both exported as digital inputs |
| Button Debug inputs | Safe D0-D53 and A0-A15 GPIO, excluding A9 joystick; button to GND; temporary pull-ups in Gombok. Existing pull-ups are preserved |
| Menu select button | D53 to GND, configured by tcMenu |
| Menu joystick Y | A9; ADC centre 512, engage distance 140, release distance 80 |
| Analog Debug view | A0-A15, including menu joystick A9 |
| Optional randomness input | A1, preferably unconnected; mixed with first-player timing |
| LCD | PCF8574 at I2C address 0x27; Mega SDA D20 and SCL D21 |
| Status output | On-board LED D13; reserved, not driven by Debug |

LCD constructor pin numbers 0-7 refer to **PCF8574 bits**, not Mega pins, after the I/O abstraction is selected. There is no LCD/FastLED pin conflict in this configuration.

Use an appropriate external 5 V supply for the strip, a common ground, and suitable data-line protection. FastLED's configured 1000 mA software estimate helps cap brightness; it does not verify or protect the physical power supply. Actual supply, wiring, joystick calibration, LCD backpack mapping and buzzer operation require on-device checks.

### Illuminated Ikarus switches

Each Ikarus unit has a switch contact and a built-in light. They are **two separate electrical controls**, paired by the matching `_ikarus_N` workbook suffix:

| Unit | Switch control / input | Built-in LED control / output |
| --- | --- | --- |
| 1 | `BTN16_ikarus_1` / D52 | `DIO01_ikarus_1` / D2 |
| 2 | `BTN17_ikarus_2` / D50 | `DIO02_ikarus_2` / D16 |
| 3 | `BTN18_ikarus_3` / D46 | `DIO03_ikarus_3` / D17 |
| 4 | `BTN19_ikarus_4` / D48 | `DIO04_ikarus_4` / D3 |
| 5 | `BTN20_ikarus_5` / D23 | `DIO05_ikarus_5` / D18 |
| 6 | `BTN21_ikarus_6` / D51 | `DIO06_ikarus_6` / D19 |

The generator emits `generated::pins::IkarusSwitches` and **22** `generated::pins::LampPairs` associations. Hyphenated button/switch and lamp IDs pair by their suffix, not prefix number: `BTN01-left-blue_1` ↔ `LED01-left-blue_1`, `SW01-metal_1` ↔ `LED05-metal_1`. The workbook's `down-5_N` ↔ `strip-down5_N` spelling is normalized explicitly. Legacy underscore Ikarus IDs remain supported. **Random 2 explicitly lights the pressed Ikarus switch's paired lamp**; the original Random still chooses a random panel lamp and Debug never automatically follows switch presses. Pair metadata does not alter Cyclon's controls or 75-pixel layout; its new profiles are documented below.

The workbook's `ActiveLevel` is exported in each pin definition: `HIGH` / `LOW` describe a transistor lamp's active GPIO level; `WS2812B` identifies strip data, never a directly switchable lamp output. Blank means unspecified and is excluded from manual GPIO output testing. All six current transistor lamps are **HIGH = ON, LOW = OFF**. Pair identity does not change their electrical independence or the 75-pixel Cyclon layout.

The generator rejects missing/duplicate lamp pairs, incorrect input/output types, invalid active levels, and transistor outputs overlapping UART, board-owned pins, input contacts or another lamp output. D4 is reserved for the strip; D2 is available for Ikarus LED #1. ActiveLevel describes polarity, not lamp voltage/current ratings or driver suitability: verify these and provide appropriate current limiting/driver hardware before connecting a lamp.

## GameLevel implementation — 2026-10-20

The earlier documentation-only review is **historical and superseded by implementation**: Cyclon menu-level profiles and white-target memory Sorrend2–5 now follow the corrected rules. Kapcsolozar3/4 and Parbaj are also implemented. The workbook `GameLevel` sheet remains a human-readable specification, **not automatically imported configuration**; see [implemented rules and explicit source choices](docs/EXCEL_CONFIG_ARCHITECTURE.md#implemented-gamelevel-rules-2026-10-20).

Implementation and validation evidence is recorded below separately from historical verification. The workbook and user-owned handoff remain unchanged; the handoff may describe an older revision. No firmware was uploaded.

## Behavior (current firmware)

- Root options are **Szint1**, **Szint2**, **Szint3**, **Szint4**, **Szint5**, **Ujrainditas**, **Debug**. Every level has Back; root levels are not Cyclon's four internal difficulties. Menu order and action IDs:

	| Level | Games in order (menu IDs) |
	| --- | --- |
	| Szint1 | Random (35), Random 2 (38), unchanged |
	| Szint2 | Cyclon (10), Sorrend (36), Sima villogo (37), unchanged placement; ASCII LCD label, mode `Flasher` |
	| Szint3 | Cyclon (42), Sorrend (39), Kapcsolozar (45), Parbaj (47) |
	| Szint4 | Cyclon (43), Sorrend (40), Kapcsolozar (46) |
	| Szint5 | Cyclon (44), Sorrend (41) |
- Boot selects **no game** and leaves the strip dark. Manually leaving a game's owning level submenu (Back/root or another menu), entering Debug or choosing Ujrainditas cancels the game, tone and all **100** buffered LEDs and restores the normal **120-second** menu/Debug timeout. Returning from Debug does not resume or select a game.
- Selecting **any game** disables tcMenu's automatic return via `renderer.turnOffResetLogic()` during Ready, gameplay and results, including after completion/failure. The game stays selected with **no menu timeout** until manually cancelled. Flasher's **10-second input timeout**, Sorrend3–5's per-answer deadlines and Kapcsolozar4's deadline are gameplay logic, not menu cancellation; the LED Debug screen retains its own **120-second** timer.
- **Random:** every safe configured digital input/switch contact, including Cyclon D39 and Ikarus contacts but excluding menu/peripheral pins, accepts fresh debounced presses. Each press chooses one of the **11 panel lamps**, a random hue and a **220–1760 Hz tone for 250 ms**. It does not light the pressed switch's matching Ikarus lamp. Re-selecting Random resets it.
- **Random 2:** accepts the same safe contacts, but lights the **pressed input's own paired lamp** for **1,200 ms**, including all six Ikarus GPIO lamps and SW01–05 strip pixels 76–80. Panel inputs retain the corrected mapping below. Simultaneous fresh presses light their respective lamps together. Unpaired BTN05/BTN06/BTN15 (D39) and SW06 contacts still start the sound/disco, without inventing a lamp association. Each fresh press starts/restarts a tiny original **eight-beat, 150 ms/beat melody** (seven notes plus a silent beat; **32-byte flash table**, not the full Greensleeves player) and **three random colored pixels on positions 1–70**, refreshed every 150 ms. A new press replaces the previous effect; held/bouncing contacts do not repeat. Completion goes dark but leaves the game selected indefinitely. Selection/reselection requires fresh presses. Back, another game, Debug or restart immediately stops audio, clears all 100 strip pixels and releases owned GPIO lamps at their inactive polarity, restoring their previous input/pull-up modes. Existing outputs are never borrowed; if another owner changes a pin's mode/level, Random 2 relinquishes it without overwriting that owner. There is no extra task or LED buffer; the workbook is unchanged.
- **Sorrend (every Szint2–5):** white-target **memory game**, growing from **1 to 32 steps**; former user-facing name Simon mondja, internal `Simon` / `playSimon` retained. Selection waits for a fresh any-panel-button start, consumed rather than answered; reselection resets. Preserve **500 ms initial pause, 500 ms Showing, 200 ms gaps**, and release gating for playback-held inputs. Repeat the hidden sequence on the same 11 mapped buttons. Each correct answer gives **540 ms green/Win**; the next Input entry starts a fresh per-answer timer. Finishing a sequence appends one step, except completion at32 returns Ready. Wrong button/chord/deadline gives **740 ms red/Loss**, then Ready for a fresh start; deadline wins at the boundary.

	| Menu level | Additional saturated-colored distractors | Per-answer Input deadline | Warning at remaining time |
	| --- | --- | --- | --- |
	| 2 | None | Unlimited | None |
	| 3 | 4–6 | 3000 ms | 2 / 1 seconds |
	| 4 | 4–10 | 2000 ms | 1 / 0.5 seconds |
	| 5 | 8–10 | 1200 ms | 1 / 0.5 seconds |

	Distractors occupy distinct channels excluding that step's white target. **Implementation choice (workbook unspecified):** refresh positions/colors per shown step, keep them fixed within that Showing, show distractors only in Showing. **Neither target nor distractors is visible in Input**; gaps are dark. Warning is **80 ms/880 Hz**, once per threshold while awaiting the answer, not a deadline extension. This replaces the old nonmemory Sorrend4/5 behavior.
- **Sima villogó (Szint2, LCD: Sima villogo, mode `Flasher`):** unchanged nonmemory game on the same 11 mapped inputs. A fresh any-panel start is consumed; release after playback is required. After **500 ms initial pause**, one random target lights for **500 ms**, goes dark for **200 ms**, then reappears and **stays visible awaiting its button**. Correct gives **540 ms green/Win**, then a new single target; unlimited rounds, no growing sequence or32-step completion. Wrong/chord/**10 seconds input inactivity** gives **740 ms red/Loss**, then Ready for a fresh start. Reselection resets.
- Workbook `Info` and `LampPairs` are exported unchanged. The panel adapter corrects the lower row's physically reversed strip direction: **BTN07–11 → 75, 74, 73, 72, 71**. Input order and both upper groups remain unchanged:

	| Buttons | 1-based strip pixels |
	| --- | --- |
	| BTN01, BTN02, BTN03 | 84, 85, 86 |
	 | BTN07, BTN08, BTN09, BTN10, BTN11 | 75, 74, 73, 72, 71 |
	| BTN12, BTN13, BTN14 | 83, 82, 81 |

	All lamps need a physical strip of **at least 86 pixels**, even though Cyclon uses only 75.

### Cyclon (Szint2–5)

- Selecting Cyclon waits for a fresh **D39** press at its ready screen. The non-blocking intro wipe is unchanged (500 ms pause, 70 x 40 ms game pixels, 5 x 100 ms score pixels).
- A held button is never reused to stop a level, restart after a loss, or start a game on boot. Release it before pressing again.
- Each selected menu-level profile has four successive internal stages. Values below are ordered stages1/2/3/4; polling remains5 ms and hardware scheduling/LED-transmission jitter is possible.

	| Menu level | Intervals (ms per LED) | Accepted widths (LEDs) |
	| --- | --- | --- |
	| 2 | 50 / 40 / 40 / 40 | 9 / 5 / 5 / 3 |
	| 3 | 40 / 35 / 30 / 30 | 9 / 5 / 5 / 3 |
	| 4 | 50 / 40 / 35 / 20 | 5 / 3 / 2 / 2 |
	| 5 | 40 / 35 / 30 / 20 | 5 / 3 / 2 / 1 |

	Odd widths are centred; **width2 is centre and centre+1**, not ±1. The displayed LED is checked, not the next index. Reselection resets the selected profile to Ready. See [exact offsets](docs/EXCEL_CONFIG_ARCHITECTURE.md#implemented-cyclon-profiles).
- Wins/losses flash green/red three times, 500 ms on and 500 ms off, without delaying menu or application dispatch. Winning advances to the next level; finishing level 4 or losing returns to idle.
- Each win plays a short rising chime (523, 659, 784 Hz; about 0.54 seconds); each loss plays a descending sound (392, 294, 196 Hz; about 0.74 seconds). Each result starts its sound **once**, alongside the LED flashes, not once per flash or while a button is held.
- Four score LEDs indicate the current difficulty level; the fifth lights during final-victory feedback. All five are cleared on the next start.
- **Ujrainditas** clears the selected game, sound and all strip pixels; select a game again to play.
- **Debug** stops the current game and sound while any of its nested views is open, so testing D39 cannot start/stop a round. Returning to root leaves no game selected and the strip dark; there is no automatic resume.
- **When no game is selected**, level submenus, Debug and nested lists return to root after **120 seconds without joystick/menu-button interaction**. `MenuIdleTimeoutSeconds` in [src/config/BoardConfig.h](src/config/BoardConfig.h) is applied/restored by [src/app/Application.cpp](src/app/Application.cpp), overriding tcMenu's30-second default. Tested buttons/live refresh do not extend it. Selected games never auto-return; gameplay deadlines, LED test timeout and25 ms I2C timeout are independent.
- JatekValaszto, Input naplo, old placeholders and Greensleeves selection remain removed. Standalone [src/music/MusicPlayer.cpp](src/music/MusicPlayer.cpp) and [src/music/Melody.cpp](src/music/Melody.cpp) remain for reuse/tests, not active firmware features.
- Diagnostics are intentionally volatile; no EEPROM addresses are assigned or written.

### Kapcsolozar (Szint3/4; LCD: Kapcsolozar)

- Portable engine: [src/game/ChallengeGames.cpp](src/game/ChallengeGames.cpp). **Selection starts the first round immediately; reselection resets/autostarts**, unlike Sorrend/Cyclon. Five literal GPIO HIGH bits are read in SW01–05 order: **D40, D42, D38, D36, D37**; paired white strip lamps are **76–80**. HIGH is not converted to active-LOW button logic. **Physical HIGH=up is an assumption to verify on hardware**, not a proven wiring fact.
- **Szint3:** lit lamp requires HIGH, dark lamp LOW, unlimited response time. **Szint4:** inverse (lit→LOW, dark→HIGH), **2000 ms from round start**, deadline wins even if the last switch stabilizes at that instant. Warning at1/0.5 seconds remaining, using80 ms/880 Hz.
- A random displayed five-bit mask excludes the combination already solved by the sampled switch vector (inverse mapping on level4), so selection cannot instantly win. **All five actual HIGH/LOW levels together must match and remain stable for30 ms**; a fresh mismatch invalidates the match immediately. This is not five independent press events.
- Win: **540 ms green/Win**; timeout: **740 ms red/Loss**, then Ready. A **fresh D39 press** starts the next independent round; held inputs/results do not restart it. Cancelling stops audio and clears all100 pixels.

### Parbaj (Szint3; LCD: Parbaj)

- Selection/reselection immediately starts a **2000 ms preview** on six upper buttons: left BTN01–03 = channels0–2, right BTN12–14 = channels8–10. Each side has three distinct colors from hue palette **0,32,64,96,160**, with **exactly one common color**. Warning at1/0.5 seconds remaining.
- After preview the common color appears on **bottom-middle BTN09 lamp, channel5, pixel73**; it is a cue, not an answer button. The six upper colors remain lit. **No response deadline.** Preview presses are ignored; all upper inputs must be released stably before fresh answers are accepted.
- First accepted upper-side answer resolves the round: correct wins/other side red; wrong loses/other side green. A same-side chord is wrong, even if it contains the common-color button. Answers accepted in the same update: both correct→both green/draw; both wrong→both red/draw; mixed→correct side green/wrong side red.
- Three **90 ms on/90 ms off** result blinks (**540 ms total**). Win audio once if any accepted answer is correct; Loss once otherwise (stopped at result completion, including the normally longer Loss pattern). Ready then needs a **fresh upper-button press** to start another preview; held/partially debounced result presses do not restart. D39 and bottom buttons are not Parbaj answers/restart controls.

All games reuse the existing two cooperative application tasks and the single100-pixel buffer. Manual Back/owning-level exit, another game, Debug or restart stops old warnings/results/audio and clears all100 pixels; no automatic resume. All application timers use unsigned elapsed-time subtraction for millis rollover. Runtime paths do not allocate Arduino `String` objects; frameworks may allocate their own menu/task storage.

## Predictable menu navigation

The old analog-joystick acceleration is disabled. A short movement produces **one step** after 30 ms of stable input. Holding a direction starts single-step repeats after **500 ms**, then repeats **once every 300 ms**, independent of how far the joystick is pushed. Returning to centre stops repeats; a delayed task never replays missed steps.

The LCD refreshes at **4 Hz**, faster than held repeats, rather than hiding several changes between frames. A9 is sampled every 10 ms with a discarded first ADC conversion, plus hysteresis around the centre. Entering/leaving a menu, auto-return, and pressing the menu/select button rearm navigation only after the joystick has returned to neutral. tcMenu remains the owner of the absolute cursor and its bounds; there is no separate stale cursor or new recurring joystick task.

Adjust the centre, thresholds, debounce, hold delay, repeat interval and display rate in [src/config/NavigationConfig.h](src/config/NavigationConfig.h). Hardware joystick centre/voltage still needs on-device verification if it does not settle near 512.

## Debug menu

- **Gombok:** identifies buttons wired between a safe GPIO input and GND. It covers **61 candidates: 46 digital pins plus 15 analog pins used as digital inputs**. UART D0/D1, strip data D4, buzzer D5, status output D13, I2C D20/D21, menu/select D53 and analog joystick A9 are excluded. Any additional configured output pin is skipped. Missing internal pull-ups are temporarily enabled; a pulled-down pin is detected after 30 ms debounce. Analog button results use **A0-A15** labels, not the underlying Mega GPIO numbers.
	- On entry, free workbook GPIO lamps are held at their **inactive level**: currently **D2, D3, D16, D17, D18, D19 LOW**. These six pins show OUT, leaving **55 tested inputs** within the same fixed 65-row list. Strip D4 and outputs already owned elsewhere are untouched. Back, view change and timeout release only test-owned lamp pins and restore their original floating/pull-up input settings. Future LOW-active lamps use HIGH (OFF), not LOW. Pressing a paired button does not light its lamp.
	- **Utolso:** most recently detected press, retained after release so short presses remain visible until the next press or test entry.
	- **Lenyomva:** currently pressed pins. **Darab:** current pressed count. **Tesztelt:** number of safe inputs participating.
	- Summaries show up to three IDs plus a **+N** overflow count; subsequent fixed rows show every candidate individually as HIGH/LOW/OUT. The list never changes size while buttons are pressed, so results do not move the cursor.
	- Leaving Gombok (Back, another view, or the two-minute auto-return) restores only pull-ups added by this test, including analog GPIO pull-ups. Existing digital/analog pull-ups remain, and a pin changed to an output by another owner is never overwritten. A9 is never modified, so navigation remains available.
- **Digitalis ki:** workbook-defined, safe digital outputs that are not already reserved by the firmware. The current workbook exposes the six Ikarus LED outputs, in GPIO order: **D2, D3, D16, D17, D18, D19**. It never drives UART, strip D4, buzzer D5, status D13, I2C D20/D21, game D39 or menu D53.
	- **DO allapot** shows the selected pin and its last driven state: initially **D2 ---**, then **D2 LOW** or **D2 HIGH** when driven.
	- **Kov pin / Elozo** cycle through safe workbook `DigitalOut` GPIO with `ActiveLevel` HIGH or LOW. WS2812B and unspecified entries are excluded. LOW/HIGH are literal GPIO levels, not logical OFF/ON; all current lamps are HIGH-active.
	- **LOW / HIGH** drive the currently selected pin as a digital output. Changing to another pin resets the previously driven test pin to LOW input mode. Leaving the submenu, leaving Debug or timing out resets only the currently driven test pin the same way.
- **LED szalag:** test the addressable strip on **D4**. Set **LED db** to the physical strip length (**1-100**, default 100), confirm it, then choose **Inditas**. The first LED lights white; each short **joystick/select-button click** advances the single white pixel, wrapping from the last selected LED back to the first. The LCD shows **LED 1/100**, **LED 2/100**, etc., using one-based positions.
	- While the test screen is active, up/down movements are ignored; the click is the stepper and there is no automatic movement or held-button repeat.
	- **Hold and release** the joystick/select button to exit to the strip-test menu. Two minutes without a step click clears the test and returns to the main menu. Restart, leaving the view and normal game frames also clear all test pixels.
	- The dedicated screen owns display rendering while active, so it uses its own rollover-safe inactivity timer with the same MenuIdleTimeoutSeconds setting. Clicks are handled by the input callback rather than waiting for an LCD refresh, preventing fast clicks from being lost or consumed as normal menu selections.
	- D4 has only one FastLED controller and a fixed 100-pixel buffer. Cyclon output remains 75 pixels with the remaining tail black; panel output uses mapped positions through 86. Test output lights only one white pixel at moderate brightness; no new recurring task or dynamic LED-buffer allocation is created. Maximum/default count is defined in [src/config/LedConfig.h](src/config/LedConfig.h).
- **Digitalis:** scroll through D0-D53. General input pins show **LOW/HIGH**; D39 is labelled Jatek, D53 Menu. Configured outputs show **OUT**. UART pins D0/D1, strip data D4, buzzer D5, status output D13 and I2C D20/D21 are reserved and display their role instead of being converted to inputs or sampled.
- **Analog:** scroll through all **A0-A15**, showing raw 10-bit readings **0-1023**. A9 is labelled Joy. An analog pin configured as a digital output shows OUT and is not read as an ADC input. **---** means not yet sampled; ERR denotes an invalid internal sample, not an automatic diagnosis of a disconnected sensor.
- **Allapot:** current application state; Debug indicates that the game is stopped for input testing.
- **Hibak:** separate rows for **Jatek task**, **Hang task** and **LCD 0x27**. Tasks show OK when scheduled or FALLBACK when their main-loop fallback is in use. The LCD address-only probe reports OK, NINCS (no ACK), I2C HIBA or TIMEOUT. It runs on Debug entry with a 25 ms Wire timeout; subsequent Wire timeouts remain visible until the next entry/probe.

Digitalis/Analog scanning is read-only: it never switches pin modes, drives a test output, changes pull-ups or disables PWM. Gombok adds scoped pull-ups to existing safe input GPIO and temporarily drives free, polarity-defined lamp GPIO inactive; it never repurposes reserved pins or existing outputs. On exit, original input/pull-up settings are restored only for pins still owned by the test; other unused inputs may float. A LOW reading can also mean a permanent ground connection or an external pull-down, not necessarily a mechanically pressed button. Analog voltages can be biased by the temporary pull-up during Gombok, so use the separate Analog view for voltage measurements. Never apply a voltage outside the Mega's 0-5 V input range or connect a test switch to a reserved/output pin.

The existing 5 ms task samples at most four digital pins and one analog channel per tick in Debug, plus all active button-test inputs when Gombok is open. Cached digital/analog channels refresh in about 80 ms; Gombok checks every task tick and retains short presses. The LCD redraws at 4 Hz. The first ADC conversion after switching channels is discarded to reduce cross-talk. The full analog sweep runs only in Debug; normal A9 navigation is polled in other modes except the dedicated LED-test screen, where directions are ignored. These are input observations plus task/LCD communication checks, not a full automatic hardware self-test (power, LED wiring and buzzer operation still require physical testing).

## Cooperative multitasking and the buzzer

The Mega uses **TaskManagerIO cooperative scheduling**, not OS threads or a new RTOS. The game/input task and audio task are registered once at startup; both have separate 5 ms update intervals in [src/config/BoardConfig.h](src/config/BoardConfig.h). The LCD/input framework has its own scheduled work. A task callback does a small amount of work and returns; no task waits for a note or an animation to finish. Arduino's timer-backed tone function generates the actual buzzer waveform asynchronously.

The game task requests one-shot feedback; Sorrend, Flasher, Kapcsolozar and Parbaj reuse Cyclon's Win/Loss patterns. Sorrend3–5 request Warning at their per-answer thresholds; Kapcsolozar4/Parbaj at their2-second deadline/preview thresholds. Random2 requests Playful on a fresh press, not each disco frame. The audio task advances effects and Random's short tone independently. Cancellation/result completion stops unfinished effects. No standalone music source is selectable. All modes use the same two tasks; failed scheduling has per-task main-loop fallback, not callback allocation retries.

Customize pitches, note lengths and silent gaps in the WinPattern and LossPattern tables in [src/audio/SoundEffects.cpp](src/audio/SoundEffects.cpp). They live in AVR flash and use no dynamic allocation. Late audio updates skip expired notes; feedback completion also cancels stale audio so it cannot carry into the next level.

Keep these state machines on the existing two-task cooperative loop; do not add a recurring task per game selection or press. They are not thread-safe or intended to be called from an ISR. A future true multithreaded port would need queued commands or synchronization around shared state and buzzer ownership. WS2812B transmission can briefly mask timer interrupts on AVR, so small audio/timing jitter still needs checking on the device; adding a cooperative task does not remove that hardware limitation.

## Build and regression tests

[platformio.ini](platformio.ini) pins the tested AVR platform, toolchain and eight libraries. The unused LiquidCrystal_I2C dependency has been removed. Project sources are compiled with Wall, Wextra and Werror, with an unused-parameter exception for tcMenu/IoAbstraction's inline callback headers. Upstream library compilation warnings are not promoted to errors, and the native portable modules use strict warnings without that exception.

Use **Terminal > Run Build Task** for **Build Gyerek_jatek (Mega)**. The VS Code test task **Test Gyerek_jatek (no hardware)** invokes [tools/run_tests.py](tools/run_tests.py).

The runner uses Python's standard unittest for project/configuration checks and compiles the actual portable C++ modules for native regression tests. No Mega, serial port or upload is involved. It locates g++/clang++/Zig on PATH, or the optional pinned Zig package in the current Python or PlatformIO Python environment. If no native compiler is installed, install the package specified in [tools/requirements-test.txt](tools/requirements-test.txt) into a Python environment, or provide a C++ compiler. The runner also accepts a sanitizer flag when the native compiler supports undefined-behavior instrumentation. Every PlatformIO build also runs the read-only project/configuration checks through [tools/check_build.py](tools/check_build.py); this preflight requires no native compiler.

Regression coverage includes debounce/held inputs, Cyclon profiles and asymmetric boundaries, growing white-target Sorrend, distractor bounds/visibility, per-answer deadlines/warnings, Kapcsolozar whole-vector matching/inversion and Parbaj palette/cue/arbitration/release gating, plus timing/rollover/cancellation. Existing audio, LCD, navigation, diagnostics, ownership, workbook and LED-test checks remain. Project checks retain matching menu/designer IDs/action callbacks, one strip controller and exactly two permanent application tasks. Native tests do not certify physical wiring or timing.

**Historical Random 2 verification, 2026-10-19 (before Sorrend update):** Mega build and the full normal and undefined-behavior-sanitized suites passed: **126 native tests and 33 Python checks**. Static SRAM: **4,123 / 8,192 bytes**; flash: **62,336 / 253,952 bytes**. Those regression cases cover all 32 logical inputs, chords, debounce/hold/retrigger/reselection/cancellation, disco timing and rollover, corrected panel/metal/Ikarus pairs, GPIO ownership/restoration/bounds, and melody timing/replacement/cancellation. This is not full-suite evidence for the later Sorrend revision.

**Historical Sorrend update, 2026-10-19:** Mega build and full normal/undefined-behavior-sanitized suites passed: **135 native tests and34 Python checks**, static SRAM **4,168 /8,192 bytes**, flash **63,386 /253,952 bytes**. These figures/results predate the corrected profiles/new games and are not current evidence.

**Current implementation session, 2026-10-20:** full normal and undefined-behavior-sanitized suites passed: **157 native tests and 36 Python checks** each. Mega build successful: static SRAM **4,356 / 8,192 bytes**, flash **67,936 / 253,952 bytes**. Static SRAM excludes runtime stack/heap. No upload or hardware verification for this revision; corrected profiles, new games, Random2 and the lower-row correction need device acceptance. Historical upload output does not validate this revision. The workbook and user-owned [docs/SESSION_HANDOFF.md](docs/SESSION_HANDOFF.md) are unchanged.

Native tests cannot verify real FastLED interrupt behavior, I2C communication, supply capacity or buzzer timing. Run those checks on the actual hardware before deployment.

## tcMenu designer and regeneration

[Gyerek_jatek_arduinomega.emf](Gyerek_jatek_arduinomega.emf) remains the menu definition. [src/Gyerek_jatek_arduinomega_menu.cpp](src/Gyerek_jatek_arduinomega_menu.cpp) and [src/Gyerek_jatek_arduinomega_menu.h](src/Gyerek_jatek_arduinomega_menu.h) were originally produced by Designer 4.4.0; the current firmware is verified against pinned tcMenu 4.5.9. The generated header date is provenance, not a claim that the newer designer was run.

The unsafe renderer plugin was replaced with a project-owned **SafeLiquidCrystalRenderer**, not edited in place under a generated filename. When regenerating the menu:

1. Preserve the SafeLiquidCrystalRenderer include, extern declaration and global renderer binding in the generated menu pair.
2. Do not compile a regenerated legacy renderer or another Arduino main/sketch alongside the existing entry point. Keep application implementations in their dedicated modules.
3. Preserve read-only diagnostic flags, callbacks and labels from the updated designer definition. Keep the menuInput::begin binding and configured display rate; do not regenerate the old accelerated setupAnalogJoystickEncoder input alongside it.
4. Run the project checks and native tests, then build the Mega environment. The project checks deliberately fail if unsafe/generated duplicates are reintroduced; PlatformIO also enforces these checks before compilation.

Greensleeves arrangement and score attribution are retained in [src/music/Melody.cpp](src/music/Melody.cpp). The custom LCD renderer retains the original tcMenu Apache license attribution.
