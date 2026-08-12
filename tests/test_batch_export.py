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


class BatchExportMenuTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.window = (EDITOR / "MainWindow.cpp").read_text(encoding="utf-8")
        cls.header = (EDITOR / "MainWindow.h").read_text(encoding="utf-8")
        cls.ui = (EDITOR / "MainWindow.ui").read_text(encoding="utf-8")

    def test_batch_actions_are_last_after_a_second_separator(self) -> None:
        file_menu = self.ui[
            self.ui.index('<widget class="QMenu" name="menuFile">') :
            self.ui.index('<widget class="QMenu" name="menuHelp">')
        ]
        actions = re.findall(r'<addaction name="([^"]+)"/>', file_menu)
        self.assertEqual(
            [
                "actExportOBJ",
                "actExportGLTF",
                "separator",
                "actExportAllOBJ",
                "actExportAllGLTF",
            ],
            actions[-5:],
        )
        self.assertIn("Export all tracks and cars OBJ", self.ui)
        self.assertIn("Export all tracks and cars to glTF", self.ui)

    def test_both_actions_are_wired_to_the_requested_formats(self) -> None:
        for action, slot, kind in (
            ("actExportAllOBJ", "OnExportAllOBJ", "EXPORT_OBJ"),
            ("actExportAllGLTF", "OnExportAllGLTF", "EXPORT_GLTF"),
        ):
            self.assertIn(slot, self.header)
            self.assertIn(
                f"connect({action}, &QAction::triggered", self.window
            )
            body = function_body(self.window, f"void CMainWindow::{slot}()")
            self.assertIn(f"eExportType::{kind}", body)

    def test_fatdata_is_selected_and_validated_before_output(self) -> None:
        body = function_body(
            self.window, "void CMainWindow::ExportAllTracksAndCars("
        )
        fatdata = body.index('"Select FATDATA Folder"')
        validation = body.index("IsFatdataFolder")
        output = body.index('"Select Output Folder"')
        export = body.index("Exporter.Export")
        self.assertLess(fatdata, validation)
        self.assertLess(validation, output)
        self.assertLess(output, export)

    def test_batch_actions_do_not_require_an_open_document(self) -> None:
        body = function_body(
            self.window, "void CMainWindow::UpdateExportActions()"
        )
        self.assertNotIn("actExportAllOBJ", body)
        self.assertNotIn("actExportAllGLTF", body)


class BatchExportPipelineTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.batch = (EDITOR / "EditorBatchExporter.cpp").read_text(
            encoding="utf-8"
        )
        cls.car = (EDITOR / "EditorCarModel.cpp").read_text(encoding="utf-8")
        cls.preview = (EDITOR / "TrackPreview.cpp").read_text(encoding="utf-8")
        cls.common = (EDITOR / "EditorExportCommon.cpp").read_text(
            encoding="utf-8"
        )

    def test_all_tracks_and_all_car_variants_are_walked(self) -> None:
        self.assertIn('suffix().compare("trk", Qt::CaseInsensitive)', self.batch)
        self.assertIn("CEditorCarModel::Count()", self.batch)
        self.assertIn("CEditorCarModel::ExportCount()", self.batch)
        self.assertIn("CEditorCarModel::HasAdvancedVariant", self.batch)
        self.assertIn("ROLLER_ED_TEST_CAR_DESIGN_COUNT", self.car)
        for name in (
            "XAUTO",
            "YAUTO",
            "XZIZIN",
            "YZIZIN",
            "SUICYCO",
            "F1WACK",
            "DEATH",
        ):
            self.assertIn(f'"{name}"', self.car)

    def test_advanced_cars_use_y_textures_and_flat_colour_remaps(self) -> None:
        self.assertIn('"yauto.bm"', self.car)
        self.assertIn('"yzizin.bm"', self.car)
        self.assertIn("car_flat_remap[uiDesign]", self.car)
        self.assertIn("ApplyAdvancedColour", self.car)

    def test_complete_reverse_geometry_and_car_backs_are_preserved(self) -> None:
        self.assertIn("SURFACE_FLAG_FLIP_BACKFACE", self.car)
        self.assertIn("CaptureFrontMaterial", self.car)
        self.assertIn("Polygon.uiTex & SURFACE_FLAG_BACK", self.car)
        self.assertIn("bCompleteReverseGeometry = true", self.batch)
        self.assertIn("bGenerateAllReverseSides", self.common)
        self.assertIn("bMirrorMaterialU", self.common)

    def test_outputs_are_organized_and_gltf_is_self_contained(self) -> None:
        self.assertIn('filePath("Tracks")', self.batch)
        self.assertIn('filePath("Cars")', self.batch)
        self.assertIn('sName + ".obj"', self.batch)
        self.assertIn('sName + ".glb"', self.batch)
        self.assertIn("Options.bBinary = true", self.batch)
        self.assertIn("TextureSource.PngBytes.assign", self.batch)

    def test_tracks_reuse_the_canonical_export_defaults(self) -> None:
        body = function_body(
            self.preview, "bool CTrackPreview::ExportToFolder("
        )
        self.assertIn("ExportGltf_Internal", body)
        self.assertIn("ExportObj_Internal", body)
        self.assertIn("true, true, true", body)
        self.assertIn("m_assets.ExportTextures", body)

    def test_car_geometry_comes_from_roller_plans_and_shared_emitter(self) -> None:
        self.assertIn("CarDesigns[uiDesign]", self.car)
        self.assertIn("ed_emit_surface", self.car)
        self.assertNotIn("RollerEd_", self.car)
        self.assertIn("sSingleObjectName", self.common)

    def test_new_sources_and_behavioral_test_are_registered(self) -> None:
        app_cmake = (EDITOR / "CMakeLists.txt").read_text(encoding="utf-8")
        root_cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
        for name in (
            "EditorBatchExporter.cpp",
            "EditorBatchExporter.h",
            "EditorCarModel.cpp",
            "EditorCarModel.h",
        ):
            self.assertIn(name, app_cmake)
        self.assertIn("trackeditor-batch-car-model-test", root_cmake)
        self.assertIn("tests/editor_car_model_test.cpp", root_cmake)


if __name__ == "__main__":
    unittest.main()
