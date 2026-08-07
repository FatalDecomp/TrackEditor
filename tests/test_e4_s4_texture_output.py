"""E4-S4: texture output that the canonical UVs can actually address.

The story reads "main and sign texture PNGs, sourced from track-assets
(E3-S5b); parity with current texture export". Sourcing was already true. What
was not true is that the exported PNG matched the atlas the canonical UVs are
expressed against: ROLLER resolves a tile as row = index >> 2, col = index & 3
with a 256-pixel stride (polytex.c), and publishes fAtlasScale/fAtlasBias
against a 256-wide atlas, while CTexture wrote a TILE_WIDTH-wide single column
with each tile transposed and flipped.

Those disagreed the moment E4-S1 switched the exporters' UV source from
WhipLib's own mapping to ROLLER's. E4-S4 makes the PNG match, which
deliberately breaks byte-parity with pre-E4-S4 exports.
"""

import re
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
ASSETS = ROOT / "TrackAssets"
EDITOR = ROOT / "TrackEditor"
DOCS = ROOT / "docs"
ROLLER = ROOT / "external" / "ROLLER" / "PROJECTS" / "ROLLER"


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


class PinnedCoreAtlasTests(unittest.TestCase):
    """The exported layout is only correct relative to the pinned ROLLER."""

    def test_roller_still_describes_a_256_wide_four_per_row_atlas(self) -> None:
        source = (ROLLER / "drawtrk3.c").read_text(encoding="utf-8")
        body = function_body(source, "bool drawtrk3_editor_texture_atlas(")
        # If either of these changes, the exported PNG has to change with it.
        self.assertIn("uiTilesPerRow = 256u / uiTileSize", body)
        self.assertIn("pAtlas->uiWidth = 256u", body)
        self.assertIn("pAtlas->uiHeight = uiAtlasRows * uiTileSize", body)

    def test_the_editor_atlas_width_matches_rollers(self) -> None:
        header = (ASSETS / "Texture.h").read_text(encoding="utf-8")
        self.assertIn("#define EXPORT_ATLAS_WIDTH 256", header)


class ExportAtlasTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.header = (ASSETS / "Texture.h").read_text(encoding="utf-8")
        cls.source = (ASSETS / "Texture.cpp").read_text(encoding="utf-8")

    def test_the_png_is_written_from_the_canonical_atlas(self) -> None:
        body = function_body(self.source, "bool CTexture::ExportToPngFile(")
        self.assertIn("GenerateExportAtlas", body)
        # The legacy column bitmap must not reach the PNG any more.
        self.assertNotIn("GenerateBitmapData", body)
        self.assertIn("stbi_write_png", body)

    def test_the_atlas_covers_content_tiles_only(self) -> None:
        # ROLLER's uiTileCount excludes the synthetic palette and transparency
        # tiles the editor appends, and the heights have to agree.
        # Comments are stripped: explaining why m_iNumTiles is wrong here is
        # not the same as using it.
        body = without_comments(
            function_body(self.source,
                          "std::uint8_t *CTexture::GenerateExportAtlas(")
        )
        self.assertIn("GetNumTiles()", body)
        self.assertNotIn("m_iNumTiles", body)
        height = without_comments(
            function_body(self.source, "int CTexture::GetExportAtlasHeight(")
        )
        self.assertIn("GetNumTiles()", height)
        self.assertNotIn("m_iNumTiles", height)

    def test_tiles_are_laid_out_row_major_four_per_row(self) -> None:
        body = function_body(self.source, "std::uint8_t *CTexture::GenerateExportAtlas(")
        self.assertIn("(i % iTilesPerRow) * TILE_WIDTH", body)
        self.assertIn("(i / iTilesPerRow) * TILE_HEIGHT", body)
        per_row = function_body(self.source, "int CTexture::GetExportTilesPerRow(")
        self.assertIn("EXPORT_ATLAS_WIDTH / TILE_WIDTH", per_row)

    def test_the_export_neither_transposes_nor_flips(self) -> None:
        # The legacy bitmap walks x outermost into a row-major buffer, which
        # transposes each tile, and runs FlipTileLines on top. Both were
        # invisible while WhipLib computed matching UVs; neither is correct for
        # a canonical UV.
        body = function_body(self.source, "std::uint8_t *CTexture::GenerateExportAtlas(")
        self.assertNotIn("FlipTileLines", body)
        # Row index outermost, and the sample is data[column][row].
        self.assertIn("for (int y = 0; y < TILE_HEIGHT; ++y)", body)
        self.assertIn("m_pTileAy[i].data[x][y]", body)

    def test_padding_slots_are_transparent(self) -> None:
        body = function_body(self.source, "std::uint8_t *CTexture::GenerateExportAtlas(")
        self.assertIn("memset", body)

    def test_the_legacy_column_bitmap_is_untouched(self) -> None:
        # WhipLib's C API publishes it and WhipLib::TextureMapping computes UVs
        # against it, so changing its shape would be an unrelated break.
        body = function_body(self.source, "std::uint8_t *CTexture::GenerateBitmapData(")
        self.assertIn("m_iNumTiles", body)
        self.assertIn("FlipTileLines", body)
        self.assertIn("TILE_WIDTH", body)
        mapping = (ROOT / "WhipLib" / "TextureMapping.cpp").read_text(
            encoding="utf-8"
        )
        self.assertIn("GetAtlasTileCount", mapping)

    def test_track_assets_stays_the_source_of_the_pngs(self) -> None:
        # E3-S5b owns palette and texture decoding; the exporters must not
        # grow their own.
        assets = (ASSETS / "TrackAssets.cpp").read_text(encoding="utf-8")
        self.assertIn("ExportToPngFile", assets)
        preview = (EDITOR / "TrackPreview.cpp").read_text(encoding="utf-8")
        self.assertEqual(2, preview.count("m_assets.ExportTextures"))
        for exporter in ("EditorObjExporter.cpp", "EditorGltfExporter.cpp",
                         "EditorExportCommon.cpp"):
            text = without_comments(
                (EDITOR / exporter).read_text(encoding="utf-8")
            )
            self.assertNotIn("stbi_write", text)
            self.assertNotIn("CTexture", text)

    def test_the_atlas_writer_owns_no_qt_or_roller_dependency(self) -> None:
        combined = without_comments(self.header) + without_comments(self.source)
        self.assertNotIn("RollerEd_", combined)
        self.assertNotIn("#include <Q", combined)


class DocumentationTests(unittest.TestCase):
    def test_the_layout_and_the_parity_break_are_recorded(self) -> None:
        doc = DOCS / "texture-export.md"
        self.assertTrue(doc.is_file(), f"missing {doc}")
        text = doc.read_text(encoding="utf-8")
        for heading in ("Atlas layout", "Orientation", "Which tiles",
                        "parity"):
            self.assertIn(heading, text)

    def test_the_exporter_docs_point_at_the_atlas_layout(self) -> None:
        for name in ("obj-export.md", "gltf-export.md"):
            text = (DOCS / name).read_text(encoding="utf-8")
            self.assertIn("texture-export.md", text)


if __name__ == "__main__":
    unittest.main()
