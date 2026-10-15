# Hardware regression checklist

Use after changes affecting inputs, navigation, menu behavior, audio or LED output. Native tests and a successful build do not replace these device checks.

**Status:** The user reported testing the firmware on 2026-10-11 and said it was OK so far. Individual cases below remain **unrecorded**, not necessarily untested. Tick them only when their result is specifically confirmed.

**2026-10-15 menu/panel/Flasher update:** the current session's Mega build and full suite passed: **118 native tests and 32 Python checks**; static SRAM **3,981 / 8,192 bytes**, flash **60,542 / 253,952 bytes**. Random, Simon, Sima villogó (mode `Flasher`), the lower-row correction and the new menu/cancellation behavior are **hardware untested**; **no firmware was uploaded** for this update. Older evidence does not validate this revision. This documentation-only update did not rerun the build or suite.

The **2026-10-14** workbook changes the strip to D4 and the game button to D39, and defines six Ikarus switch/LED pairs. These updated assignments have not been confirmed on hardware by this session.

## Record before testing

| Field | Value to record |
| --- | --- |
| Date / tester | |
| Board / firmware revision or build identifier | |
| Uncommitted firmware changes, if any | |
| Physical LED count / selected LED db | |
| Power supply and wiring configuration | |
| Native tests / build result and date | |
| Observed issues or deviations | |

## Safety and setup

- [ ] Use the correct Mega 2560 firmware and an appropriately rated external 5 V strip supply with common ground.
- [ ] Confirm LED data D4, buzzer D5, game D39, menu/select D53, joystick A9, potentiometer A15, and LCD I2C D20/D21 at 0x27.
- [ ] Use at least **86 physical strip pixels** for all 11 panel lamps (100-pixel buffer; Cyclon alone uses 75). Verify gameplay mapping: BTN01/02/03 → 84/85/86; corrected lower five BTN07/08/09/10/11 → **75/74/73/72/71**; BTN12/13/14 → 83/82/81. Excel exports stay unchanged; only the lower lamp association is reversed in PanelConfig. Check every Simon playback/answer and Flasher target/answer against the physically corresponding button; the previously reported reversed row must be eliminated. Both games use the same 11 inputs.
- [ ] Confirm the strip is no longer on D2: D2 is now Ikarus LED #1. Verify every Ikarus lamp's rated voltage/current, current limiting/driver and common ground before output testing; never apply lamp-supply voltage to a Mega GPIO.
- [ ] Connect button tests only between an eligible INPUT and GND; do not short reserved/output pins or apply voltages outside 0–5 V.
- [ ] Leave A9 connected to the joystick. Analog voltages should be checked in Analog, not while Gombok adds temporary pull-ups.

## Menu and navigation

- [ ] Root contains Szint1, Szint2, Szint3, Szint4, Szint5, Ujrainditas and Debug. Szint1 offers Random; Szint2 offers Cyclon, Simon mondja, then Sima villogó with the ASCII LCD label **Sima villogo**. Szint3–5 intentionally contain Back only; no music selector.
- [ ] Short up/down movement produces one step; hold waits about 500 ms, then repeats one step per 300 ms without speeding up.
- [ ] Releasing to centre stops movement. Entering/leaving a list or holding select does not carry movement into another view.
- [ ] Scroll to both ends of Digitalis, Analog and Gombok; all expected rows are reachable without unexpected cursor jumps.
- [ ] With no game selected, normal level submenus and nested Debug views return to root after 120 seconds without menu interaction; live sample refresh and a tested button do not extend the interval.

## Game selection, cancellation and Cyclon audio

