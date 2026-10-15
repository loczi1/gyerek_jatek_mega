"""Workbook-driven configuration regression checks."""

from dataclasses import replace
from pathlib import Path
import importlib.util
import sys
import unittest


ROOT = Path(__file__).resolve().parents[1]


def load_generator():
    spec = importlib.util.spec_from_file_location(
        "generate_config", ROOT / "tools/generate_config.py")
    module = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


class ExcelConfigTests(unittest.TestCase):
    @staticmethod
    def ikarus_fixture(generator):
        return generator.WorkbookData(
            pins=(
                generator.PinRow("BTN15", "", "39", "DigitalIn"),
                generator.PinRow("LED01", "", "4", "DigitalOut"),
                generator.PinRow("BUZ01", "", "5", "DigitalIn"),
                generator.PinRow("JOY01", "", "d53, x=a8, y=a9", "AnalogIn"),
                generator.PinRow("LCD01", "address: 0x27", "i2c (20,21)", "DigitalIn"),
                generator.PinRow("BTN16_ikarus_1", "", "52", "DigitalIn"),
                generator.PinRow("DIO01_ikarus_1", "", "2", "DigitalOut"),
            ),
            bindings=(generator.BindingRow("CYCLON", "1-4", 15, "Start", "BTN15"),),
        )

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

    def test_gpio_lists_preserve_switch_contacts_not_position_count(self):
        generator = load_generator()
        for text, expected in (
            ("25,24 (3 állású)", (25, 24, 255)),
            ("D25, d24 (3 positions)", (25, 24, 255)),
            ("6,7,8", (6, 7, 8)),
            ("d53, x=a8, y=a9", (53, 8, 9)),
            ("i2c (20,21)", (20, 21, 255)),
        ):
            with self.subTest(text=text):
                self.assertEqual(generator.extract_pin_triplet(text), expected)
        with self.assertRaisesRegex(ValueError, "At most three GPIO"):
            generator.extract_pin_triplet("6,7,8,9")

    def test_ikarus_pairs_follow_suffix_not_row_order(self):
        generator = load_generator()
        data = self.ikarus_fixture(generator)
        pins = data.pins + (
            generator.PinRow("DIO02_ikarus_2", "", "16", "DigitalOut"),
            generator.PinRow("BTN17_ikarus_2", "", "50", "DigitalIn"),
        )
        switches = generator.derive_ikarus_switches(tuple(reversed(pins)))
        self.assertEqual(
            [(row.number, row.input_control_id, row.led_control_id) for row in switches],
            [(1, "BTN16_ikarus_1", "DIO01_ikarus_1"),
             (2, "BTN17_ikarus_2", "DIO02_ikarus_2")],
        )
        header = generator.render_pins_header(replace(data, pins=pins))
        self.assertIn("IlluminatedSwitchDefinition{1, ControlId::Btn16Ikarus1, ControlId::Dio01Ikarus1}", header)
        self.assertIn("constexpr uint8_t IkarusSwitchCount = 2;", header)

    def test_ikarus_pairs_require_unique_correctly_typed_controls(self):
        generator = load_generator()
        data = self.ikarus_fixture(generator)
        invalid_pins = (
            data.pins[:-1],
            data.pins[:-2] + data.pins[-1:],
            data.pins[:-1] + (replace(data.pins[-1], pin_type="DigitalIn"),),
            data.pins + (generator.PinRow("BTN99_ikarus_1", "", "50", "DigitalIn"),),
            data.pins + (generator.PinRow("DIO99_ikarus_1", "", "16", "DigitalOut"),),
            data.pins[:-1] + (replace(data.pins[-1], control_id="DIO01_ikarus_0"),),
            data.pins[:-1] + (replace(data.pins[-1], control_id="DIO01_ikarus_256"),),
        )
        for pins in invalid_pins:
            with self.subTest(pins=pins), self.assertRaises(ValueError):
                generator.derive_ikarus_switches(pins)

    def test_ikarus_leds_cannot_use_reserved_pins_or_input_contacts(self):
        generator = load_generator()
        data = self.ikarus_fixture(generator)
        generator.validate(data.pins, data.bindings)
        for raw_pin, reason in (
            ("0", "reserved"), ("1", "reserved"), ("4", "reserved"),
            ("5", "reserved"), ("13", "reserved"), ("20", "reserved"),
            ("21", "reserved"), ("39", "reserved"), ("53", "reserved"),
            ("52", "digital input"), ("54", "one Mega digital pin"),
            ("", "one Mega digital pin"), ("2,3", "one Mega digital pin"),
        ):
            pins = data.pins[:-1] + (replace(data.pins[-1], raw_pin=raw_pin),)
            with self.subTest(pin=raw_pin), self.assertRaisesRegex(ValueError, reason):
                generator.validate(pins, data.bindings)
        duplicate_led = data.pins + (
            generator.PinRow("BTN17_ikarus_2", "", "50", "DigitalIn"),
            generator.PinRow("DIO02_ikarus_2", "", "2", "DigitalOut"),
        )
        with self.assertRaisesRegex(ValueError, "another Ikarus LED"):
            generator.validate(duplicate_led, data.bindings)

    def test_header_without_ikarus_avoids_an_empty_cpp_array(self):
        generator = load_generator()
        data = self.ikarus_fixture(generator)
        header = generator.render_pins_header(replace(data, pins=data.pins[:-2]))
        self.assertIn("constexpr uint8_t IkarusSwitchCount = 0;", header)
        self.assertNotIn("IkarusSwitches[]", header)

    def test_workbook_exports_six_separate_ikarus_input_led_pairs(self):
        generator = load_generator()
        data = generator.load_workbook(ROOT / generator.WORKBOOK_NAME)
        switches = generator.derive_ikarus_switches(data.pins)
        self.assertEqual([row.number for row in switches], list(range(1, 7)))
        by_id = {row.control_id: row for row in data.pins}
        for switch in switches:
            input_pin = by_id[switch.input_control_id]
            led_pin = by_id[switch.led_control_id]
            self.assertEqual(generator.pin_type_name(input_pin), "DigitalIn")
            self.assertEqual(generator.pin_type_name(led_pin), "DigitalOut")
            self.assertNotEqual(generator.extract_pin_triplet(input_pin.raw_pin)[0],
                                generator.extract_pin_triplet(led_pin.raw_pin)[0])

    def test_board_constants_follow_cyclon_binding_and_pot_adc_channel(self):
        generator = load_generator()
        data = generator.load_workbook(ROOT / generator.WORKBOOK_NAME)
        constants = generator.derive_board_constants(data)
        by_id = {row.control_id: row for row in data.pins}
        binding = next(row for row in data.bindings
                       if row.game_id == "CYCLON" and row.role.upper() in {"START", "ACTION"})
        self.assertEqual(constants["GameButtonControlId"], binding.control_id)
        self.assertEqual(constants["GameButtonPin"],
                         generator.extract_pin_triplet(by_id[binding.control_id].raw_pin)[0])
        pot = by_id["POT1"]
        self.assertEqual(generator.pin_type_name(pot), "AnalogIn")
        self.assertLess(generator.extract_pin_triplet(pot.raw_pin)[0], 16)

    def test_active_levels_and_all_workbook_lamp_pairs(self):
        generator = load_generator()
        data = generator.load_workbook(ROOT / generator.WORKBOOK_NAME)
        pairs = dict(generator.derive_lamp_pairs(data.pins))
        self.assertEqual(len(pairs), 22)
        self.assertEqual(pairs["BTN01-left-blue_1"], "LED01-left-blue_1")
        self.assertEqual(pairs["BTN07-down-5_1"], "LED03-strip-down5_1")
        self.assertEqual(pairs["SW01-metal_1"], "LED05-metal_1")
        self.assertEqual(pairs["BTN16-ikarus_1"], "DIO01-ikarus_1")
        self.assertEqual(dict(generator.derive_lamp_pairs(tuple(reversed(data.pins)))), pairs)
        outputs = [pin for pin in data.pins if pin.control_id.startswith("DIO")]
        self.assertEqual(len(outputs), 6)
        self.assertTrue(all(pin.active_level == "HIGH" for pin in outputs))
        self.assertTrue(all(pin.active_level == "WS2812B" for pin in data.pins
                            if pin.control_id.startswith("LED")))

    def test_active_level_parsing_validation_and_strip_discrimination(self):
        generator = load_generator()
        parsed = generator.parse_pins([dict(ID="DIO01", Info="", ArduinoPin="6",
                                            Type="DigitalOut", ActiveLevel=" low ")])
        self.assertEqual(parsed[0].active_level, "LOW")
        data = self.ikarus_fixture(generator)
        for level in ("LOW", "HIGH", ""):
            pins = data.pins[:-1] + (replace(data.pins[-1], active_level=level),)
            generator.validate(pins, data.bindings)
            header = generator.render_pins_header(replace(data, pins=pins))
            self.assertIn("ActiveLevel::" + {"LOW": "Low", "HIGH": "High", "": "Unspecified"}[level], header)
        for pin in (replace(data.pins[-1], active_level="HGIH"),
                    replace(data.pins[0], active_level="HIGH")):
            pins = tuple(pin if old.control_id == pin.control_id else old for old in data.pins)
            with self.assertRaisesRegex(ValueError, "ActiveLevel"):
                generator.validate(pins, data.bindings)
        pins = data.pins + (generator.PinRow("LED99", "", "6", "DigitalOut", "LOW"),)
        generator.validate(pins, data.bindings)
        self.assertEqual(generator.derive_board_constants(replace(data, pins=pins))["LedDataPin"], 4)
        for number in (0, 4, 5, 13, 20, 39, 52, 53):
            pins = data.pins + (generator.PinRow("DIO99", "", str(number), "DigitalOut", "HIGH"),)
            with self.subTest(pin=number), self.assertRaisesRegex(ValueError, "reserved/shared"):
                generator.validate(pins, data.bindings)

    def test_strip_pixels_preserve_exact_workbook_positions_and_mirrored_order(self):
        generator = load_generator()
        data = generator.load_workbook(ROOT / generator.WORKBOOK_NAME)
        expected = {
            **{f"LED01-left-blue_{i}": 83 + i for i in range(1, 4)},
            **{f"LED03-strip-down5_{i}": 70 + i for i in range(1, 6)},
            **{f"LED04-right-color_{i}": 84 - i for i in range(1, 4)},
            **{f"LED05-metal_{i}": 75 + i for i in range(1, 6)},
        }
        actual = {pin.control_id: generator.parse_strip_pixel(pin) for pin in data.pins
                  if generator.parse_strip_pixel(pin)}
        self.assertEqual(actual, expected)
        self.assertEqual(len(actual), 16)
        header = generator.render_pins_header(data)
        self.assertIn("uint8_t stripPixel;", header)
        for pin in data.pins:
            pixel = expected.get(pin.control_id, 0)
            row = next(line for line in header.splitlines()
                       if f"// {pin.control_id}:" in line)
            self.assertIn(f", {pixel}}}, //", row)
        self.assertEqual(
            {pin.control_id: generator.parse_strip_pixel(pin) for pin in reversed(data.pins)
             if generator.parse_strip_pixel(pin)}, expected)

    def test_strip_pixel_parsing_uses_only_explicit_ws2812b_integers(self):
        generator = load_generator()
        pin = generator.PinRow("LED99", "", "4", "DigitalOut", "WS2812B")
        for info, expected in (("1", 1), ("100", 100), (" 84 ", 84), ("084", 84),
                               ("", 0), ("position 1-49 (50-70)", 0),
                               ("1-49", 0), ("pixel 84", 0), ("84/85", 0)):
            with self.subTest(info=info):
                self.assertEqual(generator.parse_strip_pixel(replace(pin, info=info)), expected)
        for info in ("0", "101", "-1", "256", "999999999999999999999"):
            with self.subTest(info=info), self.assertRaisesRegex(ValueError, "1..100"):
                generator.parse_strip_pixel(replace(pin, info=info))
        for active_level in ("", "HIGH", "LOW"):
            for info in ("84", "0", "101", "-1"):
                with self.subTest(active_level=active_level, info=info):
                    self.assertEqual(generator.parse_strip_pixel(
                        replace(pin, info=info, active_level=active_level)), 0)

    def test_strip_pixel_validation_rejects_out_of_range_and_duplicate_lamps(self):
        generator = load_generator()
        data = self.ikarus_fixture(generator)
        lamp = generator.PinRow("LED98", "84", "4", "DigitalOut", "WS2812B")
        generator.validate(data.pins + (lamp,), data.bindings)
        for info in ("0", "101", "-1"):
            with self.subTest(info=info), self.assertRaisesRegex(ValueError, "1..100"):
                generator.validate(data.pins + (replace(lamp, info=info),), data.bindings)
        duplicate = replace(lamp, control_id="LED99", info="084")
        for lamps in ((lamp, duplicate), (duplicate, lamp)):
            with self.subTest(lamps=lamps), self.assertRaisesRegex(
                    ValueError, "Duplicate WS2812B strip pixel 84"):
                generator.validate(data.pins + lamps, data.bindings)
        generator.validate(data.pins + (lamp, replace(duplicate, info="85")), data.bindings)
        generator.validate(data.pins + (lamp, replace(duplicate, info="position 84")), data.bindings)
        generator.validate(data.pins + (lamp, replace(duplicate, raw_pin="6", active_level="HIGH")),
                           data.bindings)

    def test_lamp_pairs_reject_missing_duplicate_and_wrong_type(self):
        generator = load_generator()
        pins = (generator.PinRow("BTN01-left-blue_1", "", "33", "DigitalIn"),
                generator.PinRow("LED01-left-blue_1", "84", "4", "DigitalOut", "WS2812B"))
        invalid = (pins[:1], pins[1:],
                   pins + (replace(pins[0], control_id="SW99-left-blue_1"),),
                   (replace(pins[0], pin_type="AnalogIn"), pins[1]))
        for rows in invalid:
            with self.subTest(rows=rows), self.assertRaises(ValueError):
                generator.derive_lamp_pairs(rows)


if __name__ == "__main__":
    unittest.main()