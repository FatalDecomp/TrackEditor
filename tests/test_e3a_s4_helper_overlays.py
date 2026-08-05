import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
EDITOR = ROOT / "TrackEditor"


class PinnedCoreTests(unittest.TestCase):
    def test_the_pin_carries_the_helper_overlays(self) -> None:
        # The checkbox compiles without them; nothing draws until the core
        # that derives the helper geometry is pinned.
        roller = ROOT / "external" / "ROLLER" / "PROJECTS" / "ROLLER"
        helpers = roller / "editor_helpers.h"
        draw = (roller / "drawtrk3.h").read_text(encoding="utf-8")

        self.assertTrue(helpers.is_file())
        self.assertIn("ed_helper_ai_line_point", helpers.read_text(encoding="utf-8"))
        self.assertIn(
            "void drawtrk3_editor_draw_helpers(GameRenderer *pRenderer);", draw
        )


class CenterLineToggleTests(unittest.TestCase):
    def test_the_new_bit_does_not_collide_with_an_existing_one(self) -> None:
        flags = (EDITOR / "DisplaySettingsFlags.h").read_text(encoding="utf-8")
        values = {}
        for line in flags.splitlines():
            parts = line.split()
            if len(parts) == 3 and parts[0] == "#define" and parts[1].startswith("SHOW_"):
                values.setdefault(int(parts[2], 16), []).append(parts[1])
        # The mask is persisted in QSettings, so a collision would silently
        # tie two checkboxes together in every existing user's settings.
        collisions = {v: n for v, n in values.items() if len(n) > 1}
        self.assertEqual(collisions, {})
        self.assertIn("SHOW_CENTER_LINE", values.get(0x20000000, []))

    def test_the_checkbox_exists_and_is_read_and_written(self) -> None:
        ui = (EDITOR / "DisplaySettings.ui").read_text(encoding="utf-8")
        source = (EDITOR / "DisplaySettings.cpp").read_text(encoding="utf-8")

        self.assertIn('name="ckCenterLine"', ui)
        self.assertIn("<string>Center Line</string>", ui)
        self.assertIn("ckCenterLine->isChecked()", source)
        self.assertIn("SHOW_CENTER_LINE", source)
        self.assertIn("BLOCK_SIG_AND_DO(ckCenterLine", source)

    def test_all_three_helpers_map_to_their_overlay_flags(self) -> None:
        source = (EDITOR / "EditorOverlaySettings.cpp").read_text(encoding="utf-8")
        for legacy, overlay in (
            ("SHOW_AILINE_MODELS", "ROLLER_ED_OVERLAY_SHOW_AI_LINES"),
            ("SHOW_CENTER_LINE", "ROLLER_ED_OVERLAY_SHOW_CENTER_LINE"),
            ("SHOW_ENVIRONMENT", "ROLLER_ED_OVERLAY_SHOW_ENVIRONMENT_FLOOR"),
        ):
            self.assertIn(legacy, source)
            self.assertIn(overlay, source)

    def test_the_helpers_are_independently_toggleable(self) -> None:
        # Each is its own row in the feature table, so no two share a bit.
        source = (EDITOR / "EditorOverlaySettings.cpp").read_text(encoding="utf-8")
        table = source[source.index("g_aFeatures[]") : source.index("};", source.index("g_aFeatures[]"))]
        for name in (
            "ROLLER_ED_OVERLAY_SHOW_AI_LINES",
            "ROLLER_ED_OVERLAY_SHOW_CENTER_LINE",
            "ROLLER_ED_OVERLAY_SHOW_ENVIRONMENT_FLOOR",
        ):
            self.assertEqual(table.count(name), 1)


class CenterLineDefaultTests(unittest.TestCase):
    def test_the_center_line_starts_on(self) -> None:
        ui = (EDITOR / "DisplaySettings.ui").read_text(encoding="utf-8")
        block = ui[ui.index('name="ckCenterLine"') :]
        block = block[: block.index("</widget>")]
        self.assertIn('<property name="checked">', block)
        self.assertIn("<bool>true</bool>", block)

    def test_an_existing_profile_picks_the_default_up_once(self) -> None:
        window = (EDITOR / "MainWindow.cpp").read_text(encoding="utf-8")
        # A saved mask predating the bit has it clear for no reason the user
        # chose, so the .ui default alone would never reach an existing user.
        self.assertIn("center_line_default_applied", window)
        self.assertIn("uiShowModels |= SHOW_CENTER_LINE;", window)
        # Applied once: a later deliberate untick has to stick.
        self.assertIn(
            'settings.setValue("center_line_default_applied", true);', window
        )


if __name__ == "__main__":
    unittest.main()
