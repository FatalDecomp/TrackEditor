import re
import unittest
import xml.etree.ElementTree as ET
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
EDITOR = ROOT / "TrackEditor"


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def function_body(text: str, signature: str) -> str:
    start = text.index(signature)
    brace = text.index("{", start)
    depth = 0
    for position in range(brace, len(text)):
        if text[position] == "{":
            depth += 1
        elif text[position] == "}":
            depth -= 1
            if depth == 0:
                return text[start : position + 1]
    raise AssertionError(f"unterminated function: {signature}")


class DockIntegrationTests(unittest.TestCase):
    def test_the_edit_towers_dock_follows_the_existing_dock_convention(self) -> None:
        window = read(EDITOR / "MainWindow.cpp")
        for contract in (
            'new QDockWidget("Edit Towers", this)',
            'setObjectName("EditTowers")',
            "Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea",
            "new CEditTowerWidget(",
        ):
            self.assertIn(contract, window)

        stunt_action = window.index(
            "m_pEditStuntDockWidget->toggleViewAction()"
        )
        tower_action = window.index(
            "m_pEditTowerDockWidget->toggleViewAction()"
        )
        series_action = window.index(
            "m_pEditSeriesDockWidget->toggleViewAction()"
        )
        self.assertLess(stunt_action, tower_action)
        self.assertLess(tower_action, series_action)

    def test_dock_visibility_persists_without_resetting_old_profiles(self) -> None:
        window = read(EDITOR / "MainWindow.cpp")
        self.assertIn('settings.value(\n        "show_edit_tower"', window)
        self.assertIn('settings.setValue("show_edit_tower"', window)

        # Every dock needs a valid default placement before restoreState().
        # restoreState() then moves existing docks into their saved positions;
        # restoreDockWidget() must not be used to reinsert them afterward.
        tower_default = window.index(
            "addDockWidget(Qt::RightDockWidgetArea, "
            "p->m_pEditTowerDockWidget)"
        )
        saved_layout = window.index("restoreState(state)")
        self.assertLess(tower_default, saved_layout)
        self.assertNotIn("restoreDockWidget(", window)

        # The compatibility gate intentionally remains on the old keys: a
        # profile written before E7-S5 must retain all of its existing layout.
        gate_start = window.index("if (settings.contains(\"show_debug_data\")")
        gate_end = window.index(") {", gate_start)
        self.assertNotIn("show_edit_tower", window[gate_start:gate_end])

    def test_the_widget_and_model_are_application_sources(self) -> None:
        cmake = read(EDITOR / "CMakeLists.txt")
        for name in (
            "EditTowerWidget.cpp",
            "EditTowerWidget.h",
            "EditTowerWidget.ui",
            "EditorTowerModel.cpp",
            "EditorTowerModel.h",
        ):
            self.assertIn(name, cmake)


class ControlsTests(unittest.TestCase):
    def setUp(self) -> None:
        self.widget = read(EDITOR / "EditTowerWidget.cpp")
        self.ui_root = ET.parse(EDITOR / "EditTowerWidget.ui").getroot()

    def test_all_required_controls_are_present(self) -> None:
        names = {
            node.attrib.get("name") for node in self.ui_root.iter("widget")
        }
        self.assertTrue(
            {
                "pbTower",
                "cbMode",
                "cbZoom",
                "sbHOffset",
                "sbVOffset",
                "leRawType",
                "lblBudget",
                "lblSignDisabled",
                "lblOffsetScale",
                "lblRawPreserved",
            }.issubset(names)
        )
        ui = read(EDITOR / "EditTowerWidget.ui")
        self.assertIn("Sign exists on this chunk", ui)
        self.assertIn("0 of 32", ui)

    def test_the_five_camera_modes_have_readable_names(self) -> None:
        for name in (
            "Static",
            "Follow near (25%)",
            "Follow at distance",
            "Track surface, 2 back",
            "Overhead follow",
        ):
            self.assertIn(f'cbMode->addItem("{name}"', self.widget)

    def test_zoom_choices_publish_the_runtime_viewdist_values(self) -> None:
        for text, value in (
            ("Unchanged", 0),
            ("1 - VIEWDIST 120", 1),
            ("2 - VIEWDIST 75", 2),
            ("3 - VIEWDIST 500", 3),
            ("4 - VIEWDIST 750", 4),
        ):
            self.assertIn(f'cbZoom->addItem("{text}", {value});', self.widget)

    def test_mode_specific_offsets_are_disabled_and_overhead_notes_x128(self) -> None:
        model = read(EDITOR / "EditorTowerModel.cpp")
        self.assertIn("UsesHorizontalOffset", self.widget)
        self.assertIn("UsesVerticalOffset", self.widget)
        self.assertIn("eEditorTowerMode::TRACK_SURFACE_TWO_BACK", model)
        self.assertIn("eEditorTowerMode::OVERHEAD_FOLLOW", model)
        self.assertIn("? 128 : 32", model)
        self.assertIn("vertical offset x128 at runtime", self.widget)


