"""Compile the real portable firmware modules and run hardware-free tests."""

import argparse
import importlib.util
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys
import unittest

import generate_config

ROOT = Path(__file__).resolve().parents[1]
SOURCES = [
    "src/audio/SoundEffects.cpp",
    "src/game/CycloneGame.cpp",
    "src/game/PanelGames.cpp",
    "src/game/PairedLampOutputs.cpp",
    "src/music/Melody.cpp",
    "src/music/MusicPlayer.cpp",
    "src/app/ModeController.cpp",
    "src/debug/InputMonitor.cpp",
    "src/debug/ButtonTester.cpp",
    "src/debug/DigitalOutputTester.cpp",
    "src/debug/LedStripTester.cpp",
    "src/debug/SystemChecks.cpp",
    "src/ui/JoystickRepeat.cpp",
    "src/ui/LcdText.cpp",
    "test/native/test_audio.cpp",
    "test/native/test_debug.cpp",
    "test/native/test_input.cpp",
    "test/native/test_led_strip.cpp",
    "test/native/test_panel_games.cpp",
    "test/native/test_main.cpp",
]


def find_compiler():
    if os.environ.get("CXX"):
        return shlex.split(os.environ["CXX"], posix=os.name != "nt")
    for name in ("g++", "clang++", "zig"):
        executable = shutil.which(name)
        if executable:
            return [executable, "c++"] if name == "zig" else [executable]

    specification = importlib.util.find_spec("ziglang")
    if specification and specification.origin:
        executable = Path(specification.origin).parent / ("zig.exe" if os.name == "nt" else "zig")
        if executable.is_file():
            return [str(executable), "c++"]

    # PlatformIO's Python environment is separate from the system Python used
    # by VS Code tasks. Discover its optional Zig installation without changing PATH.
    core = Path(os.environ.get("PLATFORMIO_CORE_DIR", str(Path.home() / ".platformio")))
    patterns = ("penv/Lib/site-packages/ziglang/zig.exe", "penv/lib/python*/site-packages/ziglang/zig")
    for pattern in patterns:
        for executable in core.glob(pattern):
            if executable.is_file():
                return [str(executable), "c++"]
    raise RuntimeError(
        "No native C++ compiler found. Install the optional packages listed in "
        "tools/requirements-test.txt, or put g++/clang++ on PATH."
    )


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sanitize", action="store_true", help="Enable undefined-behavior checks if supported by your compiler")
    arguments = parser.parse_args()

    generate_config.ensure_generated(ROOT)

    project_tests = unittest.defaultTestLoader.discover(str(ROOT / "test"), pattern="test_*.py")
    if not unittest.TextTestRunner(verbosity=2).run(project_tests).wasSuccessful():
        return 1

    try:
        compiler = find_compiler()
    except RuntimeError as error:
        print(error, file=sys.stderr)
        return 1

    build_directory = ROOT / ".pio/tests/native"
    build_directory.mkdir(parents=True, exist_ok=True)
    executable = build_directory / ("regression_tests.exe" if os.name == "nt" else "regression_tests")
    command = compiler + ["-std=c++11", "-Wall", "-Wextra", "-Werror", "-pedantic", "-O0", "-g", "-Isrc"]
    if arguments.sanitize:
        command += ["-fsanitize=undefined", "-fno-sanitize-recover=all"]
    command += SOURCES + ["-o", str(executable)]
    print("Compiling portable firmware regression tests...", flush=True)
    build = subprocess.run(command, cwd=ROOT, check=False)
    if build.returncode:
        return build.returncode
    return subprocess.run([str(executable)], cwd=ROOT, check=False).returncode


if __name__ == "__main__":
    raise SystemExit(main())