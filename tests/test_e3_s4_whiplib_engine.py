"""E3-S4: the WhipLib engine and every GLEW/OpenGL dependency stay deleted.

E3-S4 deleted WhipLib's engine, shaders, and GLEW while *retaining* its CPU
model/assets/export path. **WhipLib has since been deleted outright**, along
with ModelExporter, its only remaining consumer, so the retention half of this
story no longer has a subject: what it protected either moved into `track-model`
/ `track-assets` / `TrackEditor` or went away with the CPU geometry path. See
`test_e3_s5c_geometry_cleanup.py`.

What survives, and is worth keeping enforced, is the boundary: nothing in the
source or build graph may reach GLEW, desktop OpenGL, or Qt OpenGL. E5-S2 guards
the link closure; this guards the source text.
"""

from __future__ import annotations

import pathlib
import re
import unittest

from pathlib import Path


ROOT = pathlib.Path(__file__).resolve().parents[1]
TRACK_ASSETS = ROOT / "TrackAssets"
TRACK_MODEL = ROOT / "TrackModel"
EDITOR = ROOT / "TrackEditor"


class E3S4WhipLibEngineContractTests(unittest.TestCase):
    def test_the_engine_and_its_library_are_physically_deleted(self) -> None:
        # The original list of 38 engine files is subsumed: the directory that
        # held them is gone, and so is the tool that kept the rest alive.
        self.assertFalse((ROOT / "WhipLib").exists())
        self.assertFalse((ROOT / "ModelExporter").exists())

    def test_build_and_source_tree_have_no_glew_or_opengl_dependency(self) -> None:
        audited_paths = [
            ROOT / "CMakeLists.txt",
            ROOT / "README.md",
            ROOT / "TrackEditor" / "CMakeLists.txt",
            ROOT / "TrackModel" / "CMakeLists.txt",
            ROOT / "TrackAssets" / "CMakeLists.txt",
            ROOT / "cmake" / "TrackEditorNoOpenGL.cmake",
            ROOT / ".github" / "workflows" / "build.yml",
            ROOT / "docs" / "building.md",
        ]
        for directory in (EDITOR, TRACK_MODEL, TRACK_ASSETS):
            audited_paths.extend(directory.glob("*.h"))
            audited_paths.extend(directory.glob("*.cpp"))

        # E5-S2's link-closure guard names these libraries in order to forbid
        # them, which this audit cannot tell apart from using them. It is the
        # single deliberate exception, exempted by name so that moving or
        # renaming it puts the file back under the audit.
        exempt = {Path("cmake") / "TrackEditorNoOpenGL.cmake"}

        forbidden = re.compile(
            r"glew|<GL/|find_package\s*\(\s*OpenGL|OpenGL::GL|Qt[56]::OpenGL",
            re.IGNORECASE,
        )
        violations = []
        for path in audited_paths:
            if not path.is_file():
                continue
            relative = path.relative_to(ROOT)
            if relative in exempt:
                self.assertTrue(path.is_file(), f"{relative} is exempt but missing")
                continue
            if forbidden.search(path.read_text(encoding="utf-8", errors="ignore")):
                violations.append(str(relative))
        self.assertEqual([], violations)

    def test_the_document_model_and_assets_survived_the_deletion(self) -> None:
        # The CPU model/assets path E3-S4 retained now lives in its own
        # targets; only the renderer-side geometry went away.
        for filename in ("TrackModel.cpp", "TrackModel.h", "Unmangler.cpp", "Logging.cpp"):
            self.assertTrue((TRACK_MODEL / filename).is_file(), filename)
        for filename in ("Palette.cpp", "Palette.h", "Texture.cpp", "Texture.h"):
            self.assertTrue((TRACK_ASSETS / filename).is_file(), filename)
        for filename in ("Track.cpp", "Track.h", "EditorObjImporter.cpp"):
            self.assertTrue((EDITOR / filename).is_file(), filename)

        texture = (TRACK_ASSETS / "Texture.cpp").read_text(encoding="utf-8")
        self.assertIn("ProcessTextureData", texture)
        self.assertIn("ExportToPngFile", texture)


if __name__ == "__main__":
    unittest.main()
