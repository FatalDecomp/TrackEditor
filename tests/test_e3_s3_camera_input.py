from __future__ import annotations

import pathlib
import re
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[1]
EDITOR = ROOT / "TrackEditor"


class E3S3CameraInputContractTests(unittest.TestCase):
    def test_existing_bindings_feed_the_facade_camera_controller(self) -> None:
        preview = (EDITOR / "TrackPreview.cpp").read_text(encoding="utf-8")
        mapper = (EDITOR / "QtUserKeyMapper.cpp").read_text(encoding="utf-8")

        update = re.search(
            r"void CTrackPreview::UpdateCameraPos\(float fDeltaSeconds\)\s*"
            r"\{(?P<body>.*?)\n\}",
            preview,
            re.DOTALL,
        )
        self.assertIsNotNone(update)
        body = update.group("body")
        self.assertIn("m_keyMapper.GetCameraInput()", body)
        self.assertIn("m_CameraController.Update", body)
        self.assertIn("fDeltaSeconds", body)
        self.assertIn("ScheduleCameraRender()", body)

        bindings = {
            "bMoveForward": ("Qt::Key_W",),
            "bMoveBackward": ("Qt::Key_S",),
            "bStrafeLeft": ("Qt::Key_A",),
            "bStrafeRight": ("Qt::Key_D",),
            "bMoveUp": ("Qt::Key_R", "Qt::Key_E"),
            "bMoveDown": ("Qt::Key_F", "Qt::Key_Q"),
        }
        for field, keys in bindings.items():
            assignment = re.search(rf"Input\.{field}\s*=\s*(.*?);", mapper, re.DOTALL)
            self.assertIsNotNone(assignment)
            for key in keys:
                self.assertIn(key, assignment.group(1))
        self.assertIn("Input.bMouseLook = m_bMouseLook", mapper)

        main = (EDITOR / "MainWindow.cpp").read_text(encoding="utf-8")
        self.assertIn("m_CameraClock.nsecsElapsed()", main)
        self.assertNotIn("CGameClock", main)
        self.assertNotIn("CGameInput", main)
        self.assertNotIn("CGameClock", preview)
        self.assertNotIn("CGameInput", preview)

        display = (EDITOR / "DisplaySettings.cpp").read_text(encoding="utf-8")
        self.assertIn("CEditorCameraController::SetMovementSpeed", display)
        self.assertIn("CEditorCameraController::GetMovementSpeed", main)
        self.assertNotIn("CNoclipComponent", display)
        self.assertNotIn("CNoclipComponent", main)

    def test_camera_frames_are_coalesced_and_stay_on_the_worker(self) -> None:
        header = (EDITOR / "TrackPreview.h").read_text(encoding="utf-8")
        preview = (EDITOR / "TrackPreview.cpp").read_text(encoding="utf-8")
        controller = (EDITOR / "EditorCameraController.cpp").read_text(
            encoding="utf-8"
        )

        self.assertIn("QTimer *m_pCameraRenderTimer", header)
        self.assertIn("uint64_t m_ullCameraRequestId", header)
        self.assertIn("m_pCameraRenderTimer->setSingleShot(true)", preview)
        self.assertIn("m_pCameraRenderTimer->setInterval(16)", preview)
        self.assertIn("m_ullCameraRequestId == 0", preview)
        self.assertIn("EnqueueRender", preview)
        self.assertNotIn("RollerEd_", controller)
        self.assertNotIn("RollerEd_", preview)

    def test_no_unapproved_orbit_pan_or_zoom_bindings_were_added(self) -> None:
        header = (EDITOR / "TrackPreview.h").read_text(encoding="utf-8")
        source = (EDITOR / "TrackPreview.cpp").read_text(encoding="utf-8")
        self.assertNotIn("wheelEvent", header)
        self.assertNotIn("wheelEvent", source)
        self.assertNotIn("Qt::MiddleButton", source)
        self.assertNotIn("Qt::ShiftModifier", source)
        self.assertNotIn("Qt::ControlModifier", source)

    def test_initial_camera_uses_track_header_origin_without_geometry(self) -> None:
        preview = (EDITOR / "TrackPreview.cpp").read_text(encoding="utf-8")
        self.assertIn("m_header.iHeaderUnk1", preview)
        self.assertIn("m_header.iHeaderUnk2", preview)
        self.assertIn("m_header.iFloorDepth", preview)
        self.assertNotIn("GenerateTrackMath", preview)
        self.assertNotIn("m_chunkMathAy", preview)
        self.assertFalse((EDITOR / "TrackCoordinateConversion.h").exists())


if __name__ == "__main__":
    unittest.main()
