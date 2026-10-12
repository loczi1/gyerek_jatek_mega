"""Workbook-driven configuration regression checks."""

from pathlib import Path
import importlib.util
import unittest


ROOT = Path(__file__).resolve().parents[1]


def load_generator():
    spec = importlib.util.spec_from_file_location(
        "generate_config", ROOT / "tools/generate_config.py")
    module = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    spec.loader.exec_module(module)
    return module


class ExcelConfigTests(unittest.TestCase):
    def test_generated_headers_are_in_sync_with_workbook(self):
        generator = load_generator()
        workbook = ROOT / "Gyerek_jatek_3.0.xlsx"
        expected = generator.build_outputs(workbook)
        for relative_path, content in expected.items():
            path = ROOT / relative_path
            self.assertTrue(path.exists(), f"Missing generated file: {relative_path}")
            self.assertEqual(path.read_text(encoding="utf-8"), content)

    def test_game_controls_reference_exported_controls(self):
        generator = load_generator()
        workbook = ROOT / "Gyerek_jatek_3.0.xlsx"
        workbook_data = generator.load_workbook(workbook)
        pin_ids = {row.control_id for row in workbook_data.pins}
        self.assertTrue(pin_ids)
        for binding in workbook_data.bindings:
            self.assertIn(binding.control_id, pin_ids)
            self.assertNotEqual(binding.level_mask, 0)


if __name__ == "__main__":
    unittest.main()