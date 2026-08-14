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


class BoundsRegressionTests(unittest.TestCase):
    def test_billboard_lookup_is_guarded_on_both_sides(self) -> None:
        widget = read(EDITOR / "EditSignWidget.cpp")
        update = function_body(
            widget, "void CEditSignWidget::UpdateGeometrySelection"
        )
        self.assertRegex(
            update,
            re.compile(
                r"bBillboarded\s*=\s*iSignType\s*>=\s*0\s*"
                r"&&\s*iSignType\s*<\s*g_signAyCount\s*"
                r"&&\s*g_signAy\[iSignType\]\.bBillboarded",
                re.S,
            ),
        )
        self.assertEqual(update.count("g_signAy[iSignType].bBillboarded"), 1)

    def test_safe_index_predicate_covers_empty_unknown_and_tower_values(self) -> None:
        native = read(ROOT / "tests" / "editor_sign_model_test.cpp")
        for contract in (
            "IsKnownSignIndex(-1, 17)",
            "IsKnownSignIndex(16, 17)",
            "IsKnownSignIndex(17, 17)",
            "IsKnownSignIndex(256, 17)",
        ):
            self.assertIn(contract, native)


class TowerLockoutTests(unittest.TestCase):
    def test_the_red_tower_notice_is_present(self) -> None:
        tree = ET.parse(EDITOR / "EditSignWidget.ui")
        notice = tree.find(".//widget[@name='lblTowerDisabled']")
        self.assertIsNotNone(notice)
        self.assertEqual(
            notice.findtext("property[@name='text']/string"),
            "Tower exists on this chunk",
        )

        widget = read(EDITOR / "EditSignWidget.cpp")
        self.assertIn(
            'lblTowerDisabled->setStyleSheet("QLabel { color : red; }")',
            widget,
        )
        self.assertIn("lblTowerDisabled->setVisible(bHasTower);", widget)
        self.assertIn("lblTowerDisabled->setEnabled(true);", widget)

    def test_every_sign_control_is_disabled_for_a_tower(self) -> None:
        widget = read(EDITOR / "EditSignWidget.cpp")
        update = function_body(
            widget, "void CEditSignWidget::UpdateGeometrySelection"
        )
        for control in (
            "dsbYaw",
            "dsbPitch",
            "dsbRoll",
            "sbHOffset",
            "sbVOffset",
            "cbType",
            "leUnk",
            "pbEdit",
            "lblTex",
        ):
            self.assertRegex(update, rf"{control}->setEnabled\([^;]*bChunkHasSign")
        self.assertIn("pbSign->setEnabled(!bHasTower);", update)

    def test_selecting_a_tower_does_not_mutate_the_type_combo(self) -> None:
        widget = read(EDITOR / "EditSignWidget.cpp")
        update = function_body(
            widget, "void CEditSignWidget::UpdateGeometrySelection"
        )
        guard = function_body(update, "if (!bHasTower)")
        self.assertIn("BLOCK_SIG_AND_DO(cbType", guard)
        self.assertEqual(update.count("BLOCK_SIG_AND_DO(cbType"), 1)
        self.assertIn(
            "const bool bUnk = bChunkHasSign && iSignType > 255;", update
        )


class MixedRangeTests(unittest.TestCase):
    def test_all_direct_sign_writes_use_the_shared_tower_exclusion(self) -> None:
        widget = read(EDITOR / "EditSignWidget.cpp")
        for function in (
            "YawChanged",
            "PitchChanged",
            "RollChanged",
            "HOffsetChanged",
            "VOffsetChanged",
            "TypeChanged",
            "SignClicked",
            "UnkChanged",
        ):
            body = function_body(widget, f"void CEditSignWidget::{function}")
            self.assertIn("CEditorSignModel::ApplyToRange(", body)

        model = read(EDITOR / "EditorSignModel.cpp")
        apply = function_body(model, "int CEditorSignModel::ApplyToRange")
        self.assertIn("if (IsTower(chunkAy[i].iSignType))", apply)
        self.assertLess(
            apply.index("if (IsTower(chunkAy[i].iSignType))"),
            apply.index("Operation(chunkAy[i]);"),
        )

    def test_sign_texture_dialog_routes_towers_to_scratch_storage(self) -> None:
        dialog = read(EDITOR / "EditSurfaceDialog.cpp")
        get_value = function_body(dialog, "int &CEditSurfaceDialog::GetValue")
        sign_case = get_value[get_value.index("case eSurfaceField::SURFACE_SIGN:") :]
        self.assertLess(
            sign_case.index("CEditorSignModel::IsTower("),
            sign_case.index(".iSignTexture"),
        )
        self.assertIn("return (int&)m_uiSignedBitValue;", sign_case)

    def test_native_regression_checks_byte_identity(self) -> None:
        native = read(ROOT / "tests" / "editor_sign_model_test.cpp")
        self.assertIn("TestMixedRangeLeavesEveryTowerByteIdentical", native)
        self.assertGreaterEqual(native.count("std::memcmp("), 3)
        self.assertIn("Chunks[1].iSignType = 256", native)
        self.assertIn("Chunks[2].iSignType = 777", native)


class BuildIntegrationTests(unittest.TestCase):
    def test_model_and_both_regressions_are_registered(self) -> None:
        app_cmake = read(EDITOR / "CMakeLists.txt")
        root_cmake = read(ROOT / "CMakeLists.txt")
        self.assertIn("EditorSignModel.cpp", app_cmake)
        self.assertIn("trackeditor-e7-s7-sign-model-test", root_cmake)
        self.assertIn("trackeditor-e7-s7-sign-tower-lockout-contract", root_cmake)


if __name__ == "__main__":
    unittest.main()
