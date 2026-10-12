"""Generate flash-friendly configuration headers from the workbook."""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import argparse
import re
import sys
import textwrap
import xml.etree.ElementTree as ET
import zipfile


WORKBOOK_NAME = "Gyerek_jatek_3.0.xlsx"
GENERATED_ROOT = Path("src/generated")
INVALID_PIN = 0xFF
CELL_NS = {"main": "http://schemas.openxmlformats.org/spreadsheetml/2006/main",
           "pkg": "http://schemas.openxmlformats.org/package/2006/relationships"}


@dataclass(frozen=True)
class PinRow:
    control_id: str
    info: str
    raw_pin: str
    pin_type: str


@dataclass(frozen=True)
class BindingRow:
    game_id: str
    level_text: str
    level_mask: int
    role: str
    control_id: str


@dataclass(frozen=True)
class WorkbookData:
    pins: tuple[PinRow, ...]
    bindings: tuple[BindingRow, ...]


def column_index(reference: str) -> int:
    match = re.match(r"([A-Z]+)", reference)
    if not match:
        raise ValueError(f"Invalid cell reference: {reference}")
    value = 0
    for character in match.group(1):
        value = value * 26 + ord(character) - ord("A") + 1
    return value - 1


def sheet_rows(archive: zipfile.ZipFile, workbook_root: ET.Element,
               relationship_map: dict[str, str], shared_strings: list[str],
               sheet_name: str) -> list[dict[str, str]]:
    sheets = workbook_root.find("main:sheets", CELL_NS)
    if sheets is None:
        raise ValueError("Workbook is missing the sheets collection")
    for sheet in sheets:
        if sheet.attrib.get("name") != sheet_name:
            continue
        relation_id = sheet.attrib[
            "{http://schemas.openxmlformats.org/officeDocument/2006/relationships}id"]
        target = "xl/" + relationship_map[relation_id]
        root = ET.fromstring(archive.read(target))
        rows = []
        headers = None
        for row in root.findall(".//main:sheetData/main:row", CELL_NS):
            cells: dict[int, str] = {}
            for cell in row.findall("main:c", CELL_NS):
                reference = cell.attrib.get("r", "")
                index = column_index(reference)
                value = read_cell(cell, shared_strings)
                cells[index] = value.strip()
            if not cells:
                continue
            width = max(cells) + 1
            values = [cells.get(index, "") for index in range(width)]
            if headers is None:
                headers = values
                continue
            if not any(values):
                continue
            row_map = {}
            for index, header in enumerate(headers):
                if not header:
                    continue
                row_map[header] = values[index] if index < len(values) else ""
            rows.append(row_map)
        return rows
    raise ValueError(f"Missing worksheet: {sheet_name}")


def read_cell(cell: ET.Element, shared_strings: list[str]) -> str:
    inline = cell.find("main:is", CELL_NS)
    if inline is not None:
        return "".join(text_node.text or "" for text_node in inline.iterfind(".//main:t", CELL_NS))
    value = cell.find("main:v", CELL_NS)
    if value is None:
        return ""
    raw = value.text or ""
    if cell.attrib.get("t") == "s":
        return shared_strings[int(raw)]
    return raw


def load_workbook(workbook_path: Path) -> WorkbookData:
    with zipfile.ZipFile(workbook_path) as archive:
        shared_strings = []
        if "xl/sharedStrings.xml" in archive.namelist():
            shared_root = ET.fromstring(archive.read("xl/sharedStrings.xml"))
            for item in shared_root.findall("main:si", CELL_NS):
                shared_strings.append(
                    "".join(text_node.text or "" for text_node in item.iterfind(".//main:t", CELL_NS)))

        workbook_root = ET.fromstring(archive.read("xl/workbook.xml"))
        relationships_root = ET.fromstring(archive.read("xl/_rels/workbook.xml.rels"))
        relationship_map = {
            relation.attrib["Id"]: relation.attrib["Target"]
            for relation in relationships_root.findall("pkg:Relationship", CELL_NS)
        }

        pin_rows = tuple(parse_pins(sheet_rows(
            archive, workbook_root, relationship_map, shared_strings, "PinMap_export")))
        binding_rows = tuple(parse_bindings(sheet_rows(
            archive, workbook_root, relationship_map, shared_strings, "GameControls")))
    validate(pin_rows, binding_rows)
    return WorkbookData(pin_rows, binding_rows)


def parse_pins(rows: list[dict[str, str]]) -> list[PinRow]:
    required = ("ID", "Info", "ArduinoPin", "Type")
    for column in required:
        if rows and column not in rows[0]:
            raise ValueError(f"PinMap_export is missing required column: {column}")
    parsed = []
    for row in rows:
        control_id = row.get("ID", "").strip()
        if not control_id:
            continue
        parsed.append(PinRow(
            control_id=control_id,
            info=row.get("Info", "").strip(),
            raw_pin=row.get("ArduinoPin", "").strip(),
            pin_type=row.get("Type", "").strip() or "Unknown",
        ))
    return parsed


