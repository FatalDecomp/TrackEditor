"""E4A-S6, editor half: signs and buildings in the canonical exports.

The gap E4-S1 through E4-S5 could not close was ROLLER's, not the exporters':
`drawtrk3_emit_full_track` walked track chunks only, so the extraction both
exporters consume simply had no signs in it, and `CExportWizard`'s *Include
signs* checkbox was written and never read.

ROLLER's `drawtrk3_emit_full_scenery` closed it once in the core, so this half
is small: honour the checkbox, group signs and scenery by the content class the
producer published, and restore the "Sign N" object names the pre-migration FBX
exporter wrote.
"""

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


class SceneryExportTests(unittest.TestCase):
    def setUp(self) -> None:
        self.common_h = (EDITOR / "EditorExportCommon.h").read_text(
            encoding="utf-8"
        )
        self.common = (EDITOR / "EditorExportCommon.cpp").read_text(
            encoding="utf-8"
        )
        self.wizard = (EDITOR / "ExportWizard.cpp").read_text(encoding="utf-8")
        self.wizard_ui = (EDITOR / "ExportWizard.ui").read_text(
            encoding="utf-8"
        )
        self.preview = (EDITOR / "TrackPreview.cpp").read_text(encoding="utf-8")

    def test_the_wizard_option_is_live_and_says_what_it_does(self) -> None:
        body = function_body(self.wizard, "CExportWizard::CExportWizard(")
        self.assertNotIn("setEnabled(false)", body)
        self.assertNotIn("m_bExportSigns = false", body)
        self.assertIn("ckSigns->setChecked(m_bExportSigns)", body)
        # It governs buildings as well as advert panels now, so the label and
        # the tooltip both say so rather than under-promising.
        self.assertIn("Include signs and buildings", self.wizard_ui)
        self.assertIn("buildings", function_body(
            self.wizard, "CExportWizard::CExportWizard("))

    def test_the_option_reaches_both_exporters_through_the_shared_layer(
        self,
    ) -> None:
        export = function_body(self.preview, "bool CTrackPreview::Export(")
        self.assertEqual(export.count("exportWizard.m_bExportSigns"), 2)

        for signature in (
            "bool CTrackPreview::ExportObj_Internal(",
            "bool CTrackPreview::ExportGltf_Internal(",
        ):
            body = function_body(self.preview, signature)
            self.assertIn("Options.bExportScenery = bExportScenery", body)

        for name in ("EditorObjExporter.cpp", "EditorGltfExporter.cpp"):
            source = (EDITOR / name).read_text(encoding="utf-8")
            self.assertIn(
                "Grouping.bExportScenery = Options.bExportScenery", source
            )
        # The scope decision itself lives in one place; neither format may
        # grow a filter of its own.
        self.assertEqual(self.common.count("Grouping.bExportScenery"), 1)

    def test_grouping_is_dispatched_on_content_class_not_surface_class(
        self,
    ) -> None:
        body = without_comments(
            function_body(
                self.common, "bool CEditorExportConventions::BuildObjects("
            )
        )
        # AD-8: the producer publishes what a surface is; an exporter must
        # never re-derive that from geometry or from the surface class.
        self.assertIn(
            "Primitive.unContentClass == ROLLER_ED_CONTENT_AUTHORED_SIGN", body
        )
        self.assertIn(
            "Primitive.unContentClass == ROLLER_ED_CONTENT_AUTHORED_SCENERY",
            body,
        )
        # Surface class survives only as the track body's group name.
        self.assertIn("g_aunExportedSurfaceClasses[c]", body)
        self.assertNotIn("ROLLER_ED_SURFACE_CLASS_SIGN", body)
        self.assertNotIn("ROLLER_ED_SURFACE_CLASS_BUILDING", body)

    def test_the_legacy_sign_object_names_are_restored(self) -> None:
        self.assertIn(
            "static std::string SignObjectName(uint32_t uiSignIndex)",
            self.common_h,
        )
        body = function_body(
            self.common, "std::string CEditorExportConventions::SignObjectName("
        )
        self.assertIn('"Sign " + std::to_string(uiSignIndex)', body)

        objects = function_body(
            self.common, "bool CEditorExportConventions::BuildObjects("
        )
        self.assertIn("SignObjectName(i)", objects)
        self.assertIn('SignObjectName(i) + " (Back)"', objects)
        self.assertIn("SceneryObjectName()", objects)

    def test_runtime_scenery_is_still_rejected_here_as_well(self) -> None:
        # ROLLER's traversal already drops it, but the exporters keep their own
        # filter: a core that regressed must not silently ship trees and
        # towers into a .blend.
        body = function_body(
            self.common, "bool CEditorExportConventions::IsAuthoredContent("
        )
        self.assertNotIn("ROLLER_ED_CONTENT_RUNTIME_SCENERY", body)
        self.assertIn("ROLLER_ED_CONTENT_AUTHORED_SIGN", body)
        self.assertIn("ROLLER_ED_CONTENT_AUTHORED_SCENERY", body)

    def test_the_ui_never_calls_the_facade(self) -> None:
        # E3-S1's rule is unchanged by this story.
        self.assertNotIn("RollerEd_", without_comments(self.preview))


class DocumentationTests(unittest.TestCase):
    def test_both_exporter_docs_record_the_new_scope(self) -> None:
        obj = (DOCS / "obj-export.md").read_text(encoding="utf-8")
        gltf = (DOCS / "gltf-export.md").read_text(encoding="utf-8")
        texture = (DOCS / "texture-export.md").read_text(encoding="utf-8")

        for text, name in ((obj, "obj-export.md"), (gltf, "gltf-export.md")):
            self.assertIn("E4A-S6", text, name)
            self.assertIn(
                "0005-camera-independent-scenery-traversal.md", text, name
            )
            self.assertNotIn("not exported yet", text, name)

        # The two conventions a user can see, both stated where they look.
        self.assertIn("yaw the track file", obj)
        self.assertIn("RUNTIME_SCENERY", obj)
        self.assertIn("Sign 0, Sign 1", obj)
        # E4-S4's second atlas finally has a consumer.
        self.assertIn("E4A-S6", texture)


if __name__ == "__main__":
    unittest.main()
