import re
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
EDITOR = ROOT / "TrackEditor"
EXTERNAL = ROOT / "external"
DOCS = ROOT / "docs"

# cgltf is vendored, not fetched at build time, so the pin is a fact about the
# tree. Re-vendor a newer tag rather than editing the headers in place.
CGLTF_VERSION = "1.15"
CGLTF_COMMIT = "360db1a95480fe102ae9c69b27c5d101167ff5ba"


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


class VendoredCgltfTests(unittest.TestCase):
    def test_both_headers_are_vendored(self) -> None:
        # The writer is a separate header from the parser, and the story needs
        # both: cgltf.h alone cannot serialize a document.
        for name in ("cgltf.h", "cgltf_write.h", "LICENSE", "cgltf_impl.c",
                     "CMakeLists.txt"):
            self.assertTrue((EXTERNAL / "cgltf" / name).is_file(),
                            f"missing external/cgltf/{name}")

    def test_the_vendored_version_is_pinned_and_recorded(self) -> None:
        header = (EXTERNAL / "cgltf" / "cgltf.h").read_text(encoding="utf-8")
        self.assertIn(f"Version: {CGLTF_VERSION}", header)
        impl = (EXTERNAL / "cgltf" / "cgltf_impl.c").read_text(
            encoding="utf-8"
        )
        self.assertIn(CGLTF_COMMIT, impl)
        cmake = (EXTERNAL / "cgltf" / "CMakeLists.txt").read_text(
            encoding="utf-8"
        )
        self.assertIn(CGLTF_COMMIT, cmake)

    def test_one_translation_unit_instantiates_the_headers(self) -> None:
        impl = (EXTERNAL / "cgltf" / "cgltf_impl.c").read_text(
            encoding="utf-8"
        )
        self.assertIn("#define CGLTF_IMPLEMENTATION", impl)
        self.assertIn("#define CGLTF_WRITE_IMPLEMENTATION", impl)
        # cgltf.h's declarations are include-guarded but its implementation is
        # not, and cgltf_write.h includes cgltf.h again.
        self.assertIn("#undef CGLTF_IMPLEMENTATION", impl)

    def test_the_licence_is_attributed(self) -> None:
        attribution = (EXTERNAL / "TrackEditor_LicensedLibs.txt").read_text(
            encoding="utf-8"
        )
        self.assertIn("cgltf", attribution.lower())


class ExporterBoundaryTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.header = (EDITOR / "EditorGltfExporter.h").read_text(
            encoding="utf-8"
        )
        cls.source = (EDITOR / "EditorGltfExporter.cpp").read_text(
            encoding="utf-8"
        )
        cls.common_source = (EDITOR / "EditorExportCommon.cpp").read_text(
            encoding="utf-8"
        )
        cls.combined = without_comments(cls.header) + without_comments(
            cls.source
        )

    def test_the_exporter_owns_no_qt_whiplib_or_facade_call(self) -> None:
        self.assertNotIn("RollerEd_", self.combined)
        self.assertNotIn("#include <Q", self.combined)
        self.assertNotIn('#include "Q', self.combined)
        for forbidden in ("ShapeData", "ShapeFactory", "CTrackGeometry",
                          "glm"):
            self.assertNotIn(forbidden, self.combined)

    def test_the_conventions_are_shared_with_the_obj_exporter(self) -> None:
        # AD-6a and the point of the migration: the exporters must not each
        # derive their own axis, scale, scope, or grouping.
        self.assertIn('#include "EditorExportCommon.h"', self.header)
        for owned_by_the_shared_layer in (
            "ED_EXPORT_UNIT_SCALE",
            "ROLLER_ED_CONTENT_AUTHORED_TRACK",
            'return "Center";',
        ):
            self.assertNotIn(owned_by_the_shared_layer, self.source)
        self.assertIn("CEditorExportConventions::BuildObjects", self.source)
        self.assertIn("CEditorExportConventions::ConvertPosition",
                      self.source)
        self.assertIn("CEditorExportConventions::ScreenDarkenAlpha",
                      self.source)

    def test_glTF_uses_a_double_sided_material_not_duplicated_geometry(
        self,
    ) -> None:
        # glTF can say "draw both sides" on the material, so unlike OBJ a
        # merely two-sided surface needs no reverse-wound copy. A different
        # back tile still does, because one material cannot address two.
        body = function_body(self.source, "bool IsDoubleSidedEntry(")
        self.assertIn("HasDistinctReverseMaterial", body)
        self.assertIn("HasReverseSide", body)
        self.assertIn(
            "Grouping.bReverseSideAsGeometry = !Options.bDoubleSidedMaterials",
            self.source,
        )
        # The shared layer is what honours that request.
        self.assertIn("Grouping.bReverseSideAsGeometry && HasReverseSide",
                      self.common_source)

    def test_uvs_are_not_flipped_and_do_no_tile_arithmetic(self) -> None:
        # glTF's UV origin is top-left, the same as ROLLER's, so unlike OBJ
        # there is no V flip.
        stripped = without_comments(self.source)
        self.assertIn("Material.fAtlasScale[0]", stripped)
        self.assertIn("Material.fAtlasBias[0]", stripped)
        self.assertNotIn("1.0f - fV", stripped)
        self.assertNotIn("uiTileIndex", stripped)
        self.assertNotIn("uiTileSize", stripped)

    def test_flat_colours_are_converted_out_of_srgb(self) -> None:
        # baseColorFactor is linear; the palette is sRGB.
        self.assertIn("SrgbToLinear", self.header)
        body = function_body(self.source, "float CEditorGltfExporter::SrgbToLinear(")
        self.assertIn("0.04045f", body)
        self.assertIn("2.4f", body)

    def test_screen_darken_is_never_an_ordinary_texture(self) -> None:
        stripped = without_comments(self.source)
        # Anchor on the material-building branch, not on the naming helper
        # that also mentions the kind.
        darken = stripped[
            stripped.index("Source.uiKind == ROLLER_ED_MATERIAL_SCREEN_DARKEN") :
        ]
        darken = darken[: darken.index("} else {")]
        self.assertIn("ScreenDarkenAlpha", darken)
        self.assertNotIn("base_color_texture", darken)
        # The mode itself comes from the one place that decides it.
        alpha = function_body(self.source, "cgltf_alpha_mode AlphaModeFor(")
        self.assertIn("ROLLER_ED_MATERIAL_SCREEN_DARKEN", alpha)
        self.assertIn("cgltf_alpha_mode_blend", alpha)

    def test_alpha_mode_follows_the_material_flag(self) -> None:
        # The atlas alpha is binary, so a transparent textured surface is a
        # cut-out rather than a blend.
        alpha = function_body(self.source, "cgltf_alpha_mode AlphaModeFor(")
        self.assertIn("ROLLER_ED_MATERIAL_FLAG_ALPHA_BLEND", alpha)
        self.assertIn("cgltf_alpha_mode_mask", alpha)
        self.assertIn("cgltf_alpha_mode_opaque", alpha)

    def test_atlas_tiles_collapse_to_one_material_per_identity(self) -> None:
        # The canonical table interns one material per atlas tile, but the
        # exported UVs are already atlas space, so 92 of them would arrive as
        # 92 identically named duplicates for an importer to rename.
        self.assertIn("std::map<std::string, size_t> MaterialKeys",
                      self.source)
        # The name is what makes that identity sound, so it has to distinguish
        # the cases that must stay apart.
        name = function_body(
            self.source, "std::string CEditorGltfExporter::MaterialName("
        )
        self.assertIn('"_cutout"', name)
        self.assertIn('"_two_sided"', name)
        self.assertIn("AlphaModeFor(Material)", name)

    def test_position_accessors_carry_the_bounds_glTF_requires(self) -> None:
        self.assertIn("Position.has_min = 1", self.source)
        self.assertIn("Position.has_max = 1", self.source)

    def test_the_binary_layout_is_four_byte_aligned(self) -> None:
        self.assertIn("g_uiGltfAlignment = 4", self.source)
        body = function_body(self.source, "size_t AlignUp(")
        self.assertIn("g_uiGltfAlignment", body)

    def test_the_glb_container_is_packed_explicitly(self) -> None:
        body = function_body(self.source, "void PackGlb(")
        self.assertIn("0x46546c67u", body)  # "glTF"
        self.assertIn("0x4e4f534au", body)  # "JSON"
        self.assertIn("0x004e4942u", body)  # "BIN\0"
        # The JSON chunk pads with spaces and the binary chunk with zeros, as
        # the glTF specification requires.
        self.assertIn("' '", body)

    def test_a_glb_is_self_contained_and_a_gltf_is_not(self) -> None:
        stripped = without_comments(self.source)
        # A GLB's buffer is its binary chunk, so it carries no uri.
        self.assertIn("if (!Options.bBinary)\n    Buffers[0].uri", stripped)
        # A GLB embeds its images; a .gltf points at the PNGs beside it.
        self.assertIn("const bool bEmbedImages = Options.bBinary", stripped)
        self.assertIn('Names.Add("image/png")', stripped)


class ExportUiTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.preview_header = (EDITOR / "TrackPreview.h").read_text(
            encoding="utf-8"
        )
        cls.preview = (EDITOR / "TrackPreview.cpp").read_text(
            encoding="utf-8"
        )
        cls.window = (EDITOR / "MainWindow.cpp").read_text(encoding="utf-8")
        cls.window_ui = (EDITOR / "MainWindow.ui").read_text(encoding="utf-8")
        cls.wizard = (EDITOR / "ExportWizard.cpp").read_text(encoding="utf-8")

    def test_gltf_is_offered_alongside_obj(self) -> None:
        self.assertIn("EXPORT_GLTF", self.preview_header)
        self.assertIn("actExportGLTF", self.window_ui)
        self.assertIn('<addaction name="actExportGLTF"/>', self.window_ui)
        self.assertIn("CMainWindow::OnExportGLTF", self.window)
        self.assertIn("actExportGLTF->setEnabled(bCanExport)", self.window)

    def test_the_container_follows_the_chosen_extension(self) -> None:
        body = function_body(
            self.preview, "bool CTrackPreview::ExportGltf_Internal("
        )
        self.assertIn('suffix().compare("glb"', body)
        self.assertIn("Options.bBinary = bBinary", body)
        self.assertIn("glTF Binary (*.glb)", self.preview)
        self.assertIn("glTF JSON (*.gltf)", self.preview)

    def test_a_glb_leaves_no_loose_png_behind(self) -> None:
        # A .glb embeds its images, so the PNGs are a build artifact rather
        # than part of the deliverable.
        body = function_body(
            self.preview, "bool CTrackPreview::ExportGltf_Internal("
        )
        self.assertIn("QTemporaryDir", body)
        self.assertIn("PngBytes", body)
        self.assertIn("Source.sUri", body)

    def test_both_canonical_exporters_share_the_extraction(self) -> None:
        for signature in (
            "bool CTrackPreview::ExportObj_Internal(",
            "bool CTrackPreview::ExportGltf_Internal(",
        ):
            body = function_body(self.preview, signature)
            self.assertIn("ExtractCanonicalGeometry", body)
        # E3-S1's rule still holds: UI code never calls the facade.
        self.assertNotIn("RollerEd_", without_comments(self.preview))

    def test_the_existing_export_options_are_preserved(self) -> None:
        self.assertIn("exportWizard.m_bExportSeparate", self.preview)
        self.assertIn("exportWizard.m_bExportBacks", self.preview)

    def test_signs_are_disabled_for_gltf_as_well(self) -> None:
        body = function_body(self.wizard, "CExportWizard::CExportWizard(")
        self.assertIn("eExportType::EXPORT_GLTF", body)
        self.assertIn("ckSigns->setEnabled(false)", body)


class DocumentationTests(unittest.TestCase):
    def test_every_decision_the_story_lists_is_recorded_in_repo(self) -> None:
        # E4-S2's acceptance criterion includes documenting each of these.
        doc = DOCS / "gltf-export.md"
        self.assertTrue(doc.is_file(), f"missing {doc}")
        text = doc.read_text(encoding="utf-8")
        for heading in (
            "Coordinate system",
            "Units",
            "Winding",
            "Normals",
            "Alpha mode",
            "double-sided",
            "AI",
            "Textures",
            "Output",
            "Scope",
        ):
            self.assertIn(heading, text)


if __name__ == "__main__":
    unittest.main()
