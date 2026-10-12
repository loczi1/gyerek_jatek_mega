# Hardware regression checklist

Use after changes affecting inputs, navigation, menu behavior, audio or LED output. Native tests and a successful build do not replace these device checks.

**Status:** The user reported testing the firmware on 2026-10-11 and said it was OK so far. Individual cases below remain **unrecorded**, not necessarily untested. Tick them only when their result is specifically confirmed.

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
- [ ] Confirm LED data D2, buzzer D5, game D51, menu/select D53, joystick A9, and LCD I2C D20/D21 at 0x27.
- [ ] Connect button tests only between an eligible INPUT and GND; do not short reserved/output pins or apply voltages outside 0–5 V.
- [ ] Leave A9 connected to the joystick. Analog voltages should be checked in Analog, not while Gombok adds temporary pull-ups.

## Menu and navigation

- [ ] Root contains only Jatek, Ujrainditas and Debug; no music selector or empty placeholders.
- [ ] Short up/down movement produces one step; hold waits about 500 ms, then repeats one step per 300 ms without speeding up.
- [ ] Releasing to centre stops movement. Entering/leaving a list or holding select does not carry movement into another view.
- [ ] Scroll to both ends of Digitalis, Analog and Gombok; all expected rows are reachable without unexpected cursor jumps.
- [ ] After two minutes without menu interaction, nested Debug views return to root; live sample refresh and a tested button do not extend the interval.

## Game and audio

- [ ] Boot with D51 held: no automatic game start. Release and press to start.
- [ ] Check all four levels and target-window boundaries; movement wraps without stale pixels.
- [ ] Win and loss each produce one distinct sound plus three matching green/red flashes. Menu remains usable.
- [ ] Debug entry cancels the round and sound. Returning to root restores idle, and a held D51 cannot immediately restart.
- [ ] Ujrainditas cancels feedback and returns to the ready screen.

## Gombok — digital and analog GPIO

- [ ] Check the game button on D51, one otherwise unused safe digital input, and available analog GPIO (including A0 and A15).
- [ ] Released reads HIGH and pressed-to-GND reads LOW after debounce; reports use D/A pin names correctly.
- [ ] A short press remains visible in Utolso after release; Lenyomva and Darab reflect current state.
- [ ] Multiple presses display up to three IDs plus +N; individual fixed rows show the remaining states.
- [ ] UART, I2C, D2, D5, D13, D53, A9 and configured outputs are not repurposed. Existing pull-ups remain intact.
- [ ] Back and auto-return restore only pull-ups added by Gombok. Re-entering Analog shows unbiased measurements again.

## LED szalag — manual D2 strip test

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
