from __future__ import annotations

import pathlib
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[1]
EDITOR = ROOT / "TrackEditor"


def function_body(source: str, signature: str, next_signature: str) -> str:
    start = source.index(signature)
    end = source.index(next_signature, start)
    return source[start:end]


class MultiDocumentCopyPasteContractTests(unittest.TestCase):
    def test_copy_paste_uses_scalar_rows_and_one_atomic_history_entry(self) -> None:
        source = (EDITOR / "MainWindow.cpp").read_text(encoding="utf-8")
        copy_body = function_body(
            source, "void CMainWindow::OnCopy()", "void CMainWindow::OnPaste()"
        )
        paste_body = function_body(
            source, "void CMainWindow::OnPaste()", "void CMainWindow::OnSelectAll()"
        )

        self.assertIn("m_clipBoard.push_back(GetCurrentTrack()->m_chunkAy[i])", copy_body)
        self.assertIn("m_chunkAy.insert", paste_body)
        self.assertIn("m_chunkAy.erase", paste_body)
        self.assertNotIn("OnDeleteChunkClicked()", paste_body)
        self.assertEqual(paste_body.count("SaveHistory("), 1)
        self.assertNotIn("TrackGeometry", copy_body + paste_body)
        self.assertNotIn("glm", copy_body + paste_body)

    def test_each_tab_owns_its_model_and_history(self) -> None:
        preview = (EDITOR / "TrackPreview.cpp").read_text(encoding="utf-8")
        self.assertIn("CTrack m_track", preview)
        self.assertIn("CTrackHistory m_history", preview)
        self.assertIn("p->m_history.Undo(p->m_track)", preview)
        self.assertIn("p->m_history.Redo(p->m_track)", preview)

    def test_empty_undo_unloads_instead_of_serializing_invalid_header(self) -> None:
        preview = (EDITOR / "TrackPreview.cpp").read_text(encoding="utf-8")
        service = (EDITOR / "EditorRenderService.cpp").read_text(encoding="utf-8")
        frame_state = (EDITOR / "EditorRenderQueue.cpp").read_text(encoding="utf-8")

        self.assertIn("p->m_track.m_chunkAy.empty()", preview)
        self.assertIn("EnqueueUnload", preview)
        self.assertIn("RollerEd_UnloadTrack for empty document", service)
        self.assertIn("Result.bSceneEmpty = true", service)
        self.assertIn("if (Result.bSceneEmpty)", frame_state)
        self.assertIn("eEdFrameDisplayState::PLACEHOLDER", frame_state)


if __name__ == "__main__":
    unittest.main()
