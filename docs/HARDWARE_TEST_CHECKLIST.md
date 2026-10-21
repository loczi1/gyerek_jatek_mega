# Hardware regression checklist

Use after changes affecting inputs, navigation, menu behavior, audio or LED output. Native tests and a successful build do not replace these device checks.

**Status:** The user reported testing the firmware on 2026-10-11 and said it was OK so far. Individual cases below remain **unrecorded**, not necessarily untested. Tick them only when their result is specifically confirmed.

**Historical 2026-10-15 menu/panel/Flasher update:** that update's Mega build and full suite passed: **118 native tests and 32 Python checks**; static SRAM **3,981 / 8,192 bytes**, flash **60,542 / 253,952 bytes**. Random, Simon (now user-facing Sorrend), Sima villogó (mode `Flasher`), the lower-row correction and the menu/cancellation behavior were **hardware untested**; **no firmware was uploaded** for that update. These results do not validate later revisions.

The **2026-10-14** workbook changes the strip to D4 and the game button to D39, and defines six Ikarus switch/LED pairs. These updated assignments have not been confirmed on hardware by this session.

**Historical 2026-10-19 Random 2 update (before Sorrend changes):** Mega build passed, **126 native tests and 33 Python checks** passed normally and with undefined-behavior instrumentation; static SRAM **4,123 / 8,192 bytes**, flash **62,336 / 253,952 bytes**. No upload or hardware tests were performed for that update. Random 2 cases below remain unchecked; this evidence does not verify the later Sorrend revision.

**Historical 2026-10-19 Sorrend update:** Mega build and full normal/undefined-behavior-sanitized suites passed: **135 native tests and34 Python checks**; static SRAM **4,168 /8,192 bytes**, flash **63,386 /253,952 bytes**. No upload/hardware verification was performed for that update. This does not validate the corrected profiles/new games.

**Current implementation session, 2026-10-20:** full normal and undefined-behavior-sanitized suites passed: **157 native tests and 36 Python checks** each. Mega successful: static SRAM **4,356 / 8,192 bytes**, flash **67,936 / 253,952 bytes**. No upload/hardware verification for this revision. Workbook and user-owned handoff unchanged. All acceptance cases below remain unchecked.

