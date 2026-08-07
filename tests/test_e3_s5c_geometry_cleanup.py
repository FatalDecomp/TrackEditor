"""E3-S5c: derived geometry is gone from the document types.

E3-S5c ended with two deliberate remainders: legacy CPU export geometry still
lived in WhipLib behind `CTrackGeometry`, and glm survived there under an exact
source allowlist. **Deleting WhipLib superseded both.** `CTrackGeometry`,
`ShapeFactory`, and the whole plan-data path went with it, and the reference-OBJ
importer -- the last glm consumer -- was rewritten to emit flat floats, so
`external/glm` is no longer vendored at all.

What survives from this story is the property it was protecting: the document
types own no derived geometry, and no vector-maths library is needed to express
them. The allowlist is therefore now an emptiness check.
"""

from __future__ import annotations

import pathlib
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[1]
TRACK_ASSETS = ROOT / "TrackAssets"
TRACK_MODEL = ROOT / "TrackModel"
EDITOR = ROOT / "TrackEditor"


def code_lines(text: str) -> str:
    """The source with // comments dropped."""
    return "\n".join(line.split("//", 1)[0] for line in text.splitlines())


class GeometryCleanupContractTests(unittest.TestCase):
    def test_track_has_no_derived_geometry_surface(self) -> None:
        # CTrack moved into TrackEditor/ with WhipLib's deletion.
        track_text = "\n".join(
            (EDITOR / filename).read_text(encoding="utf-8")
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

    def test_the_legacy_cpu_geometry_path_is_gone_entirely(self) -> None:
        # This used to assert that CTrackGeometry was private and derived on
        # demand inside WhipLib. WhipLib is deleted, so the stronger statement
        # holds: nothing in the tree derives CPU track geometry at all, and the
        # canonical ROLLER emitter is the only producer.
        self.assertFalse((ROOT / "WhipLib").exists())
        self.assertFalse((ROOT / "ModelExporter").exists())

        # Comments naming what was removed are how the removal is explained,
        # so this reads code rather than prose, and names the offending file
        # rather than dumping every source it scanned.
        gone = (
            "CTrackGeometry",
            "CShapeFactory",
            "CShapeData",
            "CObjExporter",
            "TextureMapping",
        )
        violations = []
        for directory in (TRACK_MODEL, TRACK_ASSETS, EDITOR):
            for path in directory.iterdir():
                if path.suffix not in {".h", ".cpp"}:
                    continue
                code = code_lines(path.read_text(encoding="utf-8", errors="ignore"))
                for token in gone:
                    if token in code:
                        violations.append(f"{path.relative_to(ROOT)}: {token}")
        self.assertEqual([], violations)

    def test_camera_and_physics_helpers_are_removed(self) -> None:
        preview = (EDITOR / "TrackPreview.cpp").read_text(encoding="utf-8")
        # MathHelpers moved into TrackEditor/ with WhipLib's deletion; it is
        # scalar angle normalization and nothing else.
        math_helpers = "\n".join(
            (EDITOR / filename).read_text(encoding="utf-8")
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

    def test_glm_is_no_longer_vendored_or_used(self) -> None:
        # Was an exact allowlist of the ten WhipLib files still using glm.
        self.assertFalse((ROOT / "external" / "glm").exists())

        glm_files = {
            path.relative_to(ROOT).as_posix()
            for directory in (TRACK_ASSETS, TRACK_MODEL, EDITOR)
            for path in directory.iterdir()
            if path.suffix in {".h", ".cpp"}
            and any(token in path.read_text(encoding="utf-8")
                    for token in ("glm::", "glm.hpp", "gtc/", "gtx/"))
        }
        self.assertEqual(set(), glm_files)

        build_text = "\n".join(
            path.read_text(encoding="utf-8")
            for path in ROOT.rglob("CMakeLists.txt")
            if path.relative_to(ROOT).parts[0] not in {"external", "out", "bin", "lib"}
        )
        self.assertNotIn("glm", build_text)


if __name__ == "__main__":
    unittest.main()
