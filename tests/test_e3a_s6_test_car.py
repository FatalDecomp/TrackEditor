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


class PinnedCoreTests(unittest.TestCase):
    def test_the_pin_carries_the_test_car(self) -> None:
        roller = ROOT / "external" / "ROLLER" / "PROJECTS" / "ROLLER"
        header = (roller / "editor_api.h").read_text(encoding="utf-8")

        self.assertTrue((roller / "editor_test_car.h").is_file())
        self.assertIn("#define ROLLER_ED_OVERLAY_STATE_VERSION 3u", header)
        self.assertIn("uint32_t uiTestCarDesign;", header)
        self.assertIn("uint32_t uiTestCarAiLine;", header)
        self.assertIn(
            "ROLLER_ED_OVERLAY_TEST_CAR_MILLION_PLUS = 1u << 10", header
        )
        self.assertIn(
            "ROLLER_ED_OVERLAY_TEST_CAR_ADVANCED = 1u << 11", header
        )


class TranslationTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.source = (EDITOR / "EditorOverlaySettings.cpp").read_text(
            encoding="utf-8"
        )
        cls.header = (EDITOR / "EditorOverlaySettings.h").read_text(
            encoding="utf-8"
        )

    def test_the_editor_model_enum_stops_at_the_translator(self) -> None:
        # The same boundary the legacy SHOW_* mask has: neither the queue nor
        # the worker may learn what an eWhipModel is.
        for name in ("EditorRenderQueue.h", "EditorRenderService.cpp"):
            source = (EDITOR / name).read_text(encoding="utf-8")
            self.assertNotIn("eWhipModel", source)
            self.assertNotIn("CAR_DESIGN", source)

    def test_the_design_table_covers_every_car_model(self) -> None:
        types = (ROOT / "WhipLib" / "Types.h").read_text(encoding="utf-8")
        enum_body = types[types.index("enum class eWhipModel") :]
        enum_body = enum_body[: enum_body.index("};")]
        models = {
            line.strip().rstrip(",").split(" ")[0]
            for line in enum_body.splitlines()
            if line.strip().startswith("CAR_")
        }
        table = self.source[
            self.source.index("g_aCarDesigns[]") : self.source.index(
                "};", self.source.index("g_aCarDesigns[]")
            )
        ]
        for model in models:
            self.assertIn(
                f"eWhipModel::{model}",
                table,
                f"{model} has no ROLLER design",
            )

    def test_the_x_and_y_variants_share_a_design_but_not_a_skin(self) -> None:
        # WhipLib's GetCoords returned identical geometry for the pair, so the
        # plan is shared -- but the Y variant is ROLLER's advanced-cars set,
        # with its own texture bank and the mirror palette remap, so the skin
        # is not. Getting only the first half right left every Y model drawing
        # its X twin.
        table = self.source[
            self.source.index("g_aCarDesigns[]") : self.source.index(
                "};", self.source.index("g_aCarDesigns[]")
            )
        ]
        for name in ("AUTO", "DESILVA", "PULSE", "GLOBAL", "MILLION",
                     "MISSION", "ZIZIN", "REISE"):
            self.assertEqual(
                table.count(f"ROLLER_CAR_DESIGN_{name},"),
                2,
                f"CAR_X{name} and CAR_Y{name} must share one design",
            )

        # Every X row takes the plain skin and every Y row the advanced one.
        for line in table.splitlines():
            if "eWhipModel::CAR_X" in line or line.strip().startswith(
                "{ eWhipModel::CAR_F1WACK"
            ) or line.strip().startswith("{ eWhipModel::CAR_DEATH"):
                self.assertIn("false", line, line)
            elif "eWhipModel::CAR_Y" in line:
                self.assertIn("true", line, line)

    def test_the_advanced_skin_reaches_the_overlay(self) -> None:
        header = (EDITOR / "EditorOverlaySettings.h").read_text(encoding="utf-8")
        self.assertIn("IsAdvancedModel", header)
        body = without_comments(
            function_body(self.source, "void CEditorOverlaySettings::Rebuild(")
        )
        self.assertIn("ROLLER_ED_OVERLAY_TEST_CAR_ADVANCED", body)
        self.assertIn("IsAdvancedModel(m_carModel)", body)

    def test_million_plus_is_published_as_a_flag(self) -> None:
        body = without_comments(
            function_body(self.source, "void CEditorOverlaySettings::Rebuild(")
        )
        self.assertIn("ROLLER_ED_OVERLAY_TEST_CAR_MILLION_PLUS", body)
        self.assertIn("uiTestCarDesign", body)
        self.assertIn("uiTestCarAiLine", body)

    def test_the_translator_still_owns_no_qt_and_calls_no_facade(self) -> None:
        # Comments stripped: the header's own prose says "calls no RollerEd_*
        # function", which is the property being checked, not a violation.
        combined = without_comments(self.header) + without_comments(self.source)
        self.assertNotIn("RollerEd_", combined)
        self.assertNotIn("#include <Q", combined)


class PreviewTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.source = (EDITOR / "TrackPreview.cpp").read_text(encoding="utf-8")
        cls.header = (EDITOR / "TrackPreview.h").read_text(encoding="utf-8")

    def test_update_car_keeps_its_signature(self) -> None:
        self.assertIn(
            "void UpdateCar(eWhipModel carModel, eShapeSection aiLine, "
            "bool bMillionPlus);",
            self.header,
        )

    def test_showing_the_car_is_a_view_change_not_a_document_edit(self) -> None:
        body = without_comments(
            function_body(self.source, "void CTrackPreview::UpdateCar(")
        )
        self.assertIn("m_OverlaySettings.SetTestCar(", body)
        self.assertIn("ScheduleCameraRender();", body)
        # No revision bump, no serialize, no reload -- it rides the same
        # coalesced render-only path as the camera and ShowModels().
        self.assertNotIn("m_uiRevision", body)
        self.assertNotIn("GetTrackData", body)
        self.assertNotIn("SaveHistory", body)

    def test_an_unchanged_selection_does_not_re_render(self) -> None:
        # UpdateCar is called from the whole-settings push, so it arrives on
        # every unrelated checkbox change too.
        body = without_comments(
            function_body(self.source, "void CTrackPreview::UpdateCar(")
        )
        self.assertIn("m_carModel == carModel", body)
        self.assertIn("return;", body)


class BuildRegistrationTests(unittest.TestCase):
    def test_the_translation_unit_can_see_the_model_enum(self) -> None:
        cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
        block = cmake[
            cmake.index("trackeditor-e3a-s2-overlay-settings-test PRIVATE") :
        ]
        block = block[: block.index("add_test")]
        self.assertIn("/WhipLib", block)


if __name__ == "__main__":
    unittest.main()
