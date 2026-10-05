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
        self.assertEqual(environment["extra_scripts"], "pre:tools/check_build.py")
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
        self.assertIn("menuNewRuntimeList(&minfoNewRuntimeList, 7,", source)

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