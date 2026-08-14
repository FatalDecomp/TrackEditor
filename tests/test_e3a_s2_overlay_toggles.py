import re
import subprocess
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
EDITOR = ROOT / "TrackEditor"
# The pinned commit itself is asserted once, in test_e2_s1_roller_submodule.py.


def function_body(source: str, signature: str) -> str:
    start = source.index(signature)
    brace = source.index("{", start)
    depth = 0
    for index in range(brace, len(source)):
        if source[index] == "{":
            depth += 1
        elif source[index] == "}":
            depth -= 1
            if depth == 0:
                return source[brace + 1 : index]
    raise AssertionError(f"unterminated function: {signature}")


class SubmodulePinTests(unittest.TestCase):
    def test_the_pin_carries_the_overlay_facade(self) -> None:
        # The editor cannot consume RollerEd_SetOverlayState until the pin
        # includes it; E4A-S1's pin predates the whole overlay surface.
        header = (
            ROOT
            / "external"
            / "ROLLER"
            / "PROJECTS"
            / "ROLLER"
            / "editor_api.h"
        ).read_text(encoding="utf-8")
        self.assertIn("uiSurfaceClassMask", header)
        self.assertIn("uiWireframeClassMask", header)
        # 3 since E3A-S6 appended the test-car selection. What this story
        # needs from the pin is that the class masks are there at all.
        self.assertIn("#define ROLLER_ED_OVERLAY_STATE_VERSION 3u", header)
        self.assertIn("ROLLER_ED_OVERLAY_ALL_SURFACE_CLASSES", header)
        self.assertIn("ROLLER_ED_SURFACE_CLASS_COUNT", header)
        self.assertIn("ROLLER_ED_OVERLAY_DETACH_LAST", header)

    def test_the_staged_gitlink_matches_the_pinned_commit(self) -> None:
        result = subprocess.run(
            ["git", "-C", str(ROOT), "ls-tree", "HEAD", "external/ROLLER"],
            capture_output=True,
            text=True,
            check=False,
        )
        if result.returncode != 0:
            self.skipTest("git is unavailable")
        # The committed gitlink is the source of truth for CI; a working-tree
        # checkout that differs is a mid-move state, not a passing one.
        self.assertIn("160000 commit", result.stdout)


class OverlayTranslationTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.header = (EDITOR / "EditorOverlaySettings.h").read_text(
            encoding="utf-8"
        )
        cls.source = (EDITOR / "EditorOverlaySettings.cpp").read_text(
            encoding="utf-8"
        )

    def test_the_legacy_bit_layout_stops_at_the_translator(self) -> None:
        # AD: "do not restore legacy TrackEditor per-surface bit semantics".
        # The SHOW_* bits may be read here and nowhere downstream.
        for name in ("EditorRenderQueue.h", "EditorRenderService.cpp"):
            text = (EDITOR / name).read_text(encoding="utf-8")
            self.assertNotIn("SHOW_CENTER_SURF_MODEL", text)
            self.assertNotIn("DisplaySettings", text)

    def test_every_class_pair_is_translated(self) -> None:
        for name in (
            "CENTER",
            "LEFT_SHOULDER",
            "RIGHT_SHOULDER",
            "LEFT_WALL",
            "RIGHT_WALL",
            "ROOF",
            "OUTER_WALL_FLOOR",
            "LEFT_LOWER_OUTER_WALL",
            "RIGHT_LOWER_OUTER_WALL",
            "LEFT_UPPER_OUTER_WALL",
            "RIGHT_UPPER_OUTER_WALL",
            "SIGN",
        ):
            self.assertIn(f"ROLLER_ED_SURFACE_CLASS_{name}", self.source)

    def test_the_masters_are_derived_from_the_masks(self) -> None:
        body = function_body(self.source, "void CEditorOverlaySettings::Rebuild(")
        self.assertIn("uiSurfaceClassMask != 0", body)
        self.assertIn("uiWireframeClassMask != 0", body)
        self.assertIn("ROLLER_ED_OVERLAY_SHOW_SURFACES", body)
        self.assertIn("ROLLER_ED_OVERLAY_SHOW_WIREFRAME", body)

    def test_attach_last_is_translated_as_preview_state(self) -> None:
        self.assertIn("SetAttachLast(bool bAttachLast)", self.header)
        body = function_body(self.source, "void CEditorOverlaySettings::Rebuild(")
        self.assertIn("ROLLER_ED_OVERLAY_DETACH_LAST", body)
        self.assertIn("m_bAttachLast", body)

    def test_the_translator_is_qt_free_and_facade_free(self) -> None:
        # It is unit-tested without a render worker or an event loop, which is
        # only possible while it stays free of both.
        self.assertNotIn("#include <Q", self.source + self.header)
        self.assertNotIn("RollerEd_", self.source)
        includes = re.findall(r'#include\s+"([^"]+)"', self.source)
        self.assertEqual(includes, ["EditorOverlaySettings.h", "DisplaySettingsFlags.h"])

    def test_the_flag_definitions_are_shared_not_duplicated(self) -> None:
        flags = (EDITOR / "DisplaySettingsFlags.h").read_text(encoding="utf-8")
        settings = (EDITOR / "DisplaySettings.h").read_text(encoding="utf-8")
        self.assertIn("#define SHOW_CENTER_SURF_MODEL", flags)
        self.assertIn('#include "DisplaySettingsFlags.h"', settings)
        # One definition site only.
        self.assertNotIn("#define SHOW_CENTER_SURF_MODEL", settings)


