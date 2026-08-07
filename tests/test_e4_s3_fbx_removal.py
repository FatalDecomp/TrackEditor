"""E4-S3: FBX export was removed outright rather than retargeted.

Spec open item 8 called FBX "optional/off-by-default; droppable once glTF
lands". glTF landed in E4-S2, and the maintainer chose to drop it. These tests
pin the removal so it cannot creep back as a half-wired option, and so that a
future exporter is added through the canonical path rather than by reviving the
legacy WhipLib one.
"""

import re
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
EDITOR = ROOT / "TrackEditor"
WHIPLIB = ROOT / "WhipLib"
DOCS = ROOT / "docs"


def without_comments(source: str) -> str:
    source = re.sub(r"/\*.*?\*/", "", source, flags=re.DOTALL)
    return re.sub(r"//.*", "", source)


def source_and_build_files() -> list[Path]:
    paths = [
        ROOT / "CMakeLists.txt",
        ROOT / "README.md",
        ROOT / "WhipLib" / "CMakeLists.txt",
        ROOT / "TrackEditor" / "CMakeLists.txt",
        ROOT / ".github" / "workflows" / "build.yml",
        ROOT / "docs" / "building.md",
        ROOT / "external" / "TrackEditor_LicensedLibs.txt",
        ROOT / "external" / "WhipLib_LicensedLibs.txt",
    ]
    # ModelExporter is retired from the CMake build but its sources are still
    # tracked, and it used to #include the header this story deleted.
    paths.extend(
        (ROOT / "ModelExporter" / name)
        for name in ("Main.cpp", "Makefile", "ModelExporter.vcxproj")
    )
    paths.extend(WHIPLIB.glob("*.h"))
    paths.extend(WHIPLIB.glob("*.cpp"))
    paths.extend(EDITOR.glob("*.h"))
    paths.extend(EDITOR.glob("*.cpp"))
    paths.extend(EDITOR.glob("*.ui"))
    return [p for p in paths if p.is_file()]


class RemovalTests(unittest.TestCase):
    def test_the_exporter_and_its_build_module_are_gone(self) -> None:
        for path in (
            WHIPLIB / "FBXExporter.cpp",
            WHIPLIB / "FBXExporter.h",
            ROOT / "cmake" / "TrackEditorFBX.cmake",
        ):
            self.assertFalse(path.exists(), f"{path} should have been removed")

    def test_no_source_or_build_file_mentions_fbx(self) -> None:
        # Comments that record *why* FBX is gone are the only allowed mention,
        # so this matches identifiers and build settings rather than prose.
        forbidden = re.compile(
            r"TRACKEDITOR_ENABLE_FBX|TRACKEDITOR_FBX_SDK_ROOT|FBXExporter"
            r"|CFBXExporter|EXPORT_FBX|actExportFBX|OnExportFBX|fbxsdk",
            re.IGNORECASE,
        )
        violations = []
        for path in source_and_build_files():
            if forbidden.search(path.read_text(encoding="utf-8", errors="ignore")):
                violations.append(str(path.relative_to(ROOT)))
        self.assertEqual([], violations)

    def test_the_retired_model_exporter_no_longer_includes_a_deleted_header(
        self,
    ) -> None:
        # It is not in the CMake build, so nothing would have caught the
        # dangling include; it writes OBJ only now.
        main = (ROOT / "ModelExporter" / "Main.cpp").read_text(
            encoding="utf-8"
        )
        self.assertNotIn('#include "FBXExporter.h"', main)
        self.assertIn("CObjExporter", main)
        makefile = (ROOT / "ModelExporter" / "Makefile").read_text(
            encoding="utf-8"
        )
        self.assertNotIn("external/FBX", makefile)

    def test_the_option_cannot_be_configured_back_on(self) -> None:
        # A leftover option() would let a build turn on a feature whose source
        # no longer exists.
        cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
        self.assertNotIn("FBX", cmake)
        self.assertFalse((ROOT / "cmake").exists())

    def test_ci_configures_without_an_fbx_flag(self) -> None:
        workflow = (ROOT / ".github" / "workflows" / "build.yml").read_text(
            encoding="utf-8"
        )
        self.assertNotIn("FBX", workflow)
        # The Windows step used a backtick continuation for the removed flag.
        self.assertNotIn("-A x64 `", workflow)


class RemainingExportPathTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        # E4-S5 moved eExportType into the format table.
        cls.formats_header = (EDITOR / "EditorExportFormat.h").read_text(
            encoding="utf-8"
        )
        cls.preview = (EDITOR / "TrackPreview.cpp").read_text(encoding="utf-8")
        cls.wizard = (EDITOR / "ExportWizard.cpp").read_text(encoding="utf-8")

    def test_every_remaining_format_is_canonical(self) -> None:
        # eExportType now holds only formats that read ROLLER's geometry.
        enum_body = self.formats_header[
            self.formats_header.index("enum eExportType") :
        ]
        enum_body = enum_body[: enum_body.index("};")]
        self.assertIn("EXPORT_OBJ", enum_body)
        self.assertIn("EXPORT_GLTF", enum_body)
        self.assertNotIn("FBX", enum_body)

    def test_the_legacy_cpu_geometry_no_longer_reaches_an_exporter(
        self,
    ) -> None:
        # FBX was the last consumer of WhipLib's CPU derivation in the export
        # path; Export() now only dispatches to the two canonical writers.
        stripped = without_comments(self.preview)
        export = stripped[stripped.index("bool CTrackPreview::Export(") :]
        for legacy in (
            "MakeTrackSurface",
            "MakeAILine",
            "MakeSigns",
            "eBackModeling",
            "CShapeData",
            "FlipTexCoordsForExport",
            "TransformVertsForExport",
        ):
            self.assertNotIn(legacy, export, legacy)
        self.assertIn("ExportObj_Internal", export)
        self.assertIn("ExportGltf_Internal", export)

    def test_the_sign_option_is_disabled_for_every_format(self) -> None:
        # With FBX gone there is no format left that could honour it, so the
        # gate is no longer conditional on the export type.
        stripped = without_comments(self.wizard)
        self.assertIn("ckSigns->setEnabled(false)", stripped)
        self.assertNotIn("m_exportType == eExportType", stripped)


class DocumentationTests(unittest.TestCase):
    def test_the_removal_is_recorded_where_the_exporters_are(self) -> None:
        obj = (DOCS / "obj-export.md").read_text(encoding="utf-8")
        self.assertIn("E4-S3 removed FBX", obj)

    def test_user_facing_docs_no_longer_promise_fbx(self) -> None:
        for name in ("README.md", "docs/building.md"):
            text = (ROOT / name).read_text(encoding="utf-8")
            self.assertNotIn("FBX", text, name)


if __name__ == "__main__":
    unittest.main()