class EncodingTests(unittest.TestCase):
    def test_the_codec_mirrors_the_verified_file_format(self) -> None:
        codec = read(EDITOR / "EditorTowerModel.cpp")
        for masked, mode in (
            ("0x101", "TRACK_SURFACE_TWO_BACK"),
            ("0x103", "FOLLOW_NEAR"),
            ("0x104", "OVERHEAD_FOLLOW"),
            ("0x105", "FOLLOW_AT_DISTANCE"),
        ):
            self.assertRegex(codec, rf"case {masked}: return .*::{mode};")
        self.assertIn("TOWER_TYPE_BASE + 16 * iCanonicalZoom + ModeNibble(mode)", codec)

    def test_noncanonical_raw_values_are_preserved_until_a_decoded_edit(self) -> None:
        widget = read(EDITOR / "EditTowerWidget.cpp")
        model = read(EDITOR / "EditorTowerModel.cpp")
        self.assertIn("BLOCK_SIG_AND_DO(leRawType, setText(", widget)
        self.assertIn("!CEditorTowerModel::IsCanonical(Chunk.iSignType)", widget)
        self.assertIn("CEditorTowerModel::SetRawType(", widget)
        self.assertIn("Encode(mode, DecodeZoom(chunkAy[i].iSignType))", model)
        self.assertIn("Encode(DecodeMode(chunkAy[i].iSignType), iZoom)", model)


class SafetyAndHistoryTests(unittest.TestCase):
    def test_sign_chunks_are_a_red_mutual_lockout(self) -> None:
        widget = read(EDITOR / "EditTowerWidget.cpp")
        self.assertIn("CEditorTowerModel::IsSign(Chunk.iSignType)", widget)
        self.assertIn('lblSignDisabled->setStyleSheet("QLabel { color : red; }")', widget)
        self.assertIn("lblSignDisabled->setVisible(bHasSign)", widget)
        self.assertIn("lblSignDisabled->setEnabled(true)", widget)
        self.assertIn("pbTower->setEnabled(bHasTower || bCanAdd)", widget)

        for control in ("cbMode", "cbZoom", "leRawType"):
            self.assertIn(f"{control}->setEnabled(bHasTower)", widget)

    def test_range_writes_skip_non_towers_and_add_never_replaces_a_sign(self) -> None:
        model = read(EDITOR / "EditorTowerModel.cpp")
        add = function_body(model, "int CEditorTowerModel::AddTowers")
        self.assertIn("chunkAy[i].iSignType != -1", add)
        self.assertIn("iTowerCount < TOWER_LIMIT", add)

        for signature in (
            "DeleteTowers",
            "SetMode",
            "SetZoom",
            "SetHorizontalOffset",
            "SetVerticalOffset",
            "SetRawType",
        ):
            body = function_body(model, f"int CEditorTowerModel::{signature}")
            self.assertIn("IsTower(chunkAy[i].iSignType)", body)

    def test_the_32_tower_budget_has_a_visible_refusal_reason(self) -> None:
        header = read(EDITOR / "EditorTowerModel.h")
        widget = read(EDITOR / "EditTowerWidget.cpp")
        self.assertIn("TOWER_LIMIT = 32", header)
        self.assertIn('sBudget += " - tower limit reached"', widget)
        self.assertIn("iTowerCount < CEditorTowerModel::TOWER_LIMIT", widget)

    def test_every_mutating_control_uses_history_and_refreshes_the_dock(self) -> None:
        widget = read(EDITOR / "EditTowerWidget.cpp")
        commit = function_body(widget, "void CEditTowerWidget::CommitEdit")
        self.assertIn("g_pMainWindow->SaveHistory(sDescription);", commit)
        self.assertIn("g_pMainWindow->UpdateWindow();", commit)
        self.assertGreaterEqual(widget.count("CommitEdit("), 7)


class NativeCoverageTests(unittest.TestCase):
    def test_codec_range_roundtrip_and_history_have_a_native_target(self) -> None:
        cmake = read(ROOT / "CMakeLists.txt")
        native = read(ROOT / "tests" / "editor_tower_model_test.cpp")
        self.assertIn("trackeditor-e7-s5-tower-model-test", cmake)
        self.assertIn("test_every_mode_and_zoom_round_trips_canonically", native)
        self.assertIn("test_range_lifecycle_skips_signs_and_obeys_the_budget", native)
        self.assertIn("roundTrip == bytes", native)
        self.assertIn("history.Undo(track)", native)
        self.assertIn("history.Redo(track)", native)
        self.assertIn("iSignType == 777", native)


if __name__ == "__main__":
    unittest.main()
