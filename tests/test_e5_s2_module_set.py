"""E5-S2 — the Qt module set is Widgets/Gui/Core, and nothing links Qt OpenGL.

E3-S1 put the viewport on a plain `QWidget` blitting an immutable `QImage` and
E3-S4 deleted the WhipLib engine, every shader, and GLEW, so the criterion was
already true at Qt 5. The story is about *keeping* it true through the version
bump and making it enforceable, which takes three layers this file pins:

1. the declared module set, here;
2. `trackeditor_assert_no_opengl_in_link_closure()`, which walks the real link
   closure at configure time and fails the build;
3. `tests/check_linked_qt_modules.py`, which reads the module names back out of
   the linked executable.

Qt 6 helps: `QOpenGL*` moved out of QtGui into its own QtOpenGL module, so at
Qt 6 the split the editor wants is the framework's own default.
"""

import re
from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]
TOP_LEVEL_CMAKE = ROOT / "CMakeLists.txt"
GUARD_MODULE = ROOT / "cmake" / "TrackEditorNoOpenGL.cmake"

ALLOWED_QT_TARGETS = {"Qt6::Core", "Qt6::Gui", "Qt6::Widgets"}

SOURCE_DIRECTORIES = (
    "TrackEditor",
    "WhipLib",
    "TrackModel",
    "TrackAssets",
    "ModelExporter",
)


def owned_cmake_files() -> list[Path]:
    owned = []
    for path in ROOT.rglob("CMakeLists.txt"):
        if path.relative_to(ROOT).parts[0] in {"external", "out", "bin", "lib"}:
            continue
        owned.append(path)
    return owned


class ModuleSetTests(unittest.TestCase):
    def test_find_package_requests_only_core_gui_and_widgets(self) -> None:
        text = TOP_LEVEL_CMAKE.read_text(encoding="utf-8")
        components = re.search(
            r"find_package\s*\(\s*Qt6[^)]*COMPONENTS([^)]*)\)", text
        )
        self.assertIsNotNone(components)
        self.assertEqual(["Core", "Gui", "Widgets"], components.group(1).split())

    def test_every_qt_link_target_is_core_gui_or_widgets(self) -> None:
        found = set()
        for path in owned_cmake_files():
            found.update(re.findall(r"Qt\d::\w+", path.read_text(encoding="utf-8")))
        self.assertTrue(found, "no Qt link targets found at all")
        self.assertEqual(set(), found - ALLOWED_QT_TARGETS)

    def test_the_link_closure_guard_exists_and_covers_every_qt_target(self) -> None:
        self.assertTrue(GUARD_MODULE.is_file())
        guard = GUARD_MODULE.read_text(encoding="utf-8")
        self.assertIn(
            "function(trackeditor_assert_no_opengl_in_link_closure root)", guard
        )
        for pattern in ("Qt[0-9]*::OpenGL", "OpenGL::", "GLEW::"):
            self.assertIn(f'"{pattern}"', guard)

        text = TOP_LEVEL_CMAKE.read_text(encoding="utf-8")
        self.assertIn("include(TrackEditorNoOpenGL)", text)

        guarded = set(
            re.findall(
                r"trackeditor_assert_no_opengl_in_link_closure\(\s*([\w-]+)\s*\)", text
            )
        )
        self.assertEqual(
            {
                "TrackEditor",
                "trackeditor-e3-s1-frame-delivery-test",
                "trackeditor-e3-s1-render-service-test",
            },
            guarded,
            "every target that links Qt must be guarded",
        )

    def test_the_linked_module_check_is_registered(self) -> None:
        text = TOP_LEVEL_CMAKE.read_text(encoding="utf-8")
        self.assertIn("trackeditor-e5-s2-linked-qt-modules", text)
        self.assertIn("check_linked_qt_modules.py", text)
        self.assertIn("$<TARGET_FILE:TrackEditor>", text)
        self.assertTrue((ROOT / "tests" / "check_linked_qt_modules.py").is_file())

    def test_no_source_file_uses_a_qt_opengl_type(self) -> None:
        forbidden = re.compile(
            r"QOpenGL\w*|QtOpenGL|QGLWidget|QGLContext|QGLFormat"
            r"|QSurfaceFormat|QOffscreenSurface|QRhi\w*"
        )
        violations = []
        for directory in SOURCE_DIRECTORIES:
            root = ROOT / directory
            if not root.is_dir():
                continue
            for path in root.rglob("*"):
                if path.suffix not in {".cpp", ".h", ".ui", ".qrc"}:
                    continue
                if forbidden.search(path.read_text(encoding="utf-8", errors="ignore")):
                    violations.append(str(path.relative_to(ROOT)))
        self.assertEqual([], violations)


if __name__ == "__main__":
    unittest.main()
