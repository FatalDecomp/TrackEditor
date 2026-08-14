import re
import unittest
import xml.etree.ElementTree as ET
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
EDITOR = ROOT / "TrackEditor"


def source(name: str) -> str:
    return (EDITOR / name).read_text(encoding="utf-8")


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


class PinnedCoreTests(unittest.TestCase):
    def test_the_pin_exposes_the_e7_s3_tower_marker_flag(self) -> None:
        header = (
            ROOT / "external" / "ROLLER" / "PROJECTS" / "ROLLER" / "editor_api.h"
        ).read_text(encoding="utf-8")
        self.assertRegex(
            header,
            r"ROLLER_ED_OVERLAY_SHOW_TOWER_MARKERS\s*=\s*1u\s*<<\s*12",
        )


class FeatureWordTests(unittest.TestCase):
    def test_towers_use_the_first_bit_of_a_second_persisted_word(self) -> None:
        flags = source("DisplaySettingsFlags.h")
        self.assertIn("#define SHOW_FEATURE_TOWERS        0x00000001", flags)
        self.assertIn('QSettings word named "show_features"', flags)

        # The retired environment bit remains retired; Towers must not consume
        # a supposedly free bit in the already persisted legacy word.
        self.assertIn("#define SHOW_ENVIRONMENT_RETIRED   0x01000000", flags)
        self.assertNotRegex(
            flags,
            r"SHOW_FEATURE_TOWERS\s+0x01000000",
        )

    def test_the_translator_maps_only_the_new_word_to_the_facade_flag(self) -> None:
        overlay = source("EditorOverlaySettings.cpp")
        table_start = overlay.index("g_aDisplayFeatures[]")
        table = overlay[table_start : overlay.index("};", table_start)]
        self.assertIn("SHOW_FEATURE_TOWERS", table)
        self.assertIn("ROLLER_ED_OVERLAY_SHOW_TOWER_MARKERS", table)

        legacy_start = overlay.index("g_aFeatures[]")
        legacy_table = overlay[legacy_start : overlay.index("};", legacy_start)]
        self.assertNotIn("SHOW_FEATURE_TOWERS", legacy_table)


class DisplaySettingsTests(unittest.TestCase):
    def test_the_default_on_checkbox_joins_the_marker_controls(self) -> None:
        tree = ET.parse(EDITOR / "DisplaySettings.ui")
        towers = None
        marker_group = None
        for layout in tree.iter("layout"):
            direct_names = {
                widget.attrib.get("name")
                for item in layout.findall("item")
                for widget in item.findall("widget")
            }
            if {"ckStunts", "ckTowers"}.issubset(direct_names):
                marker_group = layout
                break

        self.assertIsNotNone(marker_group)
        towers = marker_group.find("item/widget[@name='ckTowers']")
        self.assertIsNotNone(towers)
        self.assertEqual(towers.findtext("property[@name='text']/string"), "Towers")
        self.assertEqual(towers.findtext("property[@name='checked']/bool"), "true")

    def test_the_checkbox_is_read_written_defaulted_and_live(self) -> None:
        display = source("DisplaySettings.cpp")
        for contract in (
            "ckTowers->setChecked(true)",
            "ckTowers->isChecked()",
            "SHOW_FEATURE_TOWERS",
            "BLOCK_SIG_AND_DO(ckTowers",
            "connect(ckTowers, &QCheckBox::toggled, this, "
            "&CDisplaySettings::UpdatePreviewSelection);",
        ):
            self.assertIn(contract, display)


class PersistenceTests(unittest.TestCase):
    def test_the_new_word_is_default_on_and_persists_without_migration(self) -> None:
        window = source("MainWindow.cpp")
        self.assertRegex(
            window,
            r'settings\.value\(\s*"show_features",\s*SHOW_FEATURE_TOWERS\s*\)'
            r"\.toUInt\(\)",
        )
        self.assertRegex(
            window,
            r'settings\.setValue\(\s*"show_features",\s*'
            r"p->m_pDisplaySettings->GetFeatureSettings\(\)\s*\)",
        )
        self.assertNotIn("tower_default_applied", window)
        self.assertNotIn("towers_default_applied", window)

    def test_the_legacy_show_models_value_remains_separate(self) -> None:
        window = source("MainWindow.cpp")
        show_models_lines = [
            line for line in window.splitlines() if '"show_models"' in line
        ]
        self.assertGreaterEqual(len(show_models_lines), 2)
        self.assertTrue(all("SHOW_FEATURE_TOWERS" not in line for line in show_models_lines))


class LivePreviewTests(unittest.TestCase):
    def test_tabs_and_checkbox_updates_push_the_feature_word(self) -> None:
        window = source("MainWindow.cpp")
        self.assertGreaterEqual(window.count("->ShowFeatures("), 2)
        self.assertGreaterEqual(window.count("GetFeatureSettings()"), 4)

    def test_toggling_towers_uses_the_render_only_path(self) -> None:
        preview = source("TrackPreview.cpp")
        body = function_body(preview, "void CTrackPreview::ShowFeatures(")
        self.assertIn("m_OverlaySettings.SetShowFeatures", body)
        self.assertIn("ScheduleCameraRender();", body)
        self.assertIn("update();", body)
        for forbidden in (
            "MarkDocumentEdited",
            "SaveHistory",
            "QueueEditedTrackReload",
            "QueueLoadAndRender",
            "GetTrackData",
            "m_uiRevision",
        ):
            self.assertNotIn(forbidden, body)


if __name__ == "__main__":
    unittest.main()
