"""E5-S3 — warning-clean against Qt 6.8, functionally equivalent.

"Warning-clean" needs a warning level to be clean at, and this project had
none: `cmake_minimum_required(VERSION 3.24)` makes CMP0092 NEW, which drops
CMake's historical `/W3` from the default MSVC flags, so every target was
compiling at MSVC's default `/W1`. These cases pin the level, the
warnings-as-errors switch, and the Qt deprecation gate — which is the half of
the criterion that holds on every compiler rather than only the one that ran.

They also pin the Qt 6 API migrations themselves, so a later edit cannot
reintroduce a call that Qt 6 deleted.
"""

import re
from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]
WARNINGS_MODULE = ROOT / "cmake" / "TrackEditorWarnings.cmake"
TOP_LEVEL_CMAKE = ROOT / "CMakeLists.txt"
# E6-S1 moved the configure step into the reusable workflow.
WORKFLOW = ROOT / ".github" / "workflows" / "editor-build.yml"
EDITOR = ROOT / "TrackEditor"

# Every target this repository owns and compiles.
WARNED_TARGETS = {
    "track-model",
    "track-assets",
    "TrackEditor",
    "trackeditor-e3-s5a-track-model-test",
    "trackeditor-e3-s5b-track-assets-test",
    "trackeditor-e3-s1-frame-delivery-test",
    "trackeditor-e3-s1-render-service-test",
    "trackeditor-e3a-s2-overlay-settings-test",
    "trackeditor-e3-s3-camera-input-test",
    "trackeditor-e4-s1-obj-exporter-test",
    "trackeditor-e4-s5-export-format-test",
    "trackeditor-e4-s2-gltf-exporter-test",
}

QT_TARGETS = {
    "TrackEditor",
    "trackeditor-e3-s1-frame-delivery-test",
    "trackeditor-e3-s1-render-service-test",
}


def targets_passed_to(function: str, text: str) -> set[str]:
    """Targets reaching `function`, directly or through a foreach over a list."""
    covered = set(re.findall(rf"{function}\(\s*([\w-]+)\s*\)", text))
    for items, body in re.findall(
        r"foreach\(\s*\w+\s+IN\s+ITEMS\s*(.*?)\)(.*?)endforeach\(\)", text, re.S
    ):
        if function in body:
            covered.update(items.split())
    return covered


def code_lines(text: str) -> str:
    """The source with // comments dropped.

    The migrations are documented in comments that name the API they replaced,
    so an audit that reads prose flags its own explanation.
    """
    return "\n".join(line.split("//", 1)[0] for line in text.splitlines())


def owned_cmake_text() -> str:
    parts = []
    for path in ROOT.rglob("CMakeLists.txt"):
        if path.relative_to(ROOT).parts[0] in {"external", "out", "bin", "lib"}:
            continue
        parts.append(path.read_text(encoding="utf-8"))
    for path in (ROOT / "cmake").glob("*.cmake"):
        parts.append(path.read_text(encoding="utf-8"))
    return "\n".join(parts)


class WarningLevelTests(unittest.TestCase):
    def test_the_warning_level_is_raised_on_every_compiler(self) -> None:
        module = WARNINGS_MODULE.read_text(encoding="utf-8")
        self.assertIn("/W4", module)
        self.assertIn("-Wall -Wextra", module)
        self.assertIn("/WX", module)
        self.assertIn("-Werror", module)
        self.assertIn("option(TRACKEDITOR_WARNINGS_AS_ERRORS", module)

    def test_every_owned_target_is_covered(self) -> None:
        covered = targets_passed_to("trackeditor_set_warnings", owned_cmake_text())
        self.assertEqual(set(), WARNED_TARGETS - covered)

    def test_vendored_code_is_not_held_to_this_projects_warnings(self) -> None:
        # cgltf silences its own warnings and roller-core is the submodule's
        # business; neither may be swept into trackeditor_set_warnings().
        covered = targets_passed_to("trackeditor_set_warnings", owned_cmake_text())
        for vendored in ("cgltf", "roller-core"):
            self.assertNotIn(vendored, covered)


class QtDeprecationGateTests(unittest.TestCase):
    def test_the_gate_names_qt_6_8(self) -> None:
        module = WARNINGS_MODULE.read_text(encoding="utf-8")
        self.assertIn("QT_DISABLE_DEPRECATED_UP_TO", module)
        self.assertRegex(
            module, r"TRACKEDITOR_QT_DISABLE_DEPRECATED_UP_TO\s+0x060800"
        )

    def test_every_qt_compiling_target_takes_the_gate(self) -> None:
        gated = targets_passed_to(
            "trackeditor_gate_qt_deprecations", owned_cmake_text()
        )
        self.assertEqual(set(), QT_TARGETS - gated)


class MigratedApiTests(unittest.TestCase):
    def test_no_qt_5_only_api_remains(self) -> None:
        removed = re.compile(
            r"QDesktopWidget"
            r"|QApplication::desktop\(\)|app\.desktop\(\)|qApp->desktop\(\)"
            r"|AA_EnableHighDpiScaling|AA_UseHighDpiPixmaps"
            r"|QTextCodec|QRegExp\b|qrand\(|qsrand\("
            r"|QFontMetrics\w*\(\)\.width\("
            r"|QVariant::type\("
        )
        violations = []
        for path in EDITOR.rglob("*"):
            if path.suffix not in {".cpp", ".h", ".ui"}:
                continue
            source = code_lines(path.read_text(encoding="utf-8", errors="ignore"))
            if removed.search(source):
                violations.append(str(path.relative_to(ROOT)))
        self.assertEqual([], violations)

    def test_key_sequences_use_the_qt_6_combination_operator(self) -> None:
        # Qt 6 deletes Qt::operator+ for a modifier/key pair; '|' builds the
        # QKeyCombination QKeySequence takes. The deprecation gate turns a
        # relapse into a compile error, and this keeps it readable as a rule.
        for path in EDITOR.rglob("*.cpp"):
            text = code_lines(path.read_text(encoding="utf-8", errors="ignore"))
            self.assertNotRegex(
                text,
                r"Qt::(?:CTRL|SHIFT|ALT|META)\s*\+",
                msg=str(path.relative_to(ROOT)),
            )

    def test_the_high_dpi_resize_path_is_untouched(self) -> None:
        # E3-S1 requests device-pixel dimensions deliberately. Qt 6 always
        # enables high-DPI scaling, so losing this silently rescales the
        # viewport instead of failing.
        preview = (EDITOR / "TrackPreview.cpp").read_text(encoding="utf-8")
        self.assertIn("devicePixelRatioF()", preview)


class CiEnforcementTests(unittest.TestCase):
    def test_every_platform_builds_with_warnings_as_errors(self) -> None:
        # This shipped enforcing on Windows only, because Linux and macOS had
        # never produced a warning list to read. The first real nightly gave
        # one: the same two categories already fixed for MSVC -- member
        # initializer order and int/size_t comparison -- at eleven sites. With
        # those fixed, the AC's "all three desktop OSes" is finally enforced
        # rather than asserted.
        workflow = WORKFLOW.read_text(encoding="utf-8")
        self.assertEqual(
            4,
            workflow.count("-DTRACKEDITOR_WARNINGS_AS_ERRORS=ON"),
            "every matrix leg must enforce, not just the verified one",
        )


if __name__ == "__main__":
    unittest.main()
