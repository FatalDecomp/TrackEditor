import re
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
EDITOR = ROOT / "TrackEditor"
DOCS = ROOT / "docs"


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
    def test_the_pin_carries_the_geometry_buffer_api(self) -> None:
        # E4-S1 consumes E4A-S5's buffer API and adds nothing to ROLLER, so the
        # pin has to already publish it.
        api = (
            ROOT
            / "external"
            / "ROLLER"
            / "PROJECTS"
            / "ROLLER"
            / "editor_api.h"
        ).read_text(encoding="utf-8")
        self.assertIn("RollerEd_QueryGeometrySizes", api)
        self.assertIn("RollerEd_FillGeometry", api)
        for name in ("tEdVertex", "tEdPrimitive", "tEdMaterial"):
            self.assertIn(f"}} {name};", api)


class ExporterBoundaryTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.header = (EDITOR / "EditorObjExporter.h").read_text(
            encoding="utf-8"
        )
        cls.source = (EDITOR / "EditorObjExporter.cpp").read_text(
            encoding="utf-8"
        )
        # E4-S2 moved everything the exporters must agree on into the shared
        # layer, so the conventions are pinned there and the format-specific
        # assertions stay here.
        cls.common_header = (EDITOR / "EditorExportCommon.h").read_text(
            encoding="utf-8"
        )
        cls.common_source = (EDITOR / "EditorExportCommon.cpp").read_text(
            encoding="utf-8"
        )
        cls.combined = (
            without_comments(cls.header)
            + without_comments(cls.source)
            + without_comments(cls.common_header)
            + without_comments(cls.common_source)
        )

    def test_the_exporter_owns_no_qt_whiplib_or_facade_call(self) -> None:
        # It is a pure function of one extraction, which is what makes it
        # unit-testable without a worker, an event loop, or a loaded track.
        self.assertNotIn("RollerEd_", self.combined)
        self.assertNotIn("#include <Q", self.combined)
        self.assertNotIn('#include "Q', self.combined)
        for forbidden in ("ShapeData", "ShapeFactory", "CTrackGeometry",
                          "glm"):
            self.assertNotIn(forbidden, self.combined)

    def test_the_input_is_the_canonical_representation(self) -> None:
        self.assertIn("const tEdVertex *pVertices", self.common_header)
        self.assertIn("const tEdPrimitive *pPrimitives", self.common_header)
        self.assertIn("const tEdMaterial *pMaterials", self.common_header)

    def test_scope_is_filtered_on_the_published_content_class(self) -> None:
        # AD-6d/AD-6e: authored content only, and never inferred from the
        # surface class.
        body = function_body(
            self.common_source,
            "bool CEditorExportConventions::IsAuthoredContent(",
        )
        self.assertIn("ROLLER_ED_CONTENT_AUTHORED_TRACK", body)
        self.assertIn("ROLLER_ED_CONTENT_AUTHORED_SIGN", body)
        self.assertIn("ROLLER_ED_CONTENT_AUTHORED_SCENERY", body)
        self.assertNotIn("ROLLER_ED_CONTENT_RUNTIME_SCENERY", body)
        self.assertIn("IsAuthoredContent(Primitive.unContentClass)",
                      self.common_source)

    def test_every_pre_migration_surface_group_name_survives(self) -> None:
        for name in (
            "Center",
            "Left Shoulder",
            "Right Shoulder",
            "Left Wall",
            "Right Wall",
            "Roof",
            "Outer Wall Floor",
            "Left Lower Outer Wall",
            "Right Lower Outer Wall",
            "Left Upper Outer Wall",
            "Right Upper Outer Wall",
        ):
            self.assertIn(f'return "{name}";', self.common_source)
        self.assertIn('"Track"', self.common_source)
        self.assertIn('" (Back)"', self.common_source)
        self.assertIn('"Track (Back)"', self.common_source)

    def test_back_faces_use_the_back_material(self) -> None:
        # Reusing the front material's atlas rectangle would sample the wrong
        # tile whenever texture_back[] substitutes a different one (AD-7b).
        body = function_body(
            self.common_source,
            "uint32_t CEditorExportConventions::ReverseSideMaterial(",
        )
        self.assertIn("uiBackMaterialId", body)

        # A missing back material does not imply single-sidedness.
        has_back = function_body(
            self.common_source, "bool CEditorExportConventions::HasReverseSide("
        )
        self.assertIn("ROLLER_ED_PRIMITIVE_FLAG_TWO_SIDED", has_back)
        # OBJ cannot say "draw both sides", so it always needs real geometry.
        self.assertIn("Grouping.bReverseSideAsGeometry = true", self.source)

    def test_uvs_resolve_through_the_atlas_transform_only(self) -> None:
        stripped = without_comments(self.source)
        self.assertIn("Material.fAtlasScale[0]", stripped)
        self.assertIn("Material.fAtlasBias[0]", stripped)
        # No exporter-side tile arithmetic (AD-7b).
        self.assertNotIn("uiTileIndex", stripped)
        self.assertNotIn("uiTileSize", stripped)

    def test_the_conventions_are_shared_rather_than_re_derived(self) -> None:
        # The point of the migration: no exporter may derive its own axis,
        # scale, scope, or grouping. The format-specific file states none of
        # them.
        for owned_by_the_shared_layer in (
            "ED_EXPORT_UNIT_SCALE",
            "ROLLER_ED_CONTENT_AUTHORED_TRACK",
            "afOutXYZ",
            'return "Center";',
        ):
            self.assertNotIn(owned_by_the_shared_layer, self.source)
        self.assertIn("CEditorExportConventions::BuildObjects", self.source)

    def test_screen_darken_is_never_an_ordinary_texture(self) -> None:
        body = function_body(self.source, "void WriteMaterial(")
        darken = body[body.index("ROLLER_ED_MATERIAL_SCREEN_DARKEN"):]
        darken = darken[: darken.index("return;")]
        self.assertNotIn("map_Kd", darken)
        self.assertIn("d ", darken)
        self.assertIn("ScreenDarkenAlpha", darken)

    def test_the_axis_and_scale_conventions_are_stated_not_derived(
        self,
    ) -> None:
        self.assertIn("ED_EXPORT_UNIT_SCALE 0.01f", self.common_header)
        self.assertIn(
            "0003-canonical-geometry-conventions.md", self.common_header
        )
        body = function_body(
            self.common_source,
            "void CEditorExportConventions::ConvertPosition(",
        )
        # +Z up to +Y up, handedness preserved.
        self.assertIn("afOutXYZ[1] = afRollerXYZ[2]", body)
        self.assertIn("afOutXYZ[2] = -afRollerXYZ[1]", body)


class ExtractionPathTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.queue = (EDITOR / "EditorRenderQueue.h").read_text(
            encoding="utf-8"
        )
        cls.service_header = (EDITOR / "EditorRenderService.h").read_text(
            encoding="utf-8"
        )
        cls.service = (EDITOR / "EditorRenderService.cpp").read_text(
            encoding="utf-8"
        )
        cls.preview = (EDITOR / "TrackPreview.cpp").read_text(
            encoding="utf-8"
        )

    def test_extraction_is_a_queue_command_not_a_ui_facade_call(self) -> None:
        self.assertIn("EXTRACT_GEOMETRY", self.queue)
        self.assertIn("RollerEd_QueryGeometrySizes", self.service)
        self.assertIn("RollerEd_FillGeometry", self.service)
        # E3-S1's rule holds: UI code never calls the facade.
        self.assertNotIn("RollerEd_", without_comments(self.preview))

    def test_the_extraction_runs_on_the_worker_thread(self) -> None:
        body = function_body(
            self.service, "void ProcessExtractGeometry("
        )
        self.assertIn('AssertWorkerThread("RollerEd_QueryGeometrySizes for '
                      'export")', body)
        self.assertIn('AssertWorkerThread("RollerEd_FillGeometry")', body)

    def test_the_fill_is_epoch_checked_and_scene_gated(self) -> None:
        body = function_body(
            self.service, "void ProcessExtractGeometry("
        )
        self.assertIn("Sizes.uiSceneState != ROLLER_ED_SCENE_READY", body)
        # The fill is validated against the geometry epoch, never the
        # generation (AD-7d).
        self.assertIn("RollerEd_FillGeometry(\n        Sizes.uiGeometryEpoch",
                      body)
        self.assertNotIn("uiTrackGeneration,", body)

    def test_the_worker_publishes_a_copy_the_caller_owns(self) -> None:
        # RollerEd_FillGeometry lets no core-owned pointer escape, so the
        # snapshot is vectors the calling thread owns outright.
        self.assertIn("std::vector<tEdVertex> Vertices;", self.queue)
        self.assertIn("std::vector<tEdPrimitive> Primitives;", self.queue)
        self.assertIn("std::vector<tEdMaterial> Materials;", self.queue)

    def test_every_dropped_request_completes_its_waiter(self) -> None:
        # A blocked exporter that never wakes would hang the editor, so the
        # stop, invalidate, and skip paths all complete the slot.
        for signature in (
            "void InvalidateDocument(uint64_t ullDocumentId)",
            "void StopAndWait()",
        ):
            body = function_body(self.service, signature)
            self.assertIn("FailDroppedRequests", body)
        self.assertIn("CompleteExtraction(Request.Extraction", self.service)

    def test_extraction_moves_no_epoch_and_renders_nothing(self) -> None:
        body = function_body(
            self.service, "void ProcessExtractGeometry("
        )
        for forbidden in ("RollerEd_RenderFrame", "RollerEd_SetCamera",
                          "RollerEd_SetOverlayState", "RollerEd_LoadTrackFile",
                          "RollerEd_UnloadTrack"):
            self.assertNotIn(forbidden, body)


class ExportUiTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.preview = (EDITOR / "TrackPreview.cpp").read_text(
            encoding="utf-8"
        )
        cls.wizard = (EDITOR / "ExportWizard.cpp").read_text(encoding="utf-8")

    def test_obj_no_longer_derives_its_own_geometry(self) -> None:
        body = function_body(
            self.preview, "bool CTrackPreview::ExportObj_Internal("
        )
        self.assertIn("ExtractCanonicalGeometry", body)
        self.assertIn("CEditorObjExporter::ExportToFiles", body)
        # The extraction itself still goes through the render worker.
        extract = function_body(
            self.preview, "bool CTrackPreview::ExtractCanonicalGeometry("
        )
        self.assertIn("m_pRenderService->ExtractGeometry", extract)
        for forbidden in ("MakeTrackSurface", "MakeAILine", "MakeSigns",
                          "CShapeData", "CObjExporter"):
            self.assertNotIn(forbidden, body)

    def test_the_legacy_obj_track_writer_is_no_longer_reached(self) -> None:
        self.assertNotIn("CObjExporter::GetObjExporter().ExportTrack",
                         self.preview)

    def test_the_existing_export_options_are_preserved(self) -> None:
        # Separate sections and separate back faces still come from the
        # wizard's own checkboxes.
        self.assertIn("exportWizard.m_bExportSeparate", self.preview)
        self.assertIn("exportWizard.m_bExportBacks", self.preview)
        self.assertIn("bSeparateSections", self.preview)
        self.assertIn("bSeparateBackFaces", self.preview)

    def test_the_sign_option_reaches_the_obj_export(self) -> None:
        # E4A-S6. ROLLER now publishes a camera-independent scenery
        # traversal, so the checkbox is live and the exporter honours it.
        body = function_body(self.wizard, "CExportWizard::CExportWizard(")
        self.assertNotIn("setEnabled(false)", body)
        self.assertNotIn("m_bExportSigns = false", body)

        export = function_body(self.preview, "bool CTrackPreview::Export(")
        self.assertIn("exportWizard.m_bExportSigns", export)
        obj = function_body(self.preview,
                            "bool CTrackPreview::ExportObj_Internal(")
        self.assertIn("Options.bExportScenery = bExportScenery", obj)


class DocumentationTests(unittest.TestCase):
    def test_the_export_conventions_are_recorded_in_repo(self) -> None:
        # E4-S2 and E4-S3 state their conversions relative to this document
        # rather than re-deriving one empirically.
        doc = DOCS / "obj-export.md"
        self.assertTrue(doc.is_file(), f"missing {doc}")
        text = doc.read_text(encoding="utf-8")
        for heading in (
            "Coordinate system",
            "Scale",
            "Winding",
            "UV",
            "Materials",
            "Screen darkening",
            "Scope",
        ):
            self.assertIn(heading, text)


if __name__ == "__main__":
    unittest.main()
