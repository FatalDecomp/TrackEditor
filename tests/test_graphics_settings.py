from pathlib import Path
import re
import unittest


REPOSITORY_ROOT = Path(__file__).resolve().parents[1]
EDITOR = REPOSITORY_ROOT / "TrackEditor"
ROLLER = REPOSITORY_ROOT / "external" / "ROLLER" / "PROJECTS" / "ROLLER"


def function_body(source: str, signature: str) -> str:
    start = source.index(signature)
    brace = source.index("{", start)
    depth = 0
    for position in range(brace, len(source)):
        if source[position] == "{":
            depth += 1
        elif source[position] == "}":
            depth -= 1
            if depth == 0:
                return source[start : position + 1]
    raise AssertionError(f"function body not closed: {signature}")


class GraphicsSettingsTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.window = (EDITOR / "MainWindow.cpp").read_text(encoding="utf-8")
        cls.window_ui = (EDITOR / "MainWindow.ui").read_text(encoding="utf-8")
        cls.dialog = (EDITOR / "GraphicsDialog.cpp").read_text(encoding="utf-8")
        cls.dialog_ui = (EDITOR / "GraphicsDialog.ui").read_text(encoding="utf-8")
        cls.service = (EDITOR / "EditorRenderService.cpp").read_text(
            encoding="utf-8"
        )
        cls.api = (ROLLER / "editor_api.h").read_text(encoding="ascii")
        cls.adapter = (ROLLER / "editor_legacy_scene.c").read_text(
            encoding="utf-8"
        )
        cls.track_draw = (ROLLER / "drawtrk3.c").read_text(encoding="utf-8")

    def test_graphics_action_follows_preferences(self) -> None:
        settings_menu = self.window_ui[
            self.window_ui.index('<widget class="QMenu" name="menuSettings">') :
            self.window_ui.index("</widget>", self.window_ui.index(
                '<widget class="QMenu" name="menuSettings">'
            ))
        ]
        self.assertLess(
            settings_menu.index('name="actPreferences"'),
            settings_menu.index('name="actGraphics"'),
        )
        self.assertIn("Graphics...", self.window_ui)
        self.assertIn(
            "connect(actGraphics, &QAction::triggered, "
            "this, &CMainWindow::OnGraphics)",
            self.window,
        )

    def test_dialog_contains_the_requested_controls_and_defaults(self) -> None:
        for label in (
            "Draw Dist",
            "Hardware Rendering",
            "Software Rendering Options",
            "Display",
            "VGA",
            "SVGA",
            "Anti-aliasing",
            "Anisotropy",
            "Texture Filter",
            "Trilinear filtering",
            "LOD bias",
            "Emulate transparent borders",
        ):
            self.assertIn(f"<string>{label}</string>", self.dialog_ui)
        defaults = function_body(
            self.window, "tGraphicsPreferences::tGraphicsPreferences()"
        )
        for expected in (
            "iDrawDistancePercent(100)",
            "bHardwareRendering(true)",
            "iSoftwareDisplay(1)",
            "iAntiAliasing(0)",
            "iAnisotropy(3)",
            "iTextureFilter(0)",
            "bTrilinear(false)",
            "dLodBias(0.0)",
            "bEmulateTransparentBorders(true)",
        ):
            self.assertIn(expected, defaults)
        self.assertIn('<widget class="QSlider" name="slDrawDistance">', self.dialog_ui)
        self.assertNotIn('name="sbDrawDistance"', self.dialog_ui)

    def test_software_mode_disables_every_hardware_option(self) -> None:
        update = function_body(
            self.dialog, "void CGraphicsDialog::UpdateHardwareControls()"
        )
        self.assertIn(
            "gbHardwareOptions->setEnabled(ckHardwareRendering->isChecked())",
            update,
        )
        self.assertIn(
            "gbSoftwareOptions->setEnabled(!ckHardwareRendering->isChecked())",
            update,
        )
        for control in (
            "cbAntiAliasing",
            "cbAnisotropy",
            "cbTextureFilter",
            "ckTrilinear",
            "dsbLodBias",
            "ckEmulateTransparentBorders",
        ):
            hardware_group = self.dialog_ui[
                self.dialog_ui.index(
                    '<widget class="QGroupBox" name="gbHardwareOptions">'
                ) :
                self.dialog_ui.index(
                    '<spacer name="verticalSpacer">',
                    self.dialog_ui.index(
                        '<widget class="QGroupBox" name="gbHardwareOptions">'
                    ),
                )
            ]
            self.assertIn(f'name="{control}"', hardware_group)
        software_group = self.dialog_ui[
            self.dialog_ui.index(
                '<widget class="QGroupBox" name="gbSoftwareOptions">'
            ) : self.dialog_ui.index(
                '<spacer name="verticalSpacer">',
                self.dialog_ui.index(
                    '<widget class="QGroupBox" name="gbSoftwareOptions">'
                ),
            )
        ]
        self.assertIn('name="cbSoftwareDisplay"', software_group)

    def test_all_values_round_trip_through_trackeditor_ini(self) -> None:
        keys = (
            "graphics_draw_distance",
            "graphics_hardware_rendering",
            "graphics_software_display",
            "graphics_antialiasing",
            "graphics_anisotropy",
            "graphics_texture_filter",
            "graphics_trilinear",
            "graphics_lod_bias",
            "graphics_emulate_transparent_borders",
        )
        for key in keys:
            self.assertGreaterEqual(self.window.count(f'"{key}"'), 2, key)
        self.assertIn('m_sAppPath + "/TrackEditor.ini"', self.window)

    def test_settings_are_copied_to_the_worker_and_applied_before_render(self) -> None:
        process = function_body(
            self.service,
            "tEdRenderResult ProcessRequest(const tEdRenderRequest &Request)",
        )
        self.assertLess(
            process.index("RollerEd_SetGraphicsSettings"),
            process.index("RollerEd_LoadTrackFile"),
        )
        self.assertIn("ullGraphicsSettingsRevision", self.service)
        apply_settings = function_body(
            self.window, "void CMainWindow::ApplyGraphicsSettings()"
        )
        self.assertIn("m_pRenderService->SetGraphicsSettings(Settings)", apply_settings)
        self.assertIn("RefreshGraphicsSettings", apply_settings)

    def test_facade_validates_and_applies_every_renderer_setting(self) -> None:
        self.assertIn("tEdGraphicsSettings", self.api)
        self.assertIn("RollerEd_SetGraphicsSettings", self.api)
        for setter in (
            "game_render_set_antialiasing",
            "game_render_set_anisotropy_level",
            "game_render_set_texture_filter",
            "game_render_set_trilinear",
            "game_render_set_lod_bias",
            "game_render_set_emulate_software_track_borders",
        ):
            self.assertIn(setter, self.adapter)
        self.assertIn("g_fDrawDistanceFraction", self.adapter)
        self.assertIn("eSoftwareDisplay", self.api)
        self.assertIn("editor_scene_set_legacy_display", self.adapter)
        visibility = function_body(
            self.track_draw, "int CalcVisibleTrackEditor(unsigned int uiViewMode)"
        )
        self.assertIn("g_fDrawDistanceFraction", visibility)
        self.assertIn("TrakView[iCurrChunk].byForwardMainChunks", visibility)
        self.assertIn("TrakView[iCurrChunk].byBackwardMainChunks", visibility)
        self.assertIn("(TRAK_LEN - 1) - TrackSize", visibility)
        self.assertIn("TRAK_LEN - 1", visibility)


if __name__ == "__main__":
    unittest.main()
