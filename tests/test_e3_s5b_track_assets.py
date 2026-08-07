from __future__ import annotations

import pathlib
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[1]
TRACK_ASSETS = ROOT / "TrackAssets"
# CTrack moved into TrackEditor/ when WhipLib was deleted.
EDITOR = ROOT / "TrackEditor"


class TrackAssetsContractTests(unittest.TestCase):
    def test_track_assets_is_a_real_editor_domain_target(self) -> None:
        root_cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
        assets_cmake = (TRACK_ASSETS / "CMakeLists.txt").read_text(encoding="utf-8")
        # WhipLib was the consumer this used to check; with it deleted the
        # editor links track-assets directly.
        editor_cmake = (EDITOR / "CMakeLists.txt").read_text(encoding="utf-8")

        self.assertIn("add_subdirectory(TrackAssets)", root_cmake)
        self.assertIn("add_library(track-assets STATIC", assets_cmake)
        self.assertIn("TrackEditor::track-assets", assets_cmake)
        self.assertIn("TrackEditor::track-assets", editor_cmake)
        self.assertNotIn("Palette.cpp", editor_cmake)
        self.assertNotIn("Texture.cpp", editor_cmake)

    def test_palette_texture_and_ownership_moved_out_of_whiplib(self) -> None:
        for filename in (
            "CMakeLists.txt",
            "Palette.cpp",
            "Palette.h",
            "Texture.cpp",
            "Texture.h",
            "TrackAssets.cpp",
            "TrackAssets.h",
        ):
            self.assertTrue((TRACK_ASSETS / filename).is_file(), filename)

        for filename in ("Palette.cpp", "Palette.h", "Texture.cpp", "Texture.h"):
            self.assertFalse((EDITOR / filename).exists(), filename)

        track_header = (EDITOR / "Track.h").read_text(encoding="utf-8")
        track_source = (EDITOR / "Track.cpp").read_text(encoding="utf-8")
        self.assertIn("CTrackAssets m_assets", track_header)
        for obsolete in (
            "m_pPal",
            "m_pTex",
            "m_pBld",
            "m_sLastLoadedPal",
            "m_sLastLoadedTex",
            "m_sLastLoadedBld",
            "CTrack::LoadTextures",
        ):
            self.assertNotIn(obsolete, track_header + track_source)

        assets_header = (TRACK_ASSETS / "TrackAssets.h").read_text(encoding="utf-8")
        for cache in (
            "m_sLastLoadedPalette",
            "m_sLastLoadedTexture",
            "m_sLastLoadedBuilding",
        ):
            self.assertIn(cache, assets_header)

    def test_asset_target_has_no_renderer_or_geometry_ownership(self) -> None:
        assets_text = "\n".join(
            path.read_text(encoding="utf-8")
            for path in TRACK_ASSETS.iterdir()
            if path.suffix in {".h", ".cpp", ".txt"}
        )
        for forbidden in (
            "ROLLER::core",
            "Qt5::",
            "Qt6::",
            "Renderer",
            "glew",
            "<GL/",
            "OpenGL",
            "QOpenGL",
            "ShapeFactory",
            "ShapeData",
            "ObjExporter",
            "GenerateTrackMath",
            "tChunkMath",
            "glm",
            "tVertex",
            "GetTextureCoordinates",
            "GetColorCenterCoordinates",
        ):
            self.assertNotIn(forbidden, assets_text)

    def test_model_keeps_names_while_assets_resolve_and_export(self) -> None:
        model_header = (ROOT / "TrackModel" / "TrackModel.h").read_text(encoding="utf-8")
        assets_header = (TRACK_ASSETS / "TrackAssets.h").read_text(encoding="utf-8")
        assets_source = (TRACK_ASSETS / "TrackAssets.cpp").read_text(encoding="utf-8")
        preview_source = (ROOT / "TrackEditor" / "TrackPreview.cpp").read_text(
            encoding="utf-8"
        )

        self.assertIn("m_sTextureFile", model_header)
        self.assertIn("m_sBuildingFile", model_header)
        self.assertNotIn("CTexture", model_header)
        self.assertIn("LoadFromDocument", assets_header)
        self.assertIn('"PALETTE.PAL"', assets_source)
        self.assertIn("std::filesystem::path(sDocumentAssetRoot) / sFilename", assets_source)
        self.assertIn("ExportTextures", assets_header)
        self.assertIn("m_assets.ExportTextures", preview_source)


if __name__ == "__main__":
    unittest.main()