- [ ] Boot with buttons held: no selected game, no sound, strip dark. Selecting Cyclon in Szint2 waits for a fresh D39 press; release held D39 before starting.
- [ ] Check all four internal Cyclon difficulties and target-window boundaries; movement wraps without stale pixels. These are separate from root Szint1–5.
- [ ] Win and loss each produce one distinct sound plus three matching green/red flashes. Menu remains usable.
- [ ] For **each selected game (Cyclon, Random, Simon and Flasher)**, wait longer than 120 seconds without menu interaction across ready, gameplay and result states, including ready after completion/failure: **no automatic menu return or deselection** occurs. Simon's and Flasher's 10-second input inactivity timeouts may end a round but must leave the game selected. Keep providing game inputs to extend active play beyond 120 seconds; individual feedback phases are shorter. Selection disables tcMenu reset logic entirely via `renderer.turnOffResetLogic()`.
- [ ] For each game, manually leave its owning level submenu using Back/root or another menu during active play/feedback: game, tone and all 100 LEDs clear, including pixels above 75. No further game input is accepted until selection. Normal 120-second menu/Debug auto-return is restored after cancellation, including Debug entry and restart.
- [ ] Debug entry cancels every game and sound and clears all 100 LEDs. Returning to root does not resume or select a game; select again explicitly.
- [ ] Ujrainditas cancels every game/feedback, tone and all 100 LEDs, leaving no game selected. Repeated selection/exit does not create new recurring tasks (only the existing two application tasks).

## Szint1 — Random

- [ ] Every safe configured digital input/switch contact triggers on a fresh debounced press, including Cyclon D39, Ikarus inputs and both SW06 contacts. Menu/peripheral controls do not trigger it.
- [ ] Each press selects one of 11 panel lamps with a random hue and a 220–1760 Hz tone lasting 250 ms. A held/bouncing contact does not repeatedly trigger; release allows another press.
- [ ] Ikarus presses do not automatically light their matching built-in GPIO lamp. Confirm all 11 strip lamps are reachable with the mapping above.
- [ ] Re-selecting Random resets it. Manual Back/menu leave/Debug/restart cancels even a currently sounding tone and clears every pixel. Leaving Random idle for more than 120 seconds does not cancel selection.

## Szint2 — Simon mondja

- [ ] Selection waits for a fresh press of any of the 11 panel buttons; the start press is consumed rather than counted as an answer. Re-selection resets the sequence.
- [ ] Playback starts after 500 ms; each light lasts 500 ms with 200 ms gaps. Sequence grows from 1 to at most 32 random steps.
- [ ] Press/hold during playback is ignored; release is required before answering. Repeat the displayed sequence on the same buttons, including repeated sequence elements.
- [ ] Every correct press produces 540 ms green feedback with the same rising Cyclon Win chime, once per accepted press.
- [ ] Wrong input, a multi-button chord or 10 seconds without an input produces 740 ms red feedback with Cyclon Loss. After failure or completion of 32 steps, Ready waits for a fresh start press.
- [ ] Cancel during initial pause, playback, input and green/red feedback via manual owning-submenu exit, Debug and restart; no stale pixels, sound or automatic resume remain. A 10-second unanswered round returns to Ready after loss feedback without deselecting Simon; waiting more than 120 seconds in Ready does not return to root.

## Szint2 — Sima villogó (LCD: Sima villogo; mode Flasher)

- [ ] Selection waits for a fresh press of any of the same 11 panel buttons as Simon. The start press is consumed, not counted as an answer; held playback input must be released before answering. Re-selection resets to Ready, including during play/feedback.
- [ ] After the initial 500 ms pause, a single target lights for 500 ms, goes dark for 200 ms, then the same target reappears and **remains visible while waiting for its matching button**. This is nonmemory play, not Simon's hidden sequence.
- [ ] A correct matching press gives 540 ms green feedback and the same rising Win chime, once, then chooses a new single target. Play beyond 32 correct rounds: no accumulated/growing sequence and no 32-step completion; rounds remain unlimited. Existing Simon still grows to 32 steps unchanged.
- [ ] A wrong button, multi-button chord and 10 seconds of input inactivity each give 740 ms red feedback and the same Loss sound, then Ready waits for a fresh start press. Held input must not automatically restart; a fresh press starts a reset round.
- [ ] Menu timeout stays disabled in Ready, initial pause, shown/dark playback, visible-target input, green/red feedback and Ready after failure. Wait more than 120 seconds in Ready and keep answering to sustain play beyond 120 seconds without menu interaction; neither returns to root. The 10-second input timeout must not deselect Flasher.
- [ ] During each phase, manually leave the owning level (Back/root/another menu), enter Debug or restart: game and tone stop, **all 100 pixels clear**, normal 120-second menu/Debug timeout returns, and no stale sound/pixels or automatic resume remain. Select again explicitly to play.