def parse_bindings(rows: list[dict[str, str]]) -> list[BindingRow]:
    if not rows:
        return []
    required = ("GameId", "Level", "Action", "ControlId")
    for column in required:
        if column not in rows[0]:
            raise ValueError(f"GameControls is missing required column: {column}")
    parsed = []
    for row in rows:
        game_id = row.get("GameId", "").strip()
        level_text = row.get("Level", "").strip()
        role = row.get("Action", "").strip()
        control_id = row.get("ControlId", "").strip()
        if not any((game_id, level_text, role, control_id)):
            continue
        if not all((game_id, level_text, role, control_id)):
            raise ValueError(f"Incomplete GameControls row: {row}")
        parsed.append(BindingRow(
            game_id=game_id,
            level_text=level_text,
            level_mask=parse_level_mask(level_text),
            role=role,
            control_id=control_id,
        ))
    return parsed


def validate(pins: tuple[PinRow, ...], bindings: tuple[BindingRow, ...]) -> None:
    if not pins:
        raise ValueError("PinMap_export must contain at least one control")
    pin_ids = set()
    for pin in pins:
        if pin.control_id in pin_ids:
            raise ValueError(f"Duplicate control ID in PinMap_export: {pin.control_id}")
        pin_ids.add(pin.control_id)
    if not bindings:
        raise ValueError("GameControls must contain at least one binding")
    for binding in bindings:
        if binding.control_id not in pin_ids:
            raise ValueError(
                f"GameControls references unknown control '{binding.control_id}' for {binding.game_id}")


def parse_level_mask(text: str) -> int:
    stripped = text.replace(" ", "")
    if stripped in ("*", "ALL", "all"):
        return 0xFFFFFFFF
    mask = 0
    for part in stripped.split(","):
        if not part:
            continue
        if "-" in part:
            start_text, end_text = part.split("-", 1)
            start = int(start_text)
            end = int(end_text)
            if start < 1 or end < start or end > 32:
                raise ValueError(f"Invalid level range: {text}")
            for level in range(start, end + 1):
                mask |= 1 << (level - 1)
        else:
            level = int(part)
            if level < 1 or level > 32:
                raise ValueError(f"Invalid level value: {text}")
            mask |= 1 << (level - 1)
    if mask == 0:
        raise ValueError(f"Empty level mask: {text}")
    return mask


def pin_type_name(pin: PinRow) -> str:
    normalized = normalize_upper_identifier(pin.pin_type)
    if pin.control_id.startswith("JOY") or pin.control_id.startswith("LCD"):
        return "Composite"
    if normalized == "DIGITALIN":
        return "DigitalIn"
    if normalized == "DIGITALOUT":
        return "DigitalOut"
    if normalized == "ANALOGIN":
        return "AnalogIn"
    return "Unknown"


def extract_pin_triplet(raw_pin: str) -> tuple[int, int, int]:
    digital_match = re.search(r"\bd\s*(\d+)\b", raw_pin, flags=re.IGNORECASE)
    x_match = re.search(r"\bx\s*=\s*a\s*(\d+)\b", raw_pin, flags=re.IGNORECASE)
    y_match = re.search(r"\by\s*=\s*a\s*(\d+)\b", raw_pin, flags=re.IGNORECASE)
    i2c_match = re.search(r"i2c\s*\((\d+)\s*,\s*(\d+)\)", raw_pin, flags=re.IGNORECASE)
    if digital_match or x_match or y_match:
        return (
            int(digital_match.group(1)) if digital_match else INVALID_PIN,
            int(x_match.group(1)) if x_match else INVALID_PIN,
            int(y_match.group(1)) if y_match else INVALID_PIN,
        )
    if i2c_match:
        return int(i2c_match.group(1)), int(i2c_match.group(2)), INVALID_PIN
    number_match = re.search(r"\d+", raw_pin)
    if number_match:
        return int(number_match.group(0)), INVALID_PIN, INVALID_PIN
    return INVALID_PIN, INVALID_PIN, INVALID_PIN


