import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
EDITOR = ROOT / "TrackEditor"


class PinnedCoreTests(unittest.TestCase):
    def test_the_pin_carries_the_marker_geometry(self) -> None:
        # The checkboxes have existed since before the migration and have
        # mapped to their overlay flags since E3A-S2; nothing draws until the
        # core that derives the marker geometry is pinned.
        roller = ROOT / "external" / "ROLLER" / "PROJECTS" / "ROLLER"
        helpers = (roller / "editor_helpers.h").read_text(encoding="utf-8")

        for symbol in (
            "ed_helper_chunk_has_audio",
            "ed_helper_stunt_count",
            "ed_helper_stunt_chunk",
            "ed_helper_marker_quad",
        ):
            self.assertIn(symbol, helpers)


class MarkerToggleTests(unittest.TestCase):
    def test_both_checkboxes_map_to_their_overlay_flags(self) -> None:
        source = (EDITOR / "EditorOverlaySettings.cpp").read_text(encoding="utf-8")
        for legacy, overlay in (
            ("SHOW_AUDIO", "ROLLER_ED_OVERLAY_SHOW_AUDIO_MARKERS"),
            ("SHOW_STUNTS", "ROLLER_ED_OVERLAY_SHOW_STUNT_MARKERS"),
        ):
            self.assertIn(legacy, source)
            self.assertIn(overlay, source)

    def test_the_markers_are_independently_toggleable(self) -> None:
        # Each is its own row in the feature table, so no two share a bit.
        source = (EDITOR / "EditorOverlaySettings.cpp").read_text(encoding="utf-8")
        start = source.index("g_aFeatures[]")
        table = source[start : source.index("};", start)]
        for name in (
            "ROLLER_ED_OVERLAY_SHOW_AUDIO_MARKERS",
            "ROLLER_ED_OVERLAY_SHOW_STUNT_MARKERS",
        ):
            self.assertEqual(table.count(name), 1)

    def test_both_checkboxes_are_read_written_and_connected(self) -> None:
        source = (EDITOR / "DisplaySettings.cpp").read_text(encoding="utf-8")

        # E3A-S4 shipped a checkbox that changed the stored mask but fired no
        # signal, so it only took effect once some other box re-read the whole
        # mask. A display checkbox needs all four of these, not three.
        for box, flag in (("ckAudio", "SHOW_AUDIO"), ("ckStunts", "SHOW_STUNTS")):
            self.assertIn(f"{box}->isChecked()", source)
            self.assertIn(flag, source)
            self.assertIn(f"BLOCK_SIG_AND_DO({box}", source)
            self.assertIn(
                f"connect({box}, &QCheckBox::toggled, this, "
                "&CDisplaySettings::UpdatePreviewSelection);",
                source,
            )

    def test_the_legacy_bits_are_unchanged(self) -> None:
        # Both bits predate the migration and are persisted in QSettings, so
        # unlike E3A-S4's new bit they need no default migration -- but only
        # while they keep the values existing profiles were saved with.
        flags = (EDITOR / "DisplaySettingsFlags.h").read_text(encoding="utf-8")
        self.assertIn("#define SHOW_AUDIO                 0x08000000", flags)
        self.assertIn("#define SHOW_STUNTS                0x10000000", flags)


class MarkerBoundaryTests(unittest.TestCase):
    def test_the_legacy_mask_stops_at_the_translator(self) -> None:
        # AD-8/E3A-S2: the queue and the worker never see a SHOW_* bit.
        for name in ("EditorRenderQueue.h", "EditorRenderService.cpp"):
            source = (EDITOR / name).read_text(encoding="utf-8")
            self.assertNotIn("SHOW_AUDIO", source)
            self.assertNotIn("SHOW_STUNTS", source)

    def test_showing_markers_is_not_a_document_edit(self) -> None:
        # A view change rides the coalesced render-only path: no revision
        # bump, no serialize, no reload.
        source = (EDITOR / "TrackPreview.cpp").read_text(encoding="utf-8")
        start = source.index("void CTrackPreview::ShowModels(")
        body = source[start : source.index("\n}", start)]
        self.assertNotIn("m_uiRevision++", body)
        self.assertNotIn("GetTrackData", body)


if __name__ == "__main__":
    unittest.main()