The earlier **2026-10-20 documentation-only GameLevel review is historical, now superseded by implementation**. Current cases cover implemented Cyclon2–5, white-target memory Sorrend2–5, Kapcsolozar3/4 and Parbaj; see [implemented rules](EXCEL_CONFIG_ARCHITECTURE.md#implemented-gamelevel-rules-2026-10-20). `GameLevel` still is not imported by the generator.

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
- [ ] Use at least **86 physical strip pixels** for all 11 panel lamps (100-pixel buffer; Cyclon alone uses 75). Verify gameplay mapping: BTN01/02/03 → 84/85/86; corrected lower five BTN07/08/09/10/11 → **75/74/73/72/71**; BTN12/13/14 → 83/82/81. Excel exports stay unchanged; only the lower lamp association is reversed in PanelConfig. Check every Sorrend playback/target/answer, including Szint4/5, and Flasher target/answer against the physically corresponding button; the previously reported reversed row must be eliminated. These games use the same 11 inputs.
- [ ] Confirm the strip is no longer on D2: D2 is now Ikarus LED #1. Verify every Ikarus lamp's rated voltage/current, current limiting/driver and common ground before output testing; never apply lamp-supply voltage to a Mega GPIO.
- [ ] Connect button tests only between an eligible INPUT and GND; do not short reserved/output pins or apply voltages outside 0–5 V.
- [ ] Leave A9 connected to the joystick. Analog voltages should be checked in Analog, not while Gombok adds temporary pull-ups.

## Menu and navigation

- [ ] Root contains Szint1–5, Ujrainditas and Debug. Szint1 unchanged: Random35, Random2:38; Szint2 unchanged placement: Cyclon10, Sorrend36, Sima villogo37 (ASCII LCD label). Szint3 order: Cyclon42, Sorrend39, Kapcsolozar45, Parbaj47; Szint4: Cyclon43, Sorrend40, Kapcsolozar46; Szint5: Cyclon44, Sorrend41. Each retains Back. No Simon mondja or music selector remains. Invoke every new action to verify its defined callback selects the correct game/profile and owning submenu.
- [ ] Short up/down movement produces one step; hold waits about 500 ms, then repeats one step per 300 ms without speeding up.
- [ ] Releasing to centre stops movement. Entering/leaving a list or holding select does not carry movement into another view.
- [ ] Scroll to both ends of Digitalis, Analog and Gombok; all expected rows are reachable without unexpected cursor jumps.
- [ ] With no game selected, normal level submenus and nested Debug views return to root after 120 seconds without menu interaction; live sample refresh and a tested button do not extend the interval.

## Game selection, cancellation and Cyclon audio

- [ ] Boot with buttons/switches held: no selected game, no sound, strip dark. Selecting/reselecting Cyclon in each Szint2–5 resets to Ready and waits for fresh D39; held D39 requires release. Verify profile changes never retain the previous stage/score/sound.
- [ ] Exercise all four internal stages for every [implemented profile](EXCEL_CONFIG_ARCHITECTURE.md#implemented-cyclon-profiles): menu2 intervals50/40/40/40 ms, widths9/5/5/3; menu3:40/35/30/30,9/5/5/3; menu4:50/40/35/20,5/3/2/2; menu5:40/35/30/20,5/3/2/1. Measure timing/jitter with suitable instrumentation, not LCD refresh. Check both accepted endpoints and immediately outside each; width2 accepts centre/+1, rejects centre−1/+2. Checked position must be the displayed LED. Verify intro, wrap, score/final fifth LED and dark tail above75.
- [ ] Win and loss each produce one distinct sound plus three matching green/red flashes. Menu remains usable.
- [ ] For **every game/profile including new challenges**, wait over120 seconds without menu interaction in Ready/active play and after results: no automatic return/deselection. Sustain timed games with valid inputs where needed; short feedback need not itself last120 seconds. Sorrend2, Kapcsolozar3 and Parbaj Input have unlimited wait; Sorrend3–5, Kapcsolozar4 and Flasher may end a round by gameplay timeout but remain selected. Selection disables tcMenu reset logic entirely via `renderer.turnOffResetLogic()`.
- [ ] For each game, manually leave its owning level submenu using Back/root or another menu during active play/feedback: game, tone and all 100 LEDs clear, including pixels above 75. No further game input is accepted until selection. Normal 120-second menu/Debug auto-return is restored after cancellation, including Debug entry and restart.
- [ ] Debug entry cancels every game and sound and clears all 100 LEDs. Returning to root does not resume or select a game; select again explicitly.
- [ ] Ujrainditas cancels every game/feedback, tone and all 100 LEDs, leaving no game selected. Repeated selection/exit does not create new recurring tasks (only the existing two application tasks).

## Szint1 — Random

- [ ] Every safe configured digital input/switch contact triggers on a fresh debounced press, including Cyclon D39, Ikarus inputs and both SW06 contacts. Menu/peripheral controls do not trigger it.
- [ ] Each press selects one of 11 panel lamps with a random hue and a 220–1760 Hz tone lasting 250 ms. A held/bouncing contact does not repeatedly trigger; release allows another press.
- [ ] Ikarus presses do not automatically light their matching built-in GPIO lamp. Confirm all 11 strip lamps are reachable with the mapping above.
- [ ] Re-selecting Random resets it. Manual Back/menu leave/Debug/restart cancels even a currently sounding tone and clears every pixel. Leaving Random idle for more than 120 seconds does not cancel selection.

## Szint1 — Random 2

- [ ] Selection with a held input is silent/dark until release and a fresh press. Every safe configured input participates; joystick/menu/peripherals do not trigger the effect.
- [ ] Check each panel button against its own lamp, particularly corrected BTN07–11 → 75/74/73/72/71. SW01–05 light pixels 76–80. Each Ikarus input lights ONLY its own GPIO lamp: D52/D2, D50/D16, D46/D17, D48/D3, D23/D18, D51/D19; HIGH-active current wiring.
- [ ] BTN05/BTN06/BTN15 (D39) and SW06 contacts D25/D24 have no assigned lamp but still play the short tune and disco. Do not invent a pair by changing the workbook.
- [ ] Each fresh press gives a 1.2-second effect, seven short notes plus one silent beat; three randomly colored pixels within strip positions 1–70 change about every 150 ms. The paired lamp remains lit throughout. Listen for unacceptable FastLED/buzzer jitter on real hardware.
- [ ] Simultaneous fresh presses light their corresponding lamps together. Another fresh press during playback replaces the old lamps and restarts the tune/disco; holding or bouncing does not repeat. After 1.2 seconds all game-owned lamps and strip pixels are dark.
- [ ] Reselect Random 2 during playback; leave Szint1; switch to Random, Cyclon, any Sorrend level or Flasher; enter Debug; restart. Each path stops the old tune immediately, clears all 100 strip pixels and releases its Ikarus outputs inactive, restoring previous input/pull-up modes. Debug Gombok and Digitalis ki still work afterward. No extra tasks accumulate.
- [ ] Wait over 120 seconds after the effect ends: Random 2 stays selected. Original Random still chooses a random panel lamp with a 250 ms tone and does NOT light a paired Ikarus GPIO lamp.

## Szint2–5 — Sorrend white-target memory (internal Simon family)

- [ ] In every level, selection/reselection resets to Ready, not autostart. A fresh any-panel-button start is consumed; selection-held input must first release. Check all11 corrected button/lamp pairs, especially lower row and repeated sequence elements.
- [ ] Check500 ms initial darkness,500 ms per white target,200 ms dark gaps; sequence grows from1 to32 after each successful whole replay, with no early cap/single-target substitution. Completion32 returns Ready for a fresh start.
- [ ] Level2: no distractors or input deadline. Wait over10 seconds and120 seconds in Input without response: no Loss or menu return. Target stays hidden; timely correctness, not elapsed time, decides.
- [ ] Levels3/4/5: respectively4–6/4–10/8–10 **additional** distinct saturated-colored nonwhite distractors, excluding that step's white target. At most11 lamps total. Verify white is distinguishable and every allowed target can occur.
- [ ] Confirm explicit source choice (not workbook-specified): distractors are refreshed per shown step and fixed within that Showing, including on replay; **only Showing displays them**. Dark gaps and Input have no target/distractor cue. Correct-answer green feedback is distinct, not an answer hint.
- [ ] Ignore holds/presses during intro/playback, including partial debounce at Input transition. Require release and a fresh answer; selection-held/feedback-held buttons must not leak into the next answer or restart.
- [ ] Per-answer deadlines start at Input entry: level3 **3000 ms**, level4 **2000 ms**, level5 **1200 ms**. Each correct answer gives540 ms green/Win, then a new full deadline; total sequence may exceed a single-answer limit. Check just-before, exactly-at and after deadline using instrumented/debounced acceptance timing: boundary must lose, never accept late.
- [ ] Warning80 ms/880 Hz at remaining **2/1 seconds on3**, **1/0.5 seconds on4/5**. Check both thresholds per answer, reset after correct feedback, no continuous/catch-up burst, no deadline extension. Level2 has no warning. Listen for actual FastLED/buzzer jitter.
- [ ] Every correct answer gives540 ms green/Win once; wrong (including a former distractor's button), chord and deadline expiry give740 ms red/Loss once, then Ready. No restart while holding failed input; new press starts a new length1 sequence. Menu selection persists.
- [ ] At every level, reselect or cancel during intro, each shown step/gap, Input/warning and green/red feedback. Back/owning-level exit/another game/Debug/restart stops all audio and clears100 pixels; no autoresume. Cancellation restores normal120-second menu timeout; subsequent selection disables it again.

## Szint3/4 — Kapcsolozar (five metal switches)

- [ ] Verify exact order SW01/02/03/04/05 → **D40/D42/D38/D36/D37**, paired lamps **76/77/78/79/80**; these are strip pixels, never new GPIO outputs. Use Digitalis to record physical up/down → literal HIGH/LOW for each. **HIGH=up is an assumption requiring confirmation**; do not silently change mappings/workbook if orientation differs.
- [ ] First selection/reselection starts a round immediately, with the five white/dark target lamps; no initial D39 needed. Exercise selection with all switches LOW/HIGH and mixed combinations; displayed mask must exclude the already solved vector, including inverse mapping on4. No instant selection win.
- [ ] Level3: lit requires HIGH and dark requires LOW on all five. Wait over2/10/120 seconds with an unsolved vector: no deadline or menu return. Confirm all-off and all-on targets, whenever selected, use the same literal-level rule.
- [ ] Level4: lit requires LOW, dark HIGH;2-second timer starts at round selection/D39 round start, not first switch movement. Verify each individual switch, all-off/all-on/mixed target patterns and both wrong HIGH and wrong LOW prevent a win.
- [ ] A whole-vector match must remain stable for30 ms. Exercise bounce, sequential switch changes, one stale wrong LOW/HIGH, and a fresh mismatch during the stability window. No accumulation of individually correct presses; never win from an old stable vector while current raw inputs mismatch.
- [ ] Level4 warnings at1/0.5 seconds remaining,80 ms/880 Hz once each. Test match stable before deadline (win), at/after2000 ms (loss); deadline wins even when the last switch completes debounce that tick. Warnings neither extend time nor survive results/cancel.
- [ ] Win gives540 ms green/Win; level4 timeout740 ms red/Loss, then dark Ready. All five lamps follow feedback. Ready requires a **fresh D39 press** for another random unsolved round; held D39 during selection/result does not restart, and moving switches alone in Ready does not start. D39 is not a substitute for solving the vector.
- [ ] Reselect during Input/warning/results: immediate reset/new round. Switch level/game, Back/root, Debug or restart during each phase: stop audio, clear all100 pixels, no later result/restart. Normal timeout returns only when selection is cancelled; Ready has no menu timeout. Check GPIO ownership and Random2/Debug still work afterward.

## Szint3 — Parbaj (upper-button color duel)

- [ ] First selection/reselection immediately starts2000 ms preview. Left BTN01–03/channel0–2/pixels84–86; right BTN12–14/channel8–10/pixels83/82/81. Each side has three distinct colors from hues0/32/64/96/160, exactly one shared between sides; verify random positions and correct physical buttons across repeated rounds.
- [ ] Preview warnings at1/0.5 seconds remaining,80 ms/880 Hz. No bottom cue before preview ends. At2000 ms, common-color cue appears at **BTN09/channel5/pixel73**, while all six upper colors stay unchanged/lit. Confirm cue location and visual color match; no lower-row reversal regression.
- [ ] Preview-time presses, holds and partially debounced presses at transition are ignored. Require all upper inputs released stably30 ms before fresh answers; no immediate result on cue appearance from a held input. Unrelated lower buttons, metal switches and D39 do not answer.
- [ ] Wait beyond2/10/120 seconds after cue with no answer: no response deadline, warning repeat, menu return or deselection. Both upper colors and cue remain visible.
- [ ] Each side separately: first accepted correct common-color button wins (that side green, other red); wrong button loses (that side red, other green). Same-side chord including the correct button is wrong. Try opposite-side and same-side bounce/held/repeated inputs; exactly one result/audio event per round.
- [ ] With a synchronized setup, answers accepted in the **same update**: both correct → both green/draw; both wrong → both red/draw; mixed → only correct side green. Include same-side chord versus correct opponent. A later answer must not alter an already resolved round; physical near-simultaneity alone is not proof of same-update acceptance.
- [ ] Result is three90 ms on/90 ms off blinks,540 ms total; cue and unrelated pixels dark. Win audio once if any accepted answer correct (including both-green draw), Loss once otherwise (including both-red draw); longer Loss pattern stops at540 ms, no stale note in Ready.
- [ ] After feedback, Ready is dark. Fresh **upper-button** press starts another preview and is consumed, not answered. Held/partial result presses must release first; D39/bottom buttons do not restart. Reselection autostarts immediately.
- [ ] Cancel/reselect during preview, both warnings, cue/Input, each result blink and Ready. Another game, owning-level exit, Debug/restart stops old audio and clears100 pixels with no autoresume; normal120-second menu timeout restored on cancellation. Repeated entry/exit keeps exactly two permanent application tasks and one strip controller/buffer.

## Szint2 — Sima villogó (LCD: Sima villogo; mode Flasher)

- [ ] Selection waits for a fresh press of any of the same 11 panel buttons as Sorrend. The start press is consumed, not counted as an answer; held playback input must be released before answering. Re-selection resets to Ready, including during play/feedback.
- [ ] After the initial 500 ms pause, a single target lights for 500 ms, goes dark for 200 ms, then the same target reappears and **remains visible while waiting for its matching button**. This is nonmemory play, not basic Sorrend's hidden sequence.
- [ ] A correct matching press gives 540 ms green feedback and the same rising Win chime, once, then chooses a new single target. Play beyond 32 correct rounds: no accumulated/growing sequence and no 32-step completion; rounds remain unlimited. In contrast, Sorrend at every level grows to 32 steps with the profiles above.
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
- [ ] Record pass/fail, unexpected behavior, board/configuration details and the revision tested here. [SESSION_HANDOFF.md](SESSION_HANDOFF.md) is user-owned: do not edit it.

Do not deliberately short the I2C bus or other outputs to manufacture a fault. Missing-LCD/error-path testing requires a safe controlled setup; a failed display cannot report its own error visibly.
