from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]
EDITOR = ROOT / "TrackEditor"
ROLLER = ROOT / "external" / "ROLLER" / "PROJECTS" / "ROLLER"


class AnimatedStuntsTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.display_h = (EDITOR / "DisplaySettings.h").read_text(encoding="utf-8")
        cls.display_cpp = (EDITOR / "DisplaySettings.cpp").read_text(encoding="utf-8")
        cls.display_ui = (EDITOR / "DisplaySettings.ui").read_text(encoding="utf-8")
        cls.flags = (EDITOR / "DisplaySettingsFlags.h").read_text(encoding="utf-8")
        cls.window = (EDITOR / "MainWindow.cpp").read_text(encoding="utf-8")
        cls.preview_h = (EDITOR / "TrackPreview.h").read_text(encoding="utf-8")
        cls.preview_cpp = (EDITOR / "TrackPreview.cpp").read_text(encoding="utf-8")
        cls.queue = (EDITOR / "EditorRenderQueue.h").read_text(encoding="utf-8")
        cls.service = (EDITOR / "EditorRenderService.cpp").read_text(encoding="utf-8")
        cls.api_h = (ROLLER / "editor_api.h").read_text(encoding="ascii")
        cls.api_c = (ROLLER / "editor_api.c").read_text(encoding="ascii")
        cls.scene = (ROLLER / "editor_legacy_scene.c").read_text(encoding="ascii")

    def test_display_toggle_is_separate_and_persisted(self) -> None:
        self.assertIn('name="ckAnimateStunts"', self.display_ui)
        self.assertIn("<string>Animate Stunts</string>", self.display_ui)
        self.assertIn("GetAnimateStunts() const", self.display_h)
        self.assertIn("SetAnimateStunts(bool bAnimate)", self.display_h)
        self.assertIn(
            "connect(ckAnimateStunts, &QCheckBox::toggled", self.display_cpp
        )
        self.assertIn('settings.value(\n      "animate_stunts"', self.window)
        self.assertIn('settings.setValue("animate_stunts"', self.window)
        self.assertIn("GetCurrentPreview()->SetAnimateStunts", self.window)
        # SHOW_CENTER_LINE already owns the former 0x20000000 slot. Animation
        # is intentionally a separate bool rather than colliding with it.
        self.assertNotIn("ANIMATE_STUNTS", self.flags)

    def test_preview_queues_fixed_ticks_without_document_edits(self) -> None:
        self.assertIn("QTimer *m_pStuntTimer", self.preview_h)
        self.assertIn("uint32_t m_uiPendingStuntTicks", self.preview_h)
        self.assertIn("m_pStuntTimer->setInterval(28)", self.preview_cpp)
        self.assertIn("&CTrackPreview::QueueStuntTick", self.preview_cpp)
        self.assertIn("m_uiPendingStuntTicks < 8u", self.preview_cpp)
        self.assertIn("uiStuntTicks", self.queue)
        self.assertNotIn("MarkDocumentEdited();", self.preview_cpp[
            self.preview_cpp.index("void CTrackPreview::QueueStuntTick()"):
            self.preview_cpp.index("void CTrackPreview::OnRenderCompleted")
        ])

    def test_worker_advances_stunts_before_rendering_the_frame(self) -> None:
        advance = self.service.index("RollerEd_AdvanceStunts")
        render = self.service.index("RollerEd_RenderFrame")
        self.assertLess(advance, render)
        self.assertIn("Request.uiStuntTicks", self.service)
        self.assertIn('AssertWorkerThread("RollerEd_AdvanceStunts")', self.service)

    def test_roller_owns_animation_and_preserves_authored_extraction(self) -> None:
        self.assertIn("RollerEd_AdvanceStunts", self.api_h)
        start = self.api_c.index(
            "eRollerEdResult ROLLER_ED_CALL RollerEd_AdvanceStunts"
        )
        end = self.api_c.index(
            "eRollerEdResult ROLLER_ED_CALL RollerEd_RenderFrame", start
        )
        body = self.api_c[start:end]
        self.assertLess(
            body.index("roller_ed_sync_geometry_cache()"),
            body.index("roller_ed_legacy_scene_advance_stunts"),
        )
        self.assertIn("updatestunts();", self.scene)


if __name__ == "__main__":
    unittest.main()