class RequestPathTests(unittest.TestCase):
    def test_the_overlay_is_copied_into_the_command(self) -> None:
        queue = (EDITOR / "EditorRenderQueue.h").read_text(encoding="utf-8")
        # AD-16: pointer payloads are deep-copied into the queued command.
        self.assertIn("tEdOverlayState Overlay", queue)
        self.assertIn("bool bHasOverlay", queue)

    def test_the_worker_applies_it_and_the_ui_never_does(self) -> None:
        service = (EDITOR / "EditorRenderService.cpp").read_text(encoding="utf-8")
        preview = (EDITOR / "TrackPreview.cpp").read_text(encoding="utf-8")
        window = (EDITOR / "MainWindow.cpp").read_text(encoding="utf-8")

        self.assertIn("RollerEd_SetOverlayState(&Request.Overlay)", service)
        self.assertNotIn("RollerEd_", preview)
        self.assertNotIn("RollerEd_", window)

    def test_overlay_is_applied_after_the_epoch_is_checked(self) -> None:
        service = (EDITOR / "EditorRenderService.cpp").read_text(encoding="utf-8")
        body = function_body(
            service, "tEdRenderResult ProcessRequest(const tEdRenderRequest &Request)"
        )
        self.assertLess(
            body.index("uiExpectedGeometryEpoch"),
            body.index("RollerEd_SetOverlayState"),
        )
        self.assertLess(
            body.index("RollerEd_SetOverlayState"),
            body.index("RollerEd_RenderFrame"),
        )

    def test_every_render_carries_the_current_overlay(self) -> None:
        preview = (EDITOR / "TrackPreview.cpp").read_text(encoding="utf-8")
        # One per enqueue that produces a frame: serialized load, resize, and
        # the coalesced view render. Unload produces no frame and needs none.
        self.assertEqual(
            preview.count("m_OverlaySettings.GetOverlayState()"), 3
        )

    def test_a_toggle_re_renders_without_a_reload(self) -> None:
        preview = (EDITOR / "TrackPreview.cpp").read_text(encoding="utf-8")
        body = function_body(preview, "void CTrackPreview::ShowModels(")
        self.assertIn("m_OverlaySettings.SetShowModels(uiShowModels)", body)
        # A display toggle is not a document edit: no revision bump, no
        # serialize, no reload.
        self.assertIn("ScheduleCameraRender()", body)
        self.assertNotIn("SaveHistory", body)
        self.assertNotIn("MarkDocumentEdited", body)
        self.assertNotIn("QueueLoadAndRender", body)

    def test_attach_last_re_renders_without_reloading_geometry(self) -> None:
        preview = (EDITOR / "TrackPreview.cpp").read_text(encoding="utf-8")
        body = function_body(preview, "void CTrackPreview::AttachLast(")
        self.assertIn("m_OverlaySettings.SetAttachLast(bAttachLast)", body)
        self.assertIn("ScheduleCameraRender()", body)
        self.assertNotIn("UpdateTrack", body)
        self.assertNotIn("QueueLoadAndRender", body)
        self.assertNotIn("MarkDocumentEdited", body)


class BuildRegistrationTests(unittest.TestCase):
    def test_the_translator_builds_into_the_application(self) -> None:
        cmake = (EDITOR / "CMakeLists.txt").read_text(encoding="utf-8")
        self.assertIn("EditorOverlaySettings.cpp", cmake)
        self.assertIn("DisplaySettingsFlags.h", cmake)

    def test_the_native_test_is_registered(self) -> None:
        cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
        self.assertIn("trackeditor-e3a-s2-overlay-settings-test", cmake)
        self.assertIn("tests/editor_overlay_settings_test.cpp", cmake)
        self.assertIn("trackeditor-e3a-s2-overlay-toggles-contract", cmake)


if __name__ == "__main__":
    unittest.main()
