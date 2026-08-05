import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
EDITOR = ROOT / "TrackEditor"


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


class PinnedCoreTests(unittest.TestCase):
    def test_the_pin_carries_the_selection_highlight(self) -> None:
        # The editor half compiles without it, because SetSelectionRange
        # landed in E3A-S2 -- but nothing highlights until the core that
        # consumes the range is pinned.
        roller = ROOT / "external" / "ROLLER" / "PROJECTS" / "ROLLER"
        surface = (roller / "editor_surface.h").read_text(encoding="utf-8")
        draw = (roller / "drawtrk3.h").read_text(encoding="utf-8")

        self.assertIn("ED_SURFACE_SELECTION_ANY_CLASS", surface)
        self.assertIn("void drawtrk3_editor_apply_overlay_selection(void);", draw)


class SelectionWiringTests(unittest.TestCase):
    def test_the_preview_publishes_its_selection_to_the_overlay(self) -> None:
        preview = (EDITOR / "TrackPreview.cpp").read_text(encoding="utf-8")
        body = function_body(preview, "void CTrackPreview::UpdateGeometrySelection(")

        self.assertIn(
            "m_OverlaySettings.SetSelectionRange(m_iSelFrom, m_iSelTo)", body
        )
        # A selection change is a view change: it must not bump the document
        # revision, save history, or reload the worker scene.
        self.assertIn("ScheduleCameraRender()", body)
        self.assertNotIn("SaveHistory", body)
        self.assertNotIn("MarkDocumentEdited", body)
        self.assertNotIn("QueueLoadAndRender", body)
        self.assertNotIn("RollerEd_", body)

    def test_the_window_feeds_the_preview_before_it_republishes(self) -> None:
        window = (EDITOR / "MainWindow.cpp").read_text(encoding="utf-8")
        body = function_body(window, "void CMainWindow::UpdateGeometrySelection(")

        self.assertLess(
            body.index("m_iSelFrom = sbSelChunksFrom->value()"),
            body.index("UpdateGeometrySelection()"),
        )
        self.assertLess(
            body.index("m_iSelTo = sbSelChunksTo->value()"),
            body.index("UpdateGeometrySelection()"),
        )

    def test_the_range_is_ordered_before_it_leaves_the_window(self) -> None:
        # The core orders a reversed range anyway, but the editor's own
        # invariant is that From <= To and that an unchecked "to" collapses
        # onto From. Both spin boxes enforce it.
        window = (EDITOR / "MainWindow.cpp").read_text(encoding="utf-8")
        from_body = function_body(
            window, "void CMainWindow::OnSelChunksFromChanged(int iValue)"
        )
        to_body = function_body(
            window, "void CMainWindow::OnSelChunksToChanged(int iValue)"
        )
        self.assertIn("ckTo->isChecked()", from_body)
        self.assertIn("sbSelChunksTo, setValue(iValue)", from_body)
        self.assertIn("sbSelChunksFrom, setValue(iValue)", to_body)

    def test_the_translator_maps_the_highlight_checkbox(self) -> None:
        source = (EDITOR / "EditorOverlaySettings.cpp").read_text(encoding="utf-8")
        self.assertIn("SHOW_SELECTION_HIGHLIGHT", source)
        self.assertIn("ROLLER_ED_OVERLAY_HIGHLIGHT_SELECTION", source)

    def test_no_selection_uses_the_facade_sentinel(self) -> None:
        source = (EDITOR / "EditorOverlaySettings.cpp").read_text(encoding="utf-8")
        body = function_body(source, "void CEditorOverlaySettings::Rebuild(")
        # -1 is the editor's "nothing selected"; chunk zero is a real chunk.
        self.assertIn("m_iSelFrom < 0", body)
        self.assertIn("m_iSelTo < 0", body)
        self.assertIn("ROLLER_ED_INVALID_CHUNK_ID", body)

    def test_the_native_case_covers_the_selection_range(self) -> None:
        test = (ROOT / "tests" / "editor_overlay_settings_test.cpp").read_text(
            encoding="utf-8"
        )
        self.assertIn("test_selection_range_uses_the_sentinel_for_no_selection", test)


if __name__ == "__main__":
    unittest.main()
