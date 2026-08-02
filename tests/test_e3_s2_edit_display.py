from __future__ import annotations

import pathlib
import re
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[1]
EDITOR = ROOT / "TrackEditor"


class E3S2EditDisplayContractTests(unittest.TestCase):
    def test_visible_edits_are_debounced_and_reload_a_serialized_snapshot(self) -> None:
        header = (EDITOR / "TrackPreview.h").read_text(encoding="utf-8")
        source = (EDITOR / "TrackPreview.cpp").read_text(encoding="utf-8")

        self.assertIn("QTimer *m_pEditTimer", header)
        self.assertIn("m_pEditTimer->setSingleShot(true)", source)
        self.assertIn("m_pEditTimer->setInterval(100)", source)
        self.assertIn(
            "this, &CTrackPreview::QueueEditedTrackReload", source
        )

        edited = re.search(
            r"void CTrackPreview::MarkDocumentEdited\(\)\s*\{(?P<body>.*?)\n\}",
            source,
            re.DOTALL,
        )
        self.assertIsNotNone(edited)
        self.assertIn("m_FrameState.MarkDocumentEdited()", edited.group("body"))
        self.assertIn("isVisible()", edited.group("body"))
        self.assertIn("m_pEditTimer->start()", edited.group("body"))

        debounced = re.search(
            r"void CTrackPreview::QueueEditedTrackReload\(\)\s*\{(?P<body>.*?)\n\}",
            source,
            re.DOTALL,
        )
        self.assertIsNotNone(debounced)
        self.assertIn("isVisible()", debounced.group("body"))
        self.assertIn("QueueLoadAndRender()", debounced.group("body"))

        queue = re.search(
            r"void CTrackPreview::QueueLoadAndRender\(\)\s*\{(?P<body>.*?)\n\}",
            source,
            re.DOTALL,
        )
        self.assertIsNotNone(queue)
        body = queue.group("body")
        self.assertIn("p->m_track.GetTrackData(TrackData)", body)
        self.assertIn("EnqueueSerializedLoadAndRender", body)
        self.assertIn("m_sDocumentAssetRoot", body)

    def test_tab_switch_reloads_without_reinitializing_the_facade(self) -> None:
        main = (EDITOR / "MainWindow.cpp").read_text(encoding="utf-8")
        service = (EDITOR / "EditorRenderService.cpp").read_text(encoding="utf-8")

        tab_changed = re.search(
            r"void CMainWindow::OnTabChanged\([^)]*\)\s*\{(?P<body>.*?)\n\}",
            main,
            re.DOTALL,
        )
        self.assertIsNotNone(tab_changed)
        self.assertIn("->Activate()", tab_changed.group("body"))
        self.assertIn("QTemporaryFile", service)
        self.assertIn('AssertWorkerThread("RollerEd_LoadTrackFile")', service)
        self.assertEqual(service.count("RollerEd_Init(&InitInfo)"), 1)

    def test_document_asset_root_is_not_replaced_by_the_temp_directory(self) -> None:
        source = (EDITOR / "TrackPreview.cpp").read_text(encoding="utf-8")
        self.assertIn(
            "m_sDocumentAssetRoot = QFileInfo(sFilename).absolutePath()", source
        )
        self.assertNotIn("QFileInfo(Temporary", source)


if __name__ == "__main__":
    unittest.main()
