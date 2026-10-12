"""PlatformIO preflight: refuse unsafe menu regeneration before compiling."""

from pathlib import Path
import runpy
from typing import Any
import unittest

import generate_config
from SCons.Script import Exit, Import


env: Any
Import("env")
project = Path(env.subst("$PROJECT_DIR"))
generate_config.ensure_generated(project)
namespace = runpy.run_path(str(project / "test/test_project.py"))
suite = unittest.defaultTestLoader.loadTestsFromTestCase(namespace["ProjectTests"])
if not unittest.TextTestRunner(verbosity=0).run(suite).wasSuccessful():
    Exit(1)