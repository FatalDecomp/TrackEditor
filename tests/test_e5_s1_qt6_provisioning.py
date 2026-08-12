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
# E6-S1 moved provisioning into the reusable workflow.
WORKFLOW = ROOT / ".github" / "workflows" / "editor-build.yml"

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

    # The Qt module set itself belongs to E5-S2; see
    # tests/test_e5_s2_module_set.py.

    def test_ci_provisions_qt_6_8_on_all_three_platforms(self) -> None:
        workflow = WORKFLOW.read_text(encoding="utf-8")

        # E6-S1 collapsed three hand-written jobs into one matrix, so there is
        # now one provisioning step covering three platforms rather than three
        # copies of it. What matters is unchanged: every platform gets 6.8.3.
        actions = re.findall(r"uses:\s*jurplel/install-qt-action@\S+", workflow)
        self.assertEqual(1, len(actions), "Qt is provisioned in one shared step")

        versions = re.findall(r"^\s*version:\s*'([^']+)'", workflow, re.MULTILINE)
        self.assertEqual([PINNED_CI_QT_VERSION], versions)
        self.assertTrue(versions[0].startswith(REQUIRED_QT_MAJOR_MINOR + "."))

        # One per matrix leg. Both macOS legs use clang_64 because Qt's macOS
        # package is universal, so the same download serves Intel and Apple
        # Silicon -- it is Homebrew, not Qt, that forces two legs.
        arches = re.findall(r"^\s*qt-arch:\s*(\S+)", workflow, re.MULTILINE)
        self.assertEqual(
            ["linux_gcc_64", "clang_64", "clang_64", WINDOWS_QT_ARCH],
            arches,
            "one Qt arch per matrix leg",
        )

        self.assertNotIn("5.15", workflow)

    def test_windows_ci_keeps_the_msvc_qt_and_the_visual_studio_generator(self) -> None:
        workflow = WORKFLOW.read_text(encoding="utf-8")

        self.assertIn(f"qt-arch: {WINDOWS_QT_ARCH}", workflow)
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