## Gombok — digital and analog GPIO

- [ ] With the 2026-10-15 workbook, entering Gombok holds D2/D3/D16/D17/D18/D19 LOW (HIGH-active transistor lamps OFF); these rows show OUT and Tesztelt shows 55. D4 remains exclusively strip data.
- [ ] Back, another view and two-minute auto-return release the temporary lamp outputs and restore prior input/pull-up settings. Re-entry works. No paired button press automatically lights its lamp. Outputs already owned elsewhere remain untouched.
- [ ] Check the game button on D39, one otherwise unused safe digital input, and available analog GPIO (including A0 and A15).
- [ ] Verify both SW06 contacts on D25/D24. Check potentiometer readings on A15 in Analog, outside the button test.
- [ ] Released reads HIGH and pressed-to-GND reads LOW after debounce; reports use D/A pin names correctly.
- [ ] A short press remains visible in Utolso after release; Lenyomva and Darab reflect current state.
- [ ] Multiple presses display up to three IDs plus +N; individual fixed rows show the remaining states.
- [ ] UART, I2C, strip D4, D5, D13, D53, A9 and configured outputs are not repurposed. Existing workbook-input pull-ups remain intact.
- [ ] Back and auto-return restore only pull-ups added by Gombok. Re-entering Analog shows unbiased measurements again.

## Ikarus switches and Digitalis ki

- [ ] Verify input/LED pairs: #1 D52/D2, #2 D50/D16, #3 D46/D17, #4 D48/D3, #5 D23/D18, #6 D51/D19.
- [ ] Each switch contact changes its own Gombok/Digitalis input. It does not automatically command its own LED, including when used by Random.
- [ ] Digitalis ki selects D2, D3, D16, D17, D18, D19 in GPIO order, wrapping with Kov pin/Elozo. Reserved strip D4 and game D39 never appear.
- [ ] LOW/HIGH affects only the selected, correctly interfaced LED. The status matches the commanded GPIO level; verify the actual lamp polarity separately.
- [ ] Changing selection, Back and two-minute auto-return reset the driven test pin to LOW input mode without leaving another tested LED driven or changing the game/strip pins.

## LED szalag — manual D4 strip test

- [ ] Set LED db to the actual strip length (1–100), confirm it, then select Inditas. Exactly the first LED is white.
- [ ] Each short joystick-button click advances exactly one pixel; LCD LED numbering matches the physical LED, starting at 1.
- [ ] The last configured pixel wraps to the first. Holding select does not repeatedly step, and directional joystick input does not move the pixel.
- [ ] Hold and release select to exit: all 100 buffered pixels are cleared, including positions beyond the 75-pixel game area.
- [ ] Two minutes without step clicks clears the test and returns to root; a step click renews the interval.
- [ ] Return to Cyclon after testing an LED beyond position 75: no white pixels remain in the unused tail.

## Diagnostics and result

- [ ] Digitalis/Analog readings update live; unconnected-input fluctuations are not treated as proven faults.
- [ ] Allapot and Hibak are readable; game/audio tasks show OK under normal operation, and LCD health is consistent with the connected hardware.
- [ ] Record pass/fail, unexpected behavior, board/configuration details and the revision tested. Update [SESSION_HANDOFF.md](SESSION_HANDOFF.md) only with confirmed evidence.

Do not deliberately short the I2C bus or other outputs to manufacture a fault. Missing-LCD/error-path testing requires a safe controlled setup; a failed display cannot report its own error visibly.
