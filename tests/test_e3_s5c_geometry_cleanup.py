from __future__ import annotations

import pathlib
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[1]
WHIPLIB = ROOT / "WhipLib"
TRACK_ASSETS = ROOT / "TrackAssets"
TRACK_MODEL = ROOT / "TrackModel"
EDITOR = ROOT / "TrackEditor"


class GeometryCleanupContractTests(unittest.TestCase):
    def test_track_has_no_derived_geometry_surface(self) -> None:
        track_text = "\n".join(
            (WHIPLIB / filename).read_text(encoding="utf-8")
            for filename in ("Track.h", "Track.cpp")
        )
        for obsolete in (
            "tChunkMath",
            "CChunkMathAy",
            "m_chunkMathAy",
            "GenerateTrackMath",
            "ResetStunts",
            "UpdateStunts",
            "UseCenterStunt",
            "CollideWithChunk",
            "ProjectToTrack",
            "glm",
        ):
            self.assertNotIn(obsolete, track_text)

    def test_model_and_assets_do_not_derive_geometry(self) -> None:
        domain_text = "\n".join(
            path.read_text(encoding="utf-8")
            for directory in (TRACK_MODEL, TRACK_ASSETS)
            for path in directory.iterdir()
            if path.suffix in {".h", ".cpp", ".txt"}
        )
        for forbidden in (
            "glm",
            "tVertex",
            "TrackGeometry",
            "GetTextureCoordinates",
            "GetColorCenterCoordinates",
        ):
            self.assertNotIn(forbidden, domain_text)

    def test_legacy_export_geometry_is_private_and_on_demand(self) -> None:
        cmake = (WHIPLIB / "CMakeLists.txt").read_text(encoding="utf-8")
        geometry = (WHIPLIB / "TrackGeometry.h").read_text(encoding="utf-8")
        shapes = (WHIPLIB / "ShapeFactory.cpp").read_text(encoding="utf-8")

        self.assertIn("TrackGeometry.cpp", cmake)
        self.assertIn("explicit CTrackGeometry(const CTrackModel &Track)", geometry)
        self.assertIn("const CTrackGeometry TrackGeometry(*pTrack)", shapes)
        self.assertIn("ObjExporter", "\n".join(
            path.read_text(encoding="utf-8")
            for path in WHIPLIB.glob("*.cpp")
        ))

    def test_camera_and_physics_helpers_are_removed(self) -> None:
        preview = (EDITOR / "TrackPreview.cpp").read_text(encoding="utf-8")
        math_helpers = "\n".join(
            (WHIPLIB / filename).read_text(encoding="utf-8")
            for filename in ("MathHelpers.h", "MathHelpers.cpp")
        )

        self.assertFalse((EDITOR / "TrackCoordinateConversion.h").exists())
        self.assertIn("m_header.iHeaderUnk1", preview)
        self.assertIn("m_header.iHeaderUnk2", preview)
        self.assertIn("m_header.iFloorDepth", preview)
        self.assertNotIn("glm", preview)
        self.assertIn("ConstrainAngle", math_helpers)
        for obsolete in (
            "RayCollisionTriangle",
            "ProjectPointOntoPlane",
            "GetProjectionPercentageAlongSegment",
            "glm",
        ):
            self.assertNotIn(obsolete, math_helpers)

        editor_text = "\n".join(
            path.read_text(encoding="utf-8")
            for path in EDITOR.iterdir()
            if path.suffix in {".h", ".cpp", ".ui"}
        )
        self.assertNotIn("ANIMATE_STUNTS", editor_text)
        self.assertNotIn("ckAnimateStunts", editor_text)

    def test_residual_glm_usage_is_audited(self) -> None:
        source_roots = (WHIPLIB, TRACK_ASSETS, TRACK_MODEL, EDITOR)
        glm_files = {
            path.relative_to(ROOT).as_posix()
            for directory in source_roots
            for path in directory.iterdir()
            if path.suffix in {".h", ".cpp"}
            and any(token in path.read_text(encoding="utf-8")
                    for token in ("glm::", "glm.hpp", "gtc/", "gtx/"))
        }
        self.assertEqual(
            glm_files,
            {
                "WhipLib/ObjImporter.cpp",
                "WhipLib/ShapeData.cpp",
                "WhipLib/ShapeData.h",
                "WhipLib/ShapeFactory.cpp",
                "WhipLib/ShapeFactory.h",
                "WhipLib/TextureMapping.cpp",
                "WhipLib/TextureMapping.h",
                "WhipLib/TrackGeometry.cpp",
                "WhipLib/TrackGeometry.h",
                "WhipLib/Vertex.h",
            },
        )


if __name__ == "__main__":
    unittest.main()