def derive_board_constants(data: WorkbookData) -> dict[str, int | str]:
    pin_by_id = {row.control_id: row for row in data.pins}
    led_pins = {extract_pin_triplet(row.raw_pin)[0] for row in data.pins if row.control_id.startswith("LED")}
    buzzer = pin_by_id.get("BUZ01")
    joystick = pin_by_id.get("JOY01")
    lcd = pin_by_id.get("LCD01")
    constants: dict[str, int | str] = {}
    if len(led_pins) == 1:
        constants["LedDataPin"] = led_pins.pop()
    if buzzer is not None:
        constants["BuzzerPin"] = extract_pin_triplet(buzzer.raw_pin)[0]
    if joystick is not None:
        menu_button, joy_x, joy_y = extract_pin_triplet(joystick.raw_pin)
        constants["MenuButtonPin"] = menu_button
        constants["MenuJoystickXChannel"] = joy_x
        constants["MenuJoystickYChannel"] = joy_y
    if lcd is not None:
        sda, scl, _ = extract_pin_triplet(lcd.raw_pin)
        constants["LcdI2cSdaPin"] = sda
        constants["LcdI2cSclPin"] = scl
        address = parse_i2c_address(lcd.info)
        if address is not None:
            constants["LcdI2cAddress"] = address
    for binding in data.bindings:
        if binding.game_id == "CYCLON" and binding.role.upper() in {"ACTION", "START"}:
            constants["GameButtonControlId"] = binding.control_id
            constants["GameButtonPin"] = extract_pin_triplet(pin_by_id[binding.control_id].raw_pin)[0]
            break
    return constants


def parse_i2c_address(text: str) -> int | None:
    match = re.search(r"0x([0-9a-fA-F]+)", text)
    return int(match.group(1), 16) if match else None


def normalize_upper_identifier(value: str) -> str:
    return re.sub(r"[^A-Z0-9]+", "", value.upper())


def enum_name(value: str) -> str:
    parts = [part for part in re.split(r"[^A-Za-z0-9]+", value) if part]
    if not parts:
        raise ValueError(f"Cannot create C++ identifier from '{value}'")
    return "".join(part[:1].upper() + part[1:].lower() for part in parts)


def render_pins_header(data: WorkbookData) -> str:
    board_constants = derive_board_constants(data)
    control_names = [enum_name(row.control_id) for row in data.pins]
    pin_rows = []
    for row in data.pins:
        primary, secondary, tertiary = extract_pin_triplet(row.raw_pin)
        pin_rows.append(
            f"    PinDefinition{{{primary}, {secondary}, {tertiary}, PinType::{pin_type_name(row)}}}, // {row.control_id}: {row.raw_pin or '-'}")

    constants = []
    numeric_keys = (
        "LedDataPin",
        "BuzzerPin",
        "MenuButtonPin",
        "MenuJoystickXChannel",
        "MenuJoystickYChannel",
        "LcdI2cSdaPin",
        "LcdI2cSclPin",
        "LcdI2cAddress",
        "GameButtonPin",
    )
    for key in numeric_keys:
        if key in board_constants:
            value = board_constants[key]
            if key == "LcdI2cAddress":
                constants.append(f"constexpr uint8_t {key} = 0x{int(value):02X};")
            else:
                constants.append(f"constexpr uint8_t {key} = {int(value)};")
    if "GameButtonControlId" in board_constants:
        constants.append(
            f"constexpr ControlId GameButtonControlId = ControlId::{enum_name(str(board_constants['GameButtonControlId']))};")

    return render_header(
        guard="GYEREK_JATEK_GENERATED_PINS_H",
        body=f"""
namespace generated {{
namespace pins {{

constexpr uint8_t InvalidPin = {INVALID_PIN};

enum class ControlId : uint8_t {{
{indent_lines([f'{name},' for name in control_names], 1)}
    Count
}};

enum class PinType : uint8_t {{
    DigitalIn,
    DigitalOut,
    AnalogIn,
    Composite,
    Unknown
}};

struct PinDefinition {{
    uint8_t primaryPin;
    uint8_t secondaryPin;
    uint8_t tertiaryPin;
    PinType type;
}};

constexpr PinDefinition PinTable[] = {{
{chr(10).join(pin_rows)}
}};

constexpr PinDefinition definition(ControlId id) {{
    return PinTable[static_cast<uint8_t>(id)];
}}

constexpr uint8_t pinNumber(ControlId id) {{
    return definition(id).primaryPin;
}}

{chr(10).join(constants)}

}} // namespace pins
}} // namespace generated
""",
    )


