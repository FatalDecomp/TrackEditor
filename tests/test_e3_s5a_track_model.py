import pathlib
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[1]


class TrackModelContractTests(unittest.TestCase):
    def test_track_model_is_a_real_library_target(self):
        root_cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
        model_cmake = (ROOT / "TrackModel" / "CMakeLists.txt").read_text(encoding="utf-8")

        self.assertIn("add_subdirectory(TrackModel)", root_cmake)
        self.assertIn("add_library(track-model STATIC", model_cmake)
        self.assertIn("TrackEditor::track-model", model_cmake)
        self.assertNotIn("external/glm", model_cmake)

    def test_model_sources_have_no_geometry_or_asset_dependency(self):
        model_text = "\n".join(
            (ROOT / "TrackModel" / filename).read_text(encoding="utf-8")
            for filename in ("TrackModel.h", "TrackModel.cpp")
        )

        for forbidden in (
            "glm",
            '#include "Types.h"',
            "tChunkMath",
            "GenerateTrackMath",
            "MathHelpers",
            "Palette.h",
            "Texture.h",
            "ShapeFactory",
        ):
            self.assertNotIn(forbidden, model_text)

    def test_whiplib_layers_geometry_over_the_model(self):
        whiplib_cmake = (ROOT / "WhipLib" / "CMakeLists.txt").read_text(encoding="utf-8")
        track_header = (ROOT / "WhipLib" / "Track.h").read_text(encoding="utf-8")

        self.assertIn("TrackEditor::track-model", whiplib_cmake)
        self.assertIn("class CTrack : public CTrackModel", track_header)
        self.assertIn("CChunkMathAy m_chunkMathAy", track_header)

    def test_history_is_owned_by_track_model_not_qt_preview(self):
        model_header = (ROOT / "TrackModel" / "TrackModel.h").read_text(encoding="utf-8")
        preview_header = (ROOT / "TrackEditor" / "TrackPreview.h").read_text(encoding="utf-8")
        preview_source = (ROOT / "TrackEditor" / "TrackPreview.cpp").read_text(encoding="utf-8")

        self.assertIn("class CTrackHistory", model_header)
        self.assertNotIn("struct tTrackHistory", preview_header)
        self.assertNotIn("m_iHistoryIndex", preview_header)
        self.assertIn("CTrackHistory m_history", preview_source)


if __name__ == "__main__":
    unittest.main()
