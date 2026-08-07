"""E5-S1 — CI and local builds provision Qt 6.8 LTS.

The story is the provisioning flip, but the `Qt5::` -> `Qt6::` rename cannot be
separated from it: a tree that installs Qt 6.8 and still calls
`find_package(Qt5 ...)` does not configure at all. These cases therefore pin
both halves, plus the two Windows traps the E2 work left behind — the MSVC Qt
ABI and the Visual Studio generator.
"""

import re
from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]
WORKFLOW = ROOT / ".github" / "workflows" / "build.yml"

REQUIRED_QT_MAJOR_MINOR = "6.8"
PINNED_CI_QT_VERSION = "6.8.3"
WINDOWS_QT_ARCH = "win64_msvc2022_64"


def build_files() -> list[Path]:
    """Every CMake file this repository owns, excluding vendored trees."""
    owned = []
    for path in ROOT.rglob("CMakeLists.txt"):
        relative = path.relative_to(ROOT)
        if relative.parts[0] in {"external", "out", "bin", "lib"}:
            continue
        owned.append(path)
    return owned


class Qt6ProvisioningTests(unittest.TestCase):
    def test_cmake_requires_qt_6_8(self) -> None:
        top_level = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
        self.assertRegex(
            top_level,
            r"find_package\s*\(\s*Qt6\s+6\.8\s+REQUIRED\s+COMPONENTS\s+Core\s+Gui\s+Widgets\s*\)",
        )

    def test_no_build_file_still_names_qt5(self) -> None:
        offenders = []
        for path in build_files():
            text = path.read_text(encoding="utf-8")
            if re.search(r"\bQt5\b", text):
                offenders.append(str(path.relative_to(ROOT)))
        self.assertEqual([], offenders)

    def test_every_qt_link_target_is_qt6_core_gui_or_widgets(self) -> None:
        allowed = {"Qt6::Core", "Qt6::Gui", "Qt6::Widgets"}
        found = set()
        for path in build_files():
            found.update(
                re.findall(r"Qt\d::\w+", path.read_text(encoding="utf-8"))
            )
        self.assertTrue(found, "no Qt link targets found at all")
        self.assertEqual(set(), found - allowed)

    def test_ci_provisions_qt_6_8_on_all_three_platforms(self) -> None:
        workflow = WORKFLOW.read_text(encoding="utf-8")

        actions = re.findall(r"uses:\s*jurplel/install-qt-action@\S+", workflow)
        self.assertEqual(
            3,
            len(actions),
            "each of the Linux, macOS, and Windows jobs must provision Qt",
        )

        versions = re.findall(r"^\s*version:\s*'([^']+)'", workflow, re.MULTILINE)
        self.assertEqual([PINNED_CI_QT_VERSION] * 3, versions)
        for version in versions:
            self.assertTrue(version.startswith(REQUIRED_QT_MAJOR_MINOR + "."))

        self.assertNotIn("5.15", workflow)

    def test_windows_ci_keeps_the_msvc_qt_and_the_visual_studio_generator(self) -> None:
        workflow = WORKFLOW.read_text(encoding="utf-8")

        arches = re.findall(r"^\s*arch:\s*(\S+)", workflow, re.MULTILINE)
        self.assertEqual([WINDOWS_QT_ARCH], arches)
        self.assertNotIn("msvc2019", workflow)

        # Durable E2 state: plain Ninja lets CMake select MinGW and windres,
        # which cannot consume the UTF-16 TrackEditor.rc and is ABI-incompatible
        # with the MSVC Qt package. The Qt 6.8 arch bump does not license
        # unpinning this.
        self.assertIn('-G "Visual Studio 17 2022" -A x64', workflow)

    def test_documentation_names_qt_6_8(self) -> None:
        for relative in ("README.md", "docs/building.md"):
            text = (ROOT / relative).read_text(encoding="utf-8")
            self.assertIn("Qt 6.8", text, msg=relative)
            self.assertNotIn("Qt 5.15", text, msg=relative)
            self.assertNotIn("Qt5_DIR", text, msg=relative)
            self.assertNotIn("qtbase5-dev", text, msg=relative)


if __name__ == "__main__":
    unittest.main()
