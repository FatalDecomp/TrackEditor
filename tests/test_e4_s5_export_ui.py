"""E4-S5: export enum and UI.

The story reads "extend eExportType for glTF; hide FBX when compiled out.
AC: OBJ + glTF always offered, FBX only when built with it."

E4-S2 extended the enum and added the menu action. E4-S3 then removed FBX
outright, so the second clause has nothing left to hide and the criterion
reduces to "OBJ and glTF are always offered". These tests pin that, and pin the
file-name rules that decide which format - and for glTF, which container -
actually gets written, which is where the remaining real work was.
"""

import re
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
EDITOR = ROOT / "TrackEditor"


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


class FormatTableTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.header = (EDITOR / "EditorExportFormat.h").read_text(
            encoding="utf-8"
        )
        cls.source = (EDITOR / "EditorExportFormat.cpp").read_text(
            encoding="utf-8"
        )

    def test_the_enum_lives_with_the_table_not_in_the_viewport(self) -> None:
        self.assertIn("enum eExportType", self.header)
        self.assertIn("EXPORT_OBJ = 0", self.header)
        self.assertIn("EXPORT_GLTF", self.header)
        # TrackPreview.h is the viewport; it consumes the enum rather than
        # defining it.
        preview_header = (EDITOR / "TrackPreview.h").read_text(
            encoding="utf-8"
        )
        self.assertNotIn("enum eExportType", preview_header)
        self.assertIn('#include "EditorExportFormat.h"', preview_header)

    def test_the_table_is_qt_free_so_the_rules_are_testable(self) -> None:
        combined = without_comments(self.header) + without_comments(self.source)
        self.assertNotIn("#include <Q", combined)
        self.assertNotIn('#include "Q', combined)
        self.assertNotIn("QFileDialog", combined)
        self.assertNotIn("RollerEd_", combined)

    def test_both_formats_are_unconditional(self) -> None:
        # Nothing compiles a format in or out any more, which is what is left
        # of "FBX only when built with it".
        self.assertNotIn("#if", self.source)
        self.assertNotIn("FBX", self.source)
        self.assertIn("{ EXPORT_OBJ,", self.source)
        self.assertIn("{ EXPORT_GLTF,", self.source)

    def test_the_glTF_default_container_is_the_self_contained_one(
        self,
    ) -> None:
        # The first clause of a Qt filter is the one the dialog opens on.
        self.assertIn('"glTF Binary (*.glb);;glTF JSON (*.gltf)"', self.source)


class FileNameRuleTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.source = (EDITOR / "EditorExportFormat.cpp").read_text(
            encoding="utf-8"
        )
        cls.preview = (EDITOR / "TrackPreview.cpp").read_text(encoding="utf-8")

    def test_the_dialog_reports_which_filter_was_selected(self) -> None:
        # Without this the suffix cannot be resolved at all.
        body = function_body(self.preview, "bool CTrackPreview::Export(")
        self.assertIn("QString sSelectedFilter", body)
        self.assertIn("&sSelectedFilter", body)
        self.assertIn("CEditorExportFormats::For(exportType)", body)

    def test_a_bare_name_gets_the_selected_filters_suffix(self) -> None:
        # Qt's static getSaveFileName sets no default suffix and only the
        # Windows native dialog adds one, so a name typed without an extension
        # would otherwise reach the writer bare.
        body = function_body(self.preview, "bool CTrackPreview::Export(")
        self.assertIn("CEditorExportFormats::ApplyDefaultSuffix", body)

    def test_one_place_decides_the_gltf_container(self) -> None:
        gltf = function_body(
            self.preview, "bool CTrackPreview::ExportGltf_Internal("
        )
        self.assertIn("CEditorExportFormats::IsBinaryGltf", gltf)
        # The old ad-hoc suffix comparison is gone.
        self.assertNotIn('suffix().compare("glb"', gltf)

    def test_a_directory_dot_is_not_a_file_suffix(self) -> None:
        body = function_body(
            self.source, "std::string CEditorExportFormats::SuffixOf("
        )
        self.assertIn("LastSeparator", self.source)
        self.assertIn("uiDot < uiNameStart", body)


class MenuTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.window = (EDITOR / "MainWindow.cpp").read_text(encoding="utf-8")
        cls.window_ui = (EDITOR / "MainWindow.ui").read_text(encoding="utf-8")
        cls.formats = (EDITOR / "EditorExportFormat.cpp").read_text(
            encoding="utf-8"
        )

    def test_the_menu_offers_exactly_the_formats_in_the_table(self) -> None:
        # Adding a format to the table without a menu entry, or the reverse,
        # is the half-landed state this pins against.
        table_entries = re.findall(r"\{ (EXPORT_\w+),", self.formats)
        actions = re.findall(r'<addaction name="(actExport\w+)"/>',
                             self.window_ui)
        model_actions = [a for a in actions if a != "actExportMangled"]
        self.assertEqual(len(table_entries), len(model_actions))
        self.assertEqual(["actExportOBJ", "actExportGLTF"], model_actions)

    def test_both_actions_are_wired_and_gated_on_a_renderable_document(
        self,
    ) -> None:
        for action, slot, kind in (
            ("actExportOBJ", "OnExportOBJ", "EXPORT_OBJ"),
            ("actExportGLTF", "OnExportGLTF", "EXPORT_GLTF"),
        ):
            self.assertIn(
                f"connect({action}, &QAction::triggered, this, "
                f"&CMainWindow::{slot})",
                self.window,
            )
            self.assertIn(f"eExportType::{kind}", self.window)
            self.assertIn(f"{action}->setEnabled(bCanExport)", self.window)

        body = function_body(self.window, "void CMainWindow::UpdateExportActions(")
        self.assertIn("pPreview && pPreview->CanExport()", body)

    def test_no_export_action_is_hidden_or_compiled_out(self) -> None:
        # "FBX only when built with it" was the only conditional here. Dock
        # widgets legitimately start hidden, so this looks at export actions.
        window_stripped = without_comments(self.window)
        self.assertNotIn("TRACKEDITOR_ENABLE", window_stripped)
        for line in window_stripped.splitlines():
            if "actExport" in line:
                self.assertNotIn("setVisible", line)


if __name__ == "__main__":
    unittest.main()
