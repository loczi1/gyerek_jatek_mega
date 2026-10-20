"""Read-only project checks, including tcMenu regeneration regressions."""

import configparser
import json
from pathlib import Path
import re
import unittest


ROOT = Path(__file__).resolve().parents[1]


class ProjectTests(unittest.TestCase):
    def test_workspace_contains_only_this_project(self):
        workspace = json.loads((ROOT / "Gyerek_jatek.code-workspace").read_text(encoding="utf-8"))
        self.assertEqual(workspace["folders"], [{"name": "Gyerek_jatek", "path": "."}])
        self.assertFalse((ROOT / "Csiraztato_io.code-workspace").exists())
        self.assertFalse((ROOT / "src/Gyerek_jatek.code-workspace").exists())

    def test_dependencies_are_exact_and_unused_lcd_is_removed(self):
        config = configparser.ConfigParser()
        config.read(ROOT / "platformio.ini", encoding="utf-8")
        environment = config["env:megaatmega2560"]
        self.assertEqual(environment["platform"], "atmelavr@5.3.0")
        self.assertIn("-Werror", environment["build_src_flags"])
        self.assertIn("pre:tools/generate_config.py", environment["extra_scripts"])
        self.assertIn("pre:tools/check_build.py", environment["extra_scripts"])
        dependencies = environment["lib_deps"].split()
        self.assertEqual(len(dependencies), 8)
        for dependency in dependencies:
            self.assertRegex(dependency, r"^[^@]+@\d+\.\d+\.\d+$")
            self.assertNotIn("LiquidCrystal_I2C", dependency)

    def test_fixed_game_audio_tasks_and_no_blocking_delays(self):
        sources = list((ROOT / "src").rglob("*.cpp"))
        combined = "\n".join(path.read_text(encoding="utf-8") for path in sources)
        self.assertEqual(len(re.findall(r"scheduleFixedRate\s*\(", combined)), 2)
        application = (ROOT / "src/app/Application.cpp").read_text(encoding="utf-8")
        self.assertIn("if (initialized) return;", application)
        self.assertIn("updateTask = taskManager.scheduleFixedRate(board::AppTickMs, update);", application)
        self.assertIn("audioTask = taskManager.scheduleFixedRate(board::AudioTickMs, updateAudio);", application)
        callbacks = (ROOT / "src/ui/MenuCallbacks.cpp").read_text(encoding="utf-8")
        self.assertNotRegex(callbacks, r"scheduleFixedRate\s*\(")
        self.assertNotRegex(combined, r"\bdelay\s*\(")
        self.assertNotRegex(combined, r"\bString\b")
        self.assertEqual(len(re.findall(r"\bvoid\s+setup\s*\(", combined)), 1)
        self.assertEqual(len(re.findall(r"\bvoid\s+loop\s*\(", combined)), 1)
        self.assertIn("#include <Arduino.h>", (ROOT / "src/main.cpp").read_text(encoding="utf-8"))

    def test_designer_menu_matches_checked_in_menu(self):
        definition = json.loads((ROOT / "Gyerek_jatek_arduinomega.emf").read_text(encoding="utf-8"))
        source = (ROOT / "src/Gyerek_jatek_arduinomega_menu.cpp").read_text(encoding="utf-8")
        header = (ROOT / "src/Gyerek_jatek_arduinomega_menu.h").read_text(encoding="utf-8")
        callbacks = (ROOT / "src/ui/MenuCallbacks.cpp").read_text(encoding="utf-8")
        names_by_id = {
            int(identifier): name
            for name, identifier in re.findall(r'minfo\w+\s*=\s*\{\s*"([^"]+)",\s*(\d+)', source)
        }
        self.assertEqual(set(names_by_id), {entry["item"]["id"] for entry in definition["items"]})
        for entry in definition["items"]:
            item = entry["item"]
            self.assertEqual(names_by_id[item["id"]], item["name"])
            variable = item.get("variableName")
            if item.get("readOnly"):
                self.assertIn(f"menu{variable}.setReadOnly(true);", source)
            callback = item.get("functionName")
            if callback:
                self.assertIn(f"CALLBACK_FUNCTION {callback}(", header)
                self.assertIn(f"CALLBACK_FUNCTION {callback}(", callbacks)
            for index, text in enumerate(item.get("enumEntries", [])):
                self.assertIn(f'enumStr{variable}_{index}[] PROGMEM = "{text}";', source)
            if entry["type"] == "runtimeList":
                rows = item["initialRows"]
                self.assertIn(f"menu{variable}(&minfo{variable}, {rows}, fn{variable}RtCall,", source)
                self.assertIn(f"fn{variable}RtCall(", header)
                self.assertIn(f"fn{variable}RtCall(", callbacks)

        # Match every sibling and submenu child link, not only names and IDs.
        for parent_id in {entry["parentId"] for entry in definition["items"]}:
            siblings = [entry for entry in definition["items"] if entry["parentId"] == parent_id]
            for index, entry in enumerate(siblings):
                variable = entry["item"]["variableName"]
                next_item = ("&menu" + siblings[index + 1]["item"]["variableName"]
                             if index + 1 < len(siblings) else "nullptr")
                if entry["type"] == "subMenu":
                    children = [child for child in definition["items"]
                                if child["parentId"] == entry["item"]["id"]]
                    first_child = "&menu" + children[0]["item"]["variableName"] if children else "nullptr"
                    self.assertIn(f"BackMenuItem menuBack{variable}(&minfo{variable}, {first_child},", source)
                    self.assertIn(f"SubMenuItem menu{variable}(&minfo{variable}, &menuBack{variable}, {next_item},", source)
                    self.assertIn(f"extern SubMenuItem menu{variable};", header)
                    self.assertIn(f"extern BackMenuItem menuBack{variable};", header)
                else:
                    self.assertRegex(source, rf"menu{variable}\(&minfo{variable}, (?:[^;\n]*, )?{re.escape(next_item)}, INFO_LOCATION_PGM\);")

    def test_exactly_seven_root_options_and_level_hierarchy_without_music(self):
        definition = json.loads((ROOT / "Gyerek_jatek_arduinomega.emf").read_text(encoding="utf-8"))
        roots = [entry for entry in definition["items"] if entry["parentId"] == 0]
        self.assertEqual([entry["item"]["name"] for entry in roots],
                         ["Szint 1", "Szint 2", "Szint 3", "Szint 4", "Szint 5", "Ujrainditas", "Debug"])
        self.assertEqual([entry["type"] for entry in roots], ["subMenu"] * 5 + ["actionMenu", "subMenu"])
        self.assertEqual([entry["item"]["id"] for entry in roots], [30, 31, 32, 33, 34, 5, 7])
        expected_children = {
            30: [("Random", "Random", "RandomGame", 35), ("Random 2", "Random2", "RandomGame2", 38)],
              31: [("Cyclon", "Jatek", "Jatek", 10), ("Sorrend", "Simon", "SimonGame", 36),
                 ("Sima villogo", "Flasher", "FlasherGame", 37)],
              32: [("Sorrend", "Sorrend3", "SorrendGame3", 39)],
              33: [("Sorrend", "Sorrend4", "SorrendGame4", 40)],
              34: [("Sorrend", "Sorrend5", "SorrendGame5", 41)],
        }
        for parent_id, expected in expected_children.items():
            children = [entry for entry in definition["items"] if entry["parentId"] == parent_id]
            self.assertEqual([(entry["item"]["name"], entry["item"]["variableName"],
                               entry["item"].get("functionName"), entry["item"]["id"])
                              for entry in children], expected)
            self.assertTrue(all(entry["type"] == "actionMenu" for entry in children))
        source = (ROOT / "src/Gyerek_jatek_arduinomega_menu.cpp").read_text(encoding="utf-8")
        header = (ROOT / "src/Gyerek_jatek_arduinomega_menu.h").read_text(encoding="utf-8")
        self.assertIn("menuRestart(&minfoRestart, &menuDebug,", source)
        self.assertIn("menuDebug(&minfoDebug, &menuBackDebug, nullptr,", source)
        self.assertIn("menuMgr.initWithoutInput(&renderer, &menuLevel1);", source)
        self.assertIn("inline MenuItem& rootMenuItem() { return menuLevel1; }", header)
        callbacks = (ROOT / "src/ui/MenuCallbacks.cpp").read_text(encoding="utf-8")
        for variable, callback, function in (("Random", "RandomGame", "playRandom"),
                                             ("Random2", "RandomGame2", "playRandom2"),
                                             ("Jatek", "Jatek", "playGame"),
                                             ("Simon", "SimonGame", "playSimon"),
                                             ("Sorrend3", "SorrendGame3", "playSorrend3"),
                                             ("Sorrend4", "SorrendGame4", "playSorrend4"),
                                             ("Sorrend5", "SorrendGame5", "playSorrend5"),
                                             ("Flasher", "FlasherGame", "playFlasher")):
            self.assertIn(f"extern ActionMenuItem menu{variable};", header)
            self.assertRegex(source, rf"minfo{variable}\s*=\s*\{{[^\n]*, {callback} \}};")
            self.assertRegex(callbacks, rf"void CALLBACK_FUNCTION {callback}\(int\)\s*\{{\s*application::{function}\(\);\s*\}}")
        for unwanted in ("JatekValaszto", "Input naplo", "Bemenetek", "Zene", "enumStrJatek"):
            self.assertNotIn(unwanted, source)
        application = (ROOT / "src/app/Application.cpp").read_text(encoding="utf-8")
        controller = (ROOT / "src/app/ModeController.h").read_text(encoding="utf-8")
        self.assertNotIn("music::", application)
        self.assertNotIn("music/MusicPlayer.h", controller)
        # Debug owns only its own subtree, not every non-root level menu.
        debug_ids = {7, 20, 600}
        debug_children = [entry for entry in definition["items"] if entry["parentId"] in debug_ids]
        self.assertTrue(all(entry["item"]["variableName"] not in
                    ("Random", "Random2", "Jatek", "Simon", "Flasher", "Sorrend3", "Sorrend4", "Sorrend5",
                     "Level1", "Level2", "Level3", "Level4", "Level5")
                    for entry in debug_children))

    def test_sorrend_application_bindings_scopes_and_task_participation(self):
        header = (ROOT / "src/app/Application.h").read_text(encoding="utf-8")
        app = (ROOT / "src/app/Application.cpp").read_text(encoding="utf-8")
        for function, selection, mode, level in (
                ("playSimon", "Simon", "Simon", 2),
                ("playSorrend3", "Simon", "Simon", 3),
                ("playSorrend4", "Sorrend4", "Sorrend4", 4),
                ("playSorrend5", "Sorrend5", "Sorrend5", 5)):
            with self.subTest(function=function):
                self.assertIn(f"void {function}();", header)
                self.assertRegex(app, rf"void {function}\(\)\s*\{{\s*"
                                 rf"selectPanelGame\(SelectedGame::{selection},\s*"
                                 rf"game::PanelMode::{mode},\s*&menuBackLevel{level}\);\s*\}}")
        draw = app.split("void drawMode()", 1)[1].split("void synchronizeDebugView", 1)[0]
        update = app.split("void update()", 1)[1].split("void updateAudio()", 1)[0]
        audio = app.split("void updateAudio()", 1)[1].split("} // namespace", 1)[0]
        for selection in ("Sorrend4", "Sorrend5"):
            for section, operation in ((draw, "leds::drawPanel(panel);"),
                                       (update, "panel.update(now, panelPressedMask())"),
                                       (audio, "soundEffects.update(millis());")):
                with self.subTest(selection=selection, operation=operation):
                    self.assertRegex(section, rf"(?:if|else if)\s*\([^{{;]*"
                                     rf"selectedGame == SelectedGame::{selection}\b[^{{;]*\)\s*"
                                     rf"(?:\{{\s*)?(?:(?!\n    \}})[\s\S])*?{re.escape(operation)}")

    def test_panel_games_hardware_wiring_and_cancellation(self):
        app = (ROOT / "src/app/Application.cpp").read_text(encoding="utf-8")
        for binding in ("controller.update(now, gameButtonPressed())",
                        "panel.update(now, panelPressedMask())",
                        "soundEffects.update(millis())",
                        "gameScope && menuMgr.getCurrentMenu() != gameScope",
                        "gameScope = &menuBackLevel2;",
                        "game::PanelMode::Random, &menuBackLevel1",
                        "game::PanelMode::Simon, &menuBackLevel2"):
            self.assertIn(binding, app)
        scope = app.split("void synchronizeDebugView", 1)[1].split("void update()", 1)[0]
        self.assertIn("current == &menuBackDebug", scope)
        self.assertNotIn("!menuMgr.getNavigationStore().isShowingRoot()", scope)
        self.assertNotIn("menuBackLevel", scope)
        stop = app.split("void stopGames", 1)[1].split("void finishLedTest", 1)[0]
        self.assertIn("PanelMode::Disabled", stop)
        self.assertIn("soundEffects.stop();", stop)
        self.assertIn("stopTone();", stop)
        self.assertIn("selectedGame = SelectedGame::None;", stop)
        inputs = app.split("void configurePanelInputs", 1)[1].split("void stopGames", 1)[0]
        self.assertIn("board::isWorkbookDigitalInput(pin)", inputs)
        self.assertIn("debug::canTestButton(pin)", inputs)
        self.assertIn("portModeRegister(port)", inputs)
        self.assertIn("portInputRegister(port)", inputs)
        hardware = (ROOT / "src/hardware/LedDisplay.cpp").read_text(encoding="utf-8")
        frame = hardware.split("void drawPanel", 1)[1].split("void drawTest", 1)[0]
        self.assertIn("fill_solid(pixels, board::LedStripCapacity, CRGB::Black);", frame)
        self.assertIn("pixel > 0 && pixel <= board::LedStripCapacity", frame)
        self.assertIn("pixels[pixel - 1]", frame)
        self.assertIn("board::panelStripPixel(currentGame.litChannel())", frame)

    def test_random2_lamp_audio_and_cancellation_integration(self):
        app = (ROOT / "src/app/Application.cpp").read_text(encoding="utf-8")
        self.assertIn("game::PanelMode::Random2, &menuBackLevel1", app)
        self.assertIn("gameLamps::setEnabled(selection == SelectedGame::Random2);", app)
        stop = app.split("void stopGames", 1)[1].split("void finishLedTest", 1)[0]
        self.assertIn("gameLamps::setEnabled(false);", stop)
        cyclone = app.split("void playGame()", 1)[1].split("void selectPanelGame", 1)[0]
        self.assertIn("stopGames(millis());", cyclone)
        self.assertIn("gameLamps::draw(panel.lampInputMask(), panelInputs, panelInputCount);", app)
        self.assertIn("leds::drawRandom2(panel, panelInputs, panelInputCount);", app)
        self.assertIn("game::PanelFeedback::Melody ? audio::Effect::Playful", app)
        audio = app.split("void updateAudio()", 1)[1].split("} // namespace", 1)[0]
        self.assertIn("selectedGame == SelectedGame::Random2", audio)
        self.assertIn("soundEffects.update(millis());", audio)
        selection = app.split("void selectPanelGame", 1)[1].split("void playRandom()", 1)[0]
        self.assertLess(selection.index("diagnostics::setDigitalOutputTestEnabled(false)"),
                        selection.index("gameLamps::setEnabled"))
        self.assertLess(selection.index("diagnostics::setButtonTestEnabled(false"),
                        selection.index("gameLamps::setEnabled"))
        hardware = (ROOT / "src/hardware/LedDisplay.cpp").read_text(encoding="utf-8")
        frame = hardware.split("void drawRandom2", 1)[1].split("void drawTest", 1)[0]
        self.assertIn("fill_solid(pixels, board::LedStripCapacity, CRGB::Black);", frame)
        self.assertIn("board::pairedStripPixel(inputs[i])", frame)
        self.assertIn("pixel > 0 && pixel <= board::LedStripCapacity", frame)

    def test_debug_lists_cover_inputs_with_non_overlapping_ids(self):
        definition = json.loads((ROOT / "Gyerek_jatek_arduinomega.emf").read_text(encoding="utf-8"))
        children = [entry["item"] for entry in definition["items"] if entry["parentId"] == 7]
        self.assertEqual([item["name"] for item in children], ["Gombok", "LED szalag", "Digitalis ki", "Digitalis", "Analog", "Allapot", "Hibak"])
        self.assertTrue(all(item["readOnly"] for item in children if item["name"] not in ("LED szalag", "Digitalis ki")))
        self.assertEqual([item.get("initialRows") for item in children], [65, None, None, 54, 16, None, 3])
        occupied = set()
        for entry in definition["items"]:
            item = entry["item"]
            count = item.get("initialRows", 0) + 1
            identifiers = set(range(item["id"], item["id"] + count))
            self.assertTrue(occupied.isdisjoint(identifiers))
            occupied.update(identifiers)
        output_children = [entry["item"] for entry in definition["items"] if entry["parentId"] == 600]
        self.assertEqual([item["name"] for item in output_children], ["DO allapot", "Kov pin", "Elozo", "LOW", "HIGH"])
        self.assertTrue(output_children[0]["readOnly"])

    def test_debug_scanning_preserves_pin_modes_and_pwm(self):
        source = (ROOT / "src/hardware/Diagnostics.cpp").read_text(encoding="utf-8")
        readers = source.split("void begin()", 1)[0]
        readers = re.sub(r"//[^\n]*|/\*[\s\S]*?\*/", "", readers)
        self.assertNotRegex(readers, r"\b(?:pinMode|digitalWrite|digitalRead|analogWrite)\s*\(")
        self.assertIn("debug::canReadDigital(pin)", readers)
        self.assertIn("portModeRegister(port)", readers)
        self.assertIn("portInputRegister(port)", readers)
        self.assertIn("return debug::AnalogOutput;", readers)
        menu = (ROOT / "src/Gyerek_jatek_arduinomega_menu.cpp").read_text(encoding="utf-8")
        self.assertIn("Wire.setWireTimeout(board::I2cTimeoutMicros, true);", menu)

    def test_navigation_is_fixed_repeat_and_has_no_joystick_task(self):
        menu = (ROOT / "src/Gyerek_jatek_arduinomega_menu.cpp").read_text(encoding="utf-8")
        self.assertIn("menuInput::begin();", menu)
        self.assertIn("renderer.setUpdatesPerSecond(board::MenuDisplayUpdatesPerSecond);", menu)
        self.assertNotIn("setupAnalogJoystickEncoder", menu)
        self.assertNotIn("JoystickSwitchInput", menu)
        navigation = (ROOT / "src/ui/MenuInput.cpp").read_text(encoding="utf-8")
        self.assertIn("menuMgr.valueChanged(value);", navigation)
        self.assertIn("encoder.increment(step);", navigation)
        self.assertIn("repeat.reset(now);", navigation)
        self.assertEqual(len(re.findall(r"analogRead\(board::MenuJoystickPin\)", navigation)), 2)
        self.assertNotRegex(navigation, r"\bschedule(?:Once|FixedRate)\s*\(")
        configuration = (ROOT / "src/config/NavigationConfig.h").read_text(encoding="utf-8")
        self.assertRegex(configuration, r"MenuJoystickRepeatMs\s*=\s*300\s*;")
        self.assertRegex(configuration, r"MenuDisplayUpdatesPerSecond\s*=\s*4\s*;")
        definition = json.loads((ROOT / "Gyerek_jatek_arduinomega.emf").read_text(encoding="utf-8"))
        properties = {item["name"]: item["latestValue"] for item in definition["codeOptions"]["lastProperties"]}
        self.assertEqual(properties["UPDATES_PER_SEC"], "4")

    def test_button_test_pullups_are_scoped_and_output_protected(self):
        application = (ROOT / "src/app/Application.cpp").read_text(encoding="utf-8")
        self.assertIn("diagnostics::setButtonTestEnabled(debugViewing && menuMgr.getCurrentMenu() == &menuButtonDebug, now);", application)
        self.assertIn("menuMgr.getCurrentMenu() == &menuBackDigitalOutputs", application)
        hardware = (ROOT / "src/hardware/Diagnostics.cpp").read_text(encoding="utf-8")
        self.assertIn("if (!debug::canTestButton(pin)) return;", hardware)
        self.assertIn("if ((*portModeRegister(port) & mask) == 0)", hardware)
        self.assertIn("SREG = savedInterrupts;", hardware)
        tester = (ROOT / "src/debug/ButtonTester.cpp").read_text(encoding="utf-8")
        self.assertIn("pin != board::MenuButtonPin", tester)
        self.assertIn("changedPullups_.contains(pin)", tester)
        self.assertIn("setPullup_(pin, false);", tester)
        self.assertNotIn("setNumberOfRows", application)
        self.assertNotIn("setNumberOfRows", tester)

    def test_workbook_digital_outputs_drive_only_safe_debug_pins(self):
        generated = (ROOT / "src/generated/pins.h").read_text(encoding="utf-8")
        self.assertIn("IkarusSwitches[]", generated)
        self.assertIn("Dio01Ikarus1,", generated)
        tester = (ROOT / "src/debug/DigitalOutputTester.cpp").read_text(encoding="utf-8")
        self.assertIn("digitalRole(pin) == DigitalRole::Input", tester)
        self.assertIn("definition.type == generated::pins::PinType::DigitalOut", tester)
        self.assertIn("definition.primaryPin == pin", tester)
        hardware = (ROOT / "src/hardware/Diagnostics.cpp").read_text(encoding="utf-8")
        self.assertIn("pinMode(pin, OUTPUT);", hardware)
        self.assertIn("pinMode(pin, INPUT);", hardware)

    def test_workbook_button_contacts_and_pot_drive_initialization(self):
        hardware = (ROOT / "src/hardware/Diagnostics.cpp").read_text(encoding="utf-8")
        self.assertIn("debug::canTestButton(pin)", hardware)
        self.assertIn("pin == board::GameButtonPin || board::isWorkbookDigitalInput(pin)", hardware)
        self.assertIn("pinMode(pin, INPUT_PULLUP);", hardware)
        self.assertNotIn("FirstDiagnosticPin", hardware)
        pins = (ROOT / "src/config/PinConfig.h").read_text(encoding="utf-8")
        self.assertIn("definition.secondaryPin == pin", pins)
        self.assertIn("definition.tertiaryPin == pin", pins)
        self.assertIn("generated::pins::ControlId::Pot1", pins)
        board = (ROOT / "src/config/BoardConfig.h").read_text(encoding="utf-8")
        self.assertIn("AnalogInputPin = A0 + PotentiometerChannel;", board)

    def test_analog_button_gpio_and_joystick_exclusion(self):
        tester = (ROOT / "src/debug/ButtonTester.cpp").read_text(encoding="utf-8")
        self.assertIn("pin != board::FirstAnalogGpioPin + board::MenuJoystickChannel", tester)
        self.assertIn("InputMonitor::formatAnalogName", tester)
        hardware = (ROOT / "src/hardware/Diagnostics.cpp").read_text(encoding="utf-8")
        self.assertIn("if (!debug::canTestButton(pin)) return false;", hardware)
        self.assertIn("return (*portInputRegister(port) & mask) == 0;", hardware)
        header = (ROOT / "src/debug/ButtonTester.h").read_text(encoding="utf-8")
        self.assertIn("changedAt_[board::GpioPinCount]", header)

    def test_led_strip_menu_count_and_single_workbook_pin_controller(self):
        definition = json.loads((ROOT / "Gyerek_jatek_arduinomega.emf").read_text(encoding="utf-8"))
        children = [entry for entry in definition["items"] if entry["parentId"] == 20]
        self.assertEqual([entry["item"]["name"] for entry in children], ["LED db", "Inditas"])
        count = children[0]
        self.assertEqual(count["type"], "analogItem")
        self.assertEqual(count["defaultValue"], "99")
        self.assertEqual(count["item"]["maxValue"], 99)
        self.assertEqual(count["item"]["offset"], 1)
        hardware = (ROOT / "src/hardware/LedDisplay.cpp").read_text(encoding="utf-8")
        self.assertEqual(hardware.count("FastLED.addLeds<"), 1)
        self.assertIn("board::LedDataPin", hardware)
        self.assertIn("CRGB pixels[board::LedStripCapacity];", hardware)
        self.assertIn("fill_solid(pixels, board::LedStripCapacity, CRGB::Black);", hardware)
        self.assertIn("pixels[test.position()] = CRGB::White;", hardware)
        config = (ROOT / "src/config/LedConfig.h").read_text(encoding="utf-8")
        self.assertRegex(config, r"LedStripCapacity\s*=\s*100\s*;")

    def test_led_test_click_exit_and_timeout_are_scoped(self):
        application = (ROOT / "src/app/Application.cpp").read_text(encoding="utf-8")
        self.assertIn("menuMgr.getCurrentMenu() != &menuBackLedStrip", application)
        self.assertIn("renderer.takeOverDisplay(renderLedTest);", application)
        self.assertIn("renderer.giveBackDisplay();", application)
        self.assertIn("if (returnToRoot) menuMgr.resetMenu(true);", application)
        self.assertIn("ledTest.hasTimedOut(now)", application)
        self.assertIn("ledTest(board::MenuIdleTimeoutSeconds * 1000UL)", application)
        self.assertIn("if (held)", application)
        self.assertIn("ledTest.step(millis());", application)
        navigation = (ROOT / "src/ui/MenuInput.cpp").read_text(encoding="utf-8")
        self.assertIn("if (application::handleLedTestButton(held)) return;", navigation)
        self.assertIn("if (application::isLedTestActive())", navigation)

    def test_menu_idle_timeout_is_two_minutes_after_setup(self):
        configuration = (ROOT / "src/config/BoardConfig.h").read_text(encoding="utf-8")
        self.assertRegex(configuration, r"constexpr\s+uint16_t\s+MenuIdleTimeoutSeconds\s*=\s*120\s*;")
        application = (ROOT / "src/app/Application.cpp").read_text(encoding="utf-8")
        self.assertRegex(application, r"setupMenu\(\);[\s\S]*?"
                         r"renderer\.setResetIntervalTimeSeconds\(board::MenuIdleTimeoutSeconds\);\s*"
                         r"renderer\.menuAltered\(\);")
        # Selection disables reset, not just periodically renews its countdown.
        for start, end in (("void playGame()", "void selectPanelGame"),
                           ("void selectPanelGame", "void playRandom()")):
            selection = application.split(start, 1)[1].split(end, 1)[0]
            self.assertIn("renderer.turnOffResetLogic();", selection)
            self.assertRegex(selection, r"renderer\.turnOffResetLogic\(\);\s*renderer\.menuAltered\(\);")
        stop = application.split("void stopGames", 1)[1].split("void finishLedTest", 1)[0]
        self.assertRegex(stop, r"renderer\.setResetIntervalTimeSeconds\(board::MenuIdleTimeoutSeconds\);\s*renderer\.menuAltered\(\);")
        # This override belongs to application code, not a designer-owned file.
        menu = (ROOT / "src/Gyerek_jatek_arduinomega_menu.cpp").read_text(encoding="utf-8")
        self.assertNotIn("setResetIntervalTimeSeconds", menu)

    def test_project_owned_safe_renderer_is_bound(self):
        source = (ROOT / "src/Gyerek_jatek_arduinomega_menu.cpp").read_text(encoding="utf-8")
        header = (ROOT / "src/Gyerek_jatek_arduinomega_menu.h").read_text(encoding="utf-8")
        self.assertIn("SafeLiquidCrystalRenderer renderer(lcd, 16, 2);", source)
        self.assertIn('"ui/SafeLiquidCrystalRenderer.h"', header)
        self.assertIn("extern SafeLiquidCrystalRenderer renderer;", header)
        self.assertFalse((ROOT / "src/tcMenuLiquidCrystal.cpp").exists())
        self.assertFalse((ROOT / "src/Gyerek_jatek_arduinomega_main.cpp").exists())


if __name__ == "__main__":
    unittest.main()