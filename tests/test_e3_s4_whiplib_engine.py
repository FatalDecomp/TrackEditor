from __future__ import annotations

import pathlib
import re
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[1]
WHIPLIB = ROOT / "WhipLib"
TRACK_ASSETS = ROOT / "TrackAssets"


class E3S4WhipLibEngineContractTests(unittest.TestCase):
    def test_obsolete_engine_files_are_physically_deleted(self) -> None:
        obsolete_files = {
            "Camera.cpp",
            "Camera.h",
            "Clock.cpp",
            "Clock.h",
            "Component.h",
            "DriveComponent.cpp",
            "DriveComponent.h",
            "Entity.cpp",
            "Entity.h",
            "GameClock.cpp",
            "GameClock.h",
            "GameInput.cpp",
            "GameInput.h",
            "IndexBuffer.cpp",
            "IndexBuffer.h",
            "KeyMapper.h",
            "NoclipComponent.cpp",
            "NoclipComponent.h",
            "OpenGLDebug.h",
            "PhysicsComponent.cpp",
            "PhysicsComponent.h",
            "Renderer.cpp",
            "Renderer.h",
            "Scene.cpp",
            "Scene.h",
            "SceneManager.cpp",
            "SceneManager.h",
            "Shader.cpp",
            "Shader.h",
            "ShapeComponent.cpp",
            "ShapeComponent.h",
            "TrackComponent.cpp",
            "TrackComponent.h",
            "VertexArray.cpp",
            "VertexArray.h",
            "VertexBuffer.cpp",
            "VertexBuffer.h",
            "WinUserKeyMapper.cpp",
            "WinUserKeyMapper.h",
        }
        present = {path.name for path in WHIPLIB.iterdir() if path.is_file()}
        self.assertFalse(obsolete_files & present)
        self.assertFalse((WHIPLIB / "Shaders").exists())

    def test_build_and_source_tree_have_no_glew_or_opengl_dependency(self) -> None:
        audited_paths = [
            ROOT / "CMakeLists.txt",
            ROOT / "README.md",
            ROOT / "WhipLib" / "CMakeLists.txt",
            ROOT / "TrackEditor" / "CMakeLists.txt",
            ROOT / ".github" / "workflows" / "build.yml",
            ROOT / "docs" / "building.md",
            ROOT / "external" / "TrackEditor_LicensedLibs.txt",
            ROOT / "external" / "WhipLib_LicensedLibs.txt",
            ROOT / "ModelExporter" / "Main.cpp",
            ROOT / "ModelExporter" / "Makefile",
            ROOT / "ModelExporter" / "ModelExporter.vcxproj",
        ]
        audited_paths.extend(WHIPLIB.glob("*.h"))
        audited_paths.extend(WHIPLIB.glob("*.cpp"))
        audited_paths.extend((ROOT / "TrackEditor").glob("*.h"))
        audited_paths.extend((ROOT / "TrackEditor").glob("*.cpp"))

        forbidden = re.compile(
            r"glew|<GL/|find_package\s*\(\s*OpenGL|OpenGL::GL|Qt5::OpenGL",
            re.IGNORECASE,
        )
        violations = []
        for path in audited_paths:
            if forbidden.search(path.read_text(encoding="utf-8", errors="ignore")):
                violations.append(str(path.relative_to(ROOT)))
        self.assertEqual([], violations)

    def test_document_model_assets_and_cpu_export_geometry_remain(self) -> None:
        for filename in (
            "Track.cpp",
            "Track.h",
            "ShapeData.cpp",
            "ShapeData.h",
            "ShapeFactory.cpp",
            "ShapeFactory.h",
            "ObjImporter.cpp",
            "ObjImporter.h",
            "ObjExporter.cpp",
            "ObjExporter.h",
        ):
            self.assertTrue((WHIPLIB / filename).is_file(), filename)

        for filename in ("Palette.cpp", "Palette.h", "Texture.cpp", "Texture.h"):
            self.assertTrue((TRACK_ASSETS / filename).is_file(), filename)

        texture = (TRACK_ASSETS / "Texture.cpp").read_text(encoding="utf-8")
        shape_data = (WHIPLIB / "ShapeData.h").read_text(encoding="utf-8")
        self.assertIn("ProcessTextureData", texture)
        self.assertIn("ExportToPngFile", texture)
        self.assertIn("eShapePrimitive", shape_data)
        self.assertNotIn("CVertexBuffer", shape_data)


if __name__ == "__main__":
    unittest.main()