def render_game_config_header(data: WorkbookData) -> str:
    game_ids = unique_preserving_order(binding.game_id for binding in data.bindings)
    role_names = unique_preserving_order(binding.role for binding in data.bindings)
    control_names = {row.control_id: enum_name(row.control_id) for row in data.pins}
    binding_lines = []
    game_offsets: dict[str, tuple[int, int]] = {}
    index = 0
    for game_id in game_ids:
        game_bindings = [binding for binding in data.bindings if binding.game_id == game_id]
        game_offsets[game_id] = (index, len(game_bindings))
        for binding in game_bindings:
            binding_lines.append(
                "    ControlBinding{{GameId::{game}, 0x{mask:08X}UL, pins::ControlId::{control}, ControlRole::{role}}}, "
                "// {level_text}".format(
                    game=enum_name(binding.game_id),
                    mask=binding.level_mask,
                    control=control_names[binding.control_id],
                    role=enum_name(binding.role),
                    level_text=binding.level_text,
                ))
            index += 1
    definition_lines = []
    for game_id in game_ids:
        offset, count = game_offsets[game_id]
        definition_lines.append(
            f"    GameDefinition{{GameId::{enum_name(game_id)}, {offset}, {count}, 0, 0}},")

    return render_header(
        guard="GYEREK_JATEK_GENERATED_GAME_CONFIG_H",
        body=f"""
namespace generated {{
namespace games {{

enum class GameId : uint8_t {{
{indent_lines([f'{enum_name(game_id)},' for game_id in game_ids], 1)}
    Count
}};

enum class ControlRole : uint8_t {{
{indent_lines([f'{enum_name(role)},' for role in role_names], 1)}
    Count
}};

struct ControlBinding {{
    GameId game;
    uint32_t levelMask;
    pins::ControlId control;
    ControlRole role;
}};

struct LevelConfig {{
    GameId game;
    uint8_t level;
    uint16_t moveIntervalMs;
    uint8_t hitRadius;
    uint8_t targetMin;
    uint8_t targetMax;
    uint16_t introPauseMs;
    uint16_t introGameStepMs;
    uint16_t introScoreStepMs;
    uint16_t feedbackPhaseMs;
    uint8_t feedbackPhaseCount;
}};

struct GameDefinition {{
    GameId game;
    uint8_t bindingOffset;
    uint8_t bindingCount;
    uint8_t levelOffset;
    uint8_t levelCount;
}};

constexpr ControlBinding ControlBindings[] = {{
{chr(10).join(binding_lines)}
}};

constexpr LevelConfig LevelConfigs[] = {{
}};

constexpr GameDefinition GameDefinitions[] = {{
{chr(10).join(definition_lines)}
}};

constexpr uint32_t levelBit(uint8_t level) {{
    return level >= 1 && level <= 32 ? (1UL << (level - 1)) : 0UL;
}}

constexpr bool appliesToLevel(const ControlBinding& binding, uint8_t level) {{
    const uint32_t bit = levelBit(level);
    return bit != 0 && (binding.levelMask & bit) != 0;
}}

}} // namespace games
}} // namespace generated
""",
        includes="#include <stdint.h>\n#include \"generated/pins.h\"",
    )


def render_header(*, guard: str, body: str, includes: str = "#include <stdint.h>") -> str:
    return textwrap.dedent(f"""\
    #ifndef {guard}
    #define {guard}

    // Generated by tools/generate_config.py from {WORKBOOK_NAME}. Do not edit.
    {includes}

    {body.strip()}

    #endif
    """)


def indent_lines(lines: list[str], level: int) -> str:
    prefix = "    " * level
    return "\n".join(prefix + line for line in lines)


def unique_preserving_order(values):
    result = []
    seen = set()
    for value in values:
        if value in seen:
            continue
        seen.add(value)
        result.append(value)
    return result


def build_outputs(workbook_path: Path) -> dict[Path, str]:
    data = load_workbook(workbook_path)
    return {
        GENERATED_ROOT / "pins.h": render_pins_header(data),
        GENERATED_ROOT / "game_config.h": render_game_config_header(data),
    }


def ensure_generated(project_dir: Path, workbook_name: str = WORKBOOK_NAME) -> dict[Path, str]:
    workbook_path = project_dir / workbook_name
    outputs = build_outputs(workbook_path)
    for relative_path, content in outputs.items():
        path = project_dir / relative_path
        path.parent.mkdir(parents=True, exist_ok=True)
        existing = path.read_text(encoding="utf-8") if path.exists() else None
        if existing != content:
            path.write_text(content, encoding="utf-8", newline="\n")
    return outputs


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--project-dir", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--workbook", default=WORKBOOK_NAME)
    parser.add_argument("--check", action="store_true", help="Fail if generated files are out of date")
    arguments = parser.parse_args(argv)

    project_dir = arguments.project_dir.resolve()
    workbook_path = project_dir / arguments.workbook
    outputs = build_outputs(workbook_path)
    failed = False
    for relative_path, content in outputs.items():
        path = project_dir / relative_path
        if arguments.check:
            if not path.exists() or path.read_text(encoding="utf-8") != content:
                print(f"Out of date: {relative_path}", file=sys.stderr)
                failed = True
        else:
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(content, encoding="utf-8", newline="\n")
            print(f"Generated {relative_path}")
    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())


if "Import" in globals():
    from typing import Any
    from SCons.Script import Import

    env: Any
    Import("env")
    ensure_generated(Path(env.subst("$PROJECT_DIR")))