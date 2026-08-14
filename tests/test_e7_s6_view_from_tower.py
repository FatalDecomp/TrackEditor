import unittest
import xml.etree.ElementTree as ET
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
EDITOR = ROOT / "TrackEditor"


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def function_body(text: str, signature: str) -> str:
    start = text.index(signature)
    brace = text.index("{", start)
    depth = 0
    for position in range(brace, len(text)):
        if text[position] == "{":
            depth += 1
        elif text[position] == "}":
            depth -= 1
            if depth == 0:
                return text[start : position + 1]
    raise AssertionError(f"unterminated function: {signature}")


class TowerDockTests(unittest.TestCase):
    def test_the_dock_has_a_view_from_tower_button(self) -> None:
        tree = ET.parse(EDITOR / "EditTowerWidget.ui")
        button = tree.find(".//widget[@name='pbViewFromTower']")
        self.assertIsNotNone(button)
        self.assertEqual(
            button.findtext("property[@name='text']/string"),
            "View from tower",
        )

        widget = read(EDITOR / "EditTowerWidget.cpp")
        self.assertIn(
            "connect(pbViewFromTower, &QPushButton::clicked,", widget
        )
        self.assertIn("pbViewFromTower->setEnabled(bHasTower);", widget)

    def test_the_selected_tower_chunk_is_sent_to_the_current_preview(self) -> None:
        widget = read(EDITOR / "EditTowerWidget.cpp")
        body = function_body(
            widget, "void CEditTowerWidget::ViewFromTowerClicked()"
        )
        self.assertIn("CEditorTowerModel::IsTower(", body)
        self.assertIn("g_pMainWindow->GetCurrentPreview()", body)
        self.assertIn("pPreview->ViewFromTower(iFrom);", body)
        self.assertNotIn("RollerEd_", body)


class WorkerSnapshotTests(unittest.TestCase):
    def test_load_results_own_the_committed_tower_table(self) -> None:
        queue = read(EDITOR / "EditorRenderQueue.h")
        self.assertIn("std::vector<tEdTowerInfo> Towers;", queue)
        self.assertIn("bool bHasTowerSnapshot = false;", queue)

        service = read(EDITOR / "EditorRenderService.cpp")
        query = function_body(
            service, "bool QueryTowers(tEdRenderResult &Result)"
        )
        for contract in (
            'AssertWorkerThread("RollerEd_QueryTowerCount")',
            "RollerEd_QueryTowerCount(&uiTowerCount)",
            'AssertWorkerThread("RollerEd_QueryTower")',
            "RollerEd_QueryTower(uiIndex, &Info)",
            "ROLLER_ED_TOWER_INFO_VERSION",
        ):
            self.assertIn(contract, query)

        process = function_body(
            service,
            "tEdRenderResult ProcessRequest(const tEdRenderRequest &Request)",
        )
        self.assertIn("Result.bHasTowerSnapshot = bLoadCommand;", process)
        self.assertIn("if (bLoadCommand && !QueryTowers(Result))", process)
        self.assertLess(
            process.index("Sizes.uiSceneState != ROLLER_ED_SCENE_READY"),
            process.index("if (bLoadCommand && !QueryTowers(Result))"),
        )

    def test_only_an_accepted_result_replaces_the_preview_table(self) -> None:
        preview = read(EDITOR / "TrackPreview.cpp")
        completed = function_body(
            preview,
            "void CTrackPreview::OnRenderCompleted(const tEdRenderResult &Result)",
        )
        self.assertLess(
            completed.index("m_FrameState.ApplyResult(Result)"),
            completed.index("m_Towers = Result.Towers"),
        )
        self.assertIn("if (Result.bHasTowerSnapshot)", completed)


class CameraPlacementTests(unittest.TestCase):
    def test_the_cached_position_and_anchor_drive_the_existing_controller(self) -> None:
        preview = read(EDITOR / "TrackPreview.cpp")
        body = function_body(preview, "bool CTrackPreview::ViewFromTower")
        for contract in (
            "Tower.uiChunkId",
            "Tower.fWorldPosition",
            "Tower.fAnchorPosition",
            "CEditorCameraController::CalculateLookAtOrientation(",
            "m_CameraController.SetPosition(",
            "m_CameraController.SetOrientation(",
            "m_CameraController.ResetMouseTracking();",
            "ScheduleCameraRender();",
        ):
            self.assertIn(contract, body)
        self.assertNotIn("RollerEd_", body)

    def test_look_at_math_is_covered_by_the_native_camera_target(self) -> None:
        camera = read(EDITOR / "EditorCameraController.cpp")
        look_at = function_body(
            camera,
            "bool CEditorCameraController::CalculateLookAtOrientation",
        )
        self.assertIn("std::atan2(fDeltaY, fDeltaX)", look_at)
        self.assertIn("std::atan2(fDeltaZ, fHorizontalDistance)", look_at)

        native = read(ROOT / "tests" / "editor_camera_controller_test.cpp")
        self.assertIn("TestLookAtOrientationUsesEditorDegrees", native)
        self.assertIn("DiagonalUp", native)
        self.assertIn("Vertical", native)

    def test_worker_round_trip_is_covered_natively(self) -> None:
        native = read(ROOT / "tests" / "editor_render_service_test.cpp")
        self.assertIn("RollerEd_QueryTowerCount", native)
        self.assertIn("RollerEd_QueryTower(", native)
        self.assertIn("GoodResult.bHasTowerSnapshot", native)
        self.assertIn("!MeshResult.bHasTowerSnapshot", native)
        self.assertIn("EditedResult.Towers[0].fWorldPosition[0]", native)
        self.assertIn("EmptyTabBResult.Towers.empty()", native)


if __name__ == "__main__":
    unittest.main()
