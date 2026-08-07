"""E4-S6: reference-model import.

The story reads "keep OpenReferenceModel / ObjImporter; display is now
provided by E3A-S7, not by WhipLib. AC: importing a reference OBJ displays it
correctly alongside the track."

Both were already kept and E3A-S7 already draws the mesh, so the work was in
the word "correctly". Two things were wrong:

  * The importer copied the file's axes straight into tEdReferenceVertex. A
    reference mesh is ROLLER world space (AD-13 inherits ADR 0003, +Z up) and
    an OBJ is +Y up, so every reference model loaded lying on its side.
  * CEditorReferenceMesh always claimed ROLLER_ED_REFERENCE_HAS_NORMALS, even
    for an OBJ with no vn lines, whose normals are all zero. AD-13 says the
    core generates them when the flag is clear.
"""

import re
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
EDITOR = ROOT / "TrackEditor"
WHIPLIB = ROOT / "WhipLib"
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


class KeptPathTests(unittest.TestCase):
    def test_the_import_entry_points_are_still_the_editors_own(self) -> None:
        # "Keep OpenReferenceModel / ObjImporter."
        for path in (WHIPLIB / "ObjImporter.cpp", WHIPLIB / "ObjImporter.h"):
            self.assertTrue(path.is_file(), f"{path} should still exist")
        preview = (EDITOR / "TrackPreview.cpp").read_text(encoding="utf-8")
        self.assertIn("void CTrackPreview::OpenReferenceModel(", preview)
        self.assertIn("CObjImporter::GetObjImporter().ImportObj", preview)

    def test_display_goes_through_the_e3a_s7_reference_mesh(self) -> None:
        # "Display is now provided by E3A-S7, not by WhipLib": the imported
        # shape is converted to the facade's vertex and handed to the mesh,
        # never drawn by WhipLib.
        body = function_body(
            (EDITOR / "TrackPreview.cpp").read_text(encoding="utf-8"),
            "void CTrackPreview::OpenReferenceModel(",
        )
        self.assertIn("tEdReferenceVertex", body)
        self.assertIn("m_ReferenceMesh.SetGeometry", body)
        self.assertIn("ScheduleReferenceMeshUpload", body)


class ConversionTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.common_header = (EDITOR / "EditorExportCommon.h").read_text(
            encoding="utf-8"
        )
        cls.common_source = (EDITOR / "EditorExportCommon.cpp").read_text(
            encoding="utf-8"
        )
        cls.preview = (EDITOR / "TrackPreview.cpp").read_text(encoding="utf-8")
        cls.importer = (WHIPLIB / "ObjImporter.cpp").read_text(
            encoding="utf-8"
        )

    def test_the_import_conversion_lives_beside_its_export_inverse(
        self,
    ) -> None:
        # One mapping in one place is what stops the two halves drifting.
        self.assertIn("ImportPosition", self.common_header)
        self.assertIn("ImportDirection", self.common_header)

    def test_the_import_is_the_algebraic_inverse_of_the_export(self) -> None:
        export = function_body(
            self.common_source,
            "void CEditorExportConventions::ConvertPosition(",
        )
        imp = function_body(
            self.common_source,
            "void CEditorExportConventions::ImportPosition(",
        )
        # export: out = (x, z, -y) * S
        self.assertIn("afOutXYZ[1] = afRollerXYZ[2]", export)
        self.assertIn("afOutXYZ[2] = -afRollerXYZ[1]", export)
        # import: roller = (x, -z, y) / S
        self.assertIn("afRollerXYZ[1] = -afFileXYZ[2]", imp)
        self.assertIn("afRollerXYZ[2] = afFileXYZ[1]", imp)
        # Both sides use the one scale constant rather than a literal.
        self.assertIn("ED_EXPORT_UNIT_SCALE", export)
        self.assertIn("ED_EXPORT_UNIT_SCALE", imp)

    def test_the_scale_is_not_applied_twice(self) -> None:
        # The importer used to multiply by 100 itself, which would compound
        # with the conversion's own division.
        stripped = without_comments(self.importer)
        self.assertNotIn("100.0", stripped)
        body = function_body(self.preview, "void CTrackPreview::OpenReferenceModel(")
        self.assertIn("CEditorExportConventions::ImportPosition", body)
        self.assertIn("CEditorExportConventions::ImportDirection", body)

    def test_positions_are_not_copied_axis_for_axis(self) -> None:
        # The straight copy is what laid every reference model on its side.
        body = function_body(self.preview, "void CTrackPreview::OpenReferenceModel(")
        self.assertNotIn("Target.fPosition[1] = Source.position.y", body)
        self.assertNotIn("Target.fNormal[1] = Source.normal.y", body)


class NormalsFlagTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.header = (EDITOR / "EditorReferenceMesh.h").read_text(
            encoding="utf-8"
        )
        cls.source = (EDITOR / "EditorReferenceMesh.cpp").read_text(
            encoding="utf-8"
        )

    def test_has_normals_follows_the_file(self) -> None:
        body = function_body(self.source, "tEdReferenceMesh CEditorReferenceMesh::GetMesh(")
        self.assertIn("m_bHasNormals ? ROLLER_ED_REFERENCE_HAS_NORMALS : 0u",
                      body)

    def test_the_flag_is_derived_from_the_geometry_not_the_caller(
        self,
    ) -> None:
        body = function_body(
            self.source, "void CEditorReferenceMesh::SetGeometry("
        )
        self.assertIn("m_bHasNormals = false", body)
        self.assertIn("fNormal[0] != 0.0f", body)
        # Clearing must not leave a stale answer behind.
        clear = function_body(self.source, "void CEditorReferenceMesh::Clear(")
        self.assertIn("m_bHasNormals = false", clear)

    def test_the_reference_mesh_stays_unit_testable(self) -> None:
        combined = without_comments(self.header) + without_comments(self.source)
        self.assertNotIn("RollerEd_", combined)
        self.assertNotIn("#include <Q", combined)


class DocumentationTests(unittest.TestCase):
    def test_the_import_conventions_are_recorded(self) -> None:
        doc = DOCS / "reference-model.md"
        self.assertTrue(doc.is_file(), f"missing {doc}")
        text = doc.read_text(encoding="utf-8")
        for heading in ("Coordinate", "Normals", "round trip", "Units"):
            self.assertIn(heading, text)


if __name__ == "__main__":
    unittest.main()
