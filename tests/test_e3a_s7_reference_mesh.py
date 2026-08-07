import re
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


def without_comments(source: str) -> str:
    source = re.sub(r"/\*.*?\*/", "", source, flags=re.DOTALL)
    return re.sub(r"//.*", "", source)


class PinnedCoreTests(unittest.TestCase):
    def test_the_pin_carries_reference_mesh_rendering(self) -> None:
        roller = ROOT / "external" / "ROLLER" / "PROJECTS" / "ROLLER"
        api = (roller / "editor_api.c").read_text(encoding="utf-8")
        draw = (roller / "drawtrk3.h").read_text(encoding="utf-8")

        # The facade used to validate the header and then refuse.
        body = function_body(
            api, "eRollerEdResult ROLLER_ED_CALL RollerEd_SetReferenceMesh("
        )
        self.assertNotIn("ROLLER_ED_RESULT_UNSUPPORTED", body)
        self.assertIn(
            "void drawtrk3_editor_draw_reference_mesh(GameRenderer *pRenderer);",
            draw,
        )


class PayloadTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.header = (EDITOR / "EditorReferenceMesh.h").read_text(
            encoding="utf-8"
        )
        cls.source = (EDITOR / "EditorReferenceMesh.cpp").read_text(
            encoding="utf-8"
        )

    def test_the_payload_owns_no_qt_and_calls_no_facade(self) -> None:
        combined = without_comments(self.header) + without_comments(self.source)
        self.assertNotIn("RollerEd_", combined)
        self.assertNotIn("#include <Q", combined)

    def test_it_publishes_a_versioned_struct(self) -> None:
        body = without_comments(
            function_body(
                self.source, "tEdReferenceMesh CEditorReferenceMesh::GetMesh("
            )
        )
        self.assertIn("ROLLER_ED_REFERENCE_MESH_VERSION", body)
        self.assertIn("sizeof(Mesh)", body)

    def test_wireframe_and_normals_are_mesh_flags(self) -> None:
        body = without_comments(
            function_body(
                self.source, "tEdReferenceMesh CEditorReferenceMesh::GetMesh("
            )
        )
        self.assertIn("ROLLER_ED_REFERENCE_HAS_NORMALS", body)
        self.assertIn("ROLLER_ED_REFERENCE_WIREFRAME", body)

    def test_an_empty_mesh_publishes_null_rather_than_a_dangling_pointer(
        self,
    ) -> None:
        body = without_comments(
            function_body(
                self.source, "tEdReferenceMesh CEditorReferenceMesh::GetMesh("
            )
        )
        # AD-13: NULL vertices or a zero count clears the mesh, so an empty
        # vector must not publish data() as a non-null pointer.
        self.assertIn("m_Vertices.empty()", body)
        self.assertIn("m_Indices.empty()", body)


class QueueTests(unittest.TestCase):
    def test_the_mesh_is_deep_copied_into_the_command(self) -> None:
        # AD-16. The facade copies during the call, but the call happens on
        # the worker long after the UI built the arrays.
        queue = (EDITOR / "EditorRenderQueue.h").read_text(encoding="utf-8")
        self.assertIn("struct tEdReferenceMeshPayload", queue)
        self.assertIn("std::vector<tEdReferenceVertex> Vertices;", queue)
        self.assertIn("std::vector<uint32_t> Indices;", queue)
        self.assertIn("bool bHasReferenceMesh = false;", queue)

        service = (EDITOR / "EditorRenderService.cpp").read_text(
            encoding="utf-8"
        )
        body = without_comments(
            function_body(service, "uint64_t CEditorRenderService::EnqueueRender(")
        )
        self.assertIn("Request.ReferenceMesh = *pReferenceMesh;", body)
        self.assertIn("Request.bHasReferenceMesh = true;", body)

    def test_the_worker_applies_it_before_rendering(self) -> None:
        service = (EDITOR / "EditorRenderService.cpp").read_text(
            encoding="utf-8"
        )
        body = without_comments(
            function_body(
                service,
                "tEdRenderResult ProcessRequest(const tEdRenderRequest &Request)",
            )
        )
        self.assertIn("RollerEd_SetReferenceMesh", body)
        # After the epoch check, like the camera and the overlay, and before
        # the frame is rendered.
        self.assertLess(
            body.index("RollerEd_SetOverlayState"),
            body.index("RollerEd_SetReferenceMesh"),
        )
        self.assertLess(
            body.index("RollerEd_SetReferenceMesh"),
            body.index("RollerEd_RenderFrame"),
        )

    def test_only_the_ui_thread_builds_the_payload(self) -> None:
        # UI code never calls RollerEd_*; the mesh reaches the core through
        # the queue like every other command.
        preview = without_comments(
            (EDITOR / "TrackPreview.cpp").read_text(encoding="utf-8")
        )
        self.assertNotIn("RollerEd_SetReferenceMesh", preview)


class PreviewTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.source = (EDITOR / "TrackPreview.cpp").read_text(encoding="utf-8")

    def test_the_placeholder_dialog_is_gone(self) -> None:
        body = function_body(
            self.source, "void CTrackPreview::OpenReferenceModel("
        )
        self.assertNotIn("will return with the roller-core overlay stories", body)
        self.assertIn("ImportObj", body)
        self.assertIn("SetGeometry", body)

    def test_moving_the_model_is_a_view_change(self) -> None:
        body = without_comments(
            function_body(
                self.source, "void CTrackPreview::UpdateReferenceModelPos_Internal("
            )
        )
        self.assertIn("ScheduleReferenceMeshUpload", body)

        upload = without_comments(
            function_body(
                self.source, "void CTrackPreview::ScheduleReferenceMeshUpload("
            )
        )
        self.assertIn("ScheduleCameraRender();", upload)
        # No revision bump, no serialize, no reload.
        self.assertNotIn("m_uiRevision", upload)
        self.assertNotIn("GetTrackData", upload)
        self.assertNotIn("SaveHistory", upload)

    def test_the_mesh_uploads_once_per_change(self) -> None:
        # Re-sending the whole model on every camera nudge would copy it
        # through the queue each frame.
        body = without_comments(
            function_body(
                self.source,
                "const tEdReferenceMeshPayload *CTrackPreview::TakePendingReferenceMesh(",
            )
        )
        self.assertIn("m_bReferenceMeshDirty", body)
        self.assertIn("return nullptr;", body)


class WireframeTests(unittest.TestCase):
    def test_the_wireframe_checkbox_is_fully_wired(self) -> None:
        # E3A-S4's lesson: a display checkbox needs reading, writing, and
        # connecting, not just existing.
        settings = (EDITOR / "DisplaySettings.cpp").read_text(encoding="utf-8")
        self.assertIn("ckRefModelWireframe->isChecked()", settings)
        self.assertIn("BLOCK_SIG_AND_DO(ckRefModelWireframe", settings)
        self.assertIn(
            "connect(ckRefModelWireframe, &QCheckBox::toggled, this, "
            "&CDisplaySettings::UpdatePreviewSelection);",
            settings,
        )

    def test_it_reaches_the_mesh_rather_than_the_overlay(self) -> None:
        window = (EDITOR / "MainWindow.cpp").read_text(encoding="utf-8")
        body = without_comments(
            function_body(window, "void CMainWindow::OnUpdatePreview(")
        )
        self.assertIn("UpdateReferenceModelWireframe", body)
        self.assertIn("SHOW_REF_WIRE_MODEL", body)

        # The per-class surface wireframes are overlay flags; this one is not.
        overlay = (EDITOR / "EditorOverlaySettings.cpp").read_text(
            encoding="utf-8"
        )
        self.assertNotIn("SHOW_REF_WIRE_MODEL", overlay)


class BuildRegistrationTests(unittest.TestCase):
    def test_the_translation_unit_is_registered(self) -> None:
        app = (EDITOR / "CMakeLists.txt").read_text(encoding="utf-8")
        self.assertIn("EditorReferenceMesh.cpp", app)
        root = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
        self.assertIn("EditorReferenceMesh.cpp", root)
        self.assertIn("trackeditor-e3a-s7-reference-mesh-contract", root)


if __name__ == "__main__":
    unittest.main()
