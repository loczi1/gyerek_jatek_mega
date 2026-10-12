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

    def test_exactly_three_root_options_and_no_music_selection(self):
        definition = json.loads((ROOT / "Gyerek_jatek_arduinomega.emf").read_text(encoding="utf-8"))
        roots = [entry for entry in definition["items"] if entry["parentId"] == 0]
        self.assertEqual([entry["item"]["name"] for entry in roots], ["Jatek", "Ujrainditas", "Debug"])
        self.assertEqual([entry["type"] for entry in roots], ["actionMenu", "actionMenu", "subMenu"])
        source = (ROOT / "src/Gyerek_jatek_arduinomega_menu.cpp").read_text(encoding="utf-8")
        self.assertIn("menuJatek(&minfoJatek, &menuRestart,", source)
        self.assertIn("menuRestart(&minfoRestart, &menuDebug,", source)
        self.assertIn("menuDebug(&minfoDebug, &menuBackDebug, nullptr,", source)
        self.assertIn("menuMgr.initWithoutInput(&renderer, &menuJatek);", source)
        for unwanted in ("JatekValaszto", "Input naplo", "Bemenetek", "Zene", "enumStrJatek"):
            self.assertNotIn(unwanted, source)
        application = (ROOT / "src/app/Application.cpp").read_text(encoding="utf-8")
        controller = (ROOT / "src/app/ModeController.h").read_text(encoding="utf-8")
        self.assertNotIn("music::", application)
        self.assertNotIn("music/MusicPlayer.h", controller)
        self.assertIn("isShowingRoot()", application)

    def test_debug_lists_cover_inputs_with_non_overlapping_ids(self):
        definition = json.loads((ROOT / "Gyerek_jatek_arduinomega.emf").read_text(encoding="utf-8"))
        children = [entry["item"] for entry in definition["items"] if entry["parentId"] == 7]
        self.assertEqual([item["name"] for item in children], ["Gombok", "LED szalag", "Digitalis", "Analog", "Allapot", "Hibak"])
        self.assertTrue(all(item["readOnly"] for item in children if item["name"] != "LED szalag"))
        self.assertEqual([item.get("initialRows") for item in children], [65, None, 54, 16, None, 3])
        occupied = set()
        for entry in definition["items"]:
            item = entry["item"]
            count = item.get("initialRows", 0) + 1
            identifiers = set(range(item["id"], item["id"] + count))
            self.assertTrue(occupied.isdisjoint(identifiers))
            occupied.update(identifiers)

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

    def test_analog_button_gpio_and_joystick_exclusion(self):
        tester = (ROOT / "src/debug/ButtonTester.cpp").read_text(encoding="utf-8")
        self.assertIn("pin != board::FirstAnalogGpioPin + board::MenuJoystickChannel", tester)
        self.assertIn("InputMonitor::formatAnalogName", tester)
        hardware = (ROOT / "src/hardware/Diagnostics.cpp").read_text(encoding="utf-8")
        self.assertIn("if (!debug::canTestButton(pin)) return false;", hardware)
        self.assertIn("return (*portInputRegister(port) & mask) == 0;", hardware)
        header = (ROOT / "src/debug/ButtonTester.h").read_text(encoding="utf-8")
        self.assertIn("changedAt_[board::GpioPinCount]", header)

    def test_led_strip_menu_count_and_single_d2_controller(self):
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