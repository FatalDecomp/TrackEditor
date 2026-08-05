import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
ROLLER = ROOT / "external" / "ROLLER"
ROLLER_SOURCE = ROLLER / "PROJECTS" / "ROLLER"


class CanonicalSurfaceEmitterIntegrationTests(unittest.TestCase):
    def test_pinned_core_contains_the_generic_callback_emitter(self) -> None:
        header = (ROLLER_SOURCE / "editor_surface.h").read_text(encoding="utf-8")
        source = (ROLLER_SOURCE / "editor_surface.c").read_text(encoding="utf-8")

        self.assertIn("tEdEmitSurfaceFn", header)
        self.assertIn("tEdSurfaceEmission", header)
        self.assertIn("bool ed_emit_surface(", header)
        self.assertIn("ed_surface_compute_render_uvs", header + source)
        self.assertNotIn("ed_emit_left_wall_surface", header + source)
        self.assertTrue(
            (ROLLER / "tests" / "test_e4a_s1_canonical_surface_emitter.py").is_file()
        )

    def test_rendering_consumes_every_canonical_surface_through_one_seam(self) -> None:
        draw = (ROLLER_SOURCE / "drawtrk3.c").read_text(encoding="utf-8")
        building = (ROLLER_SOURCE / "building.c").read_text(encoding="utf-8")
        tower = (ROLLER_SOURCE / "tower.c").read_text(encoding="utf-8")

        self.assertIn("draw_emitted_surface", draw)
        # E4A-S2 renamed this when the producer became the shared traversal.
        self.assertGreaterEqual(
            draw.count("emit_track_chunk_surface_to_renderer("), 12
        )
        # Two draw call sites since E3A-S2: the surface fill, and the editor's
        # edge pass over the same emission -- wireframe in E3A-S2, selection
        # outlines in E3A-S3. ROLLER's own suite pins which is which; here it
        # is enough that no third one appeared.
        self.assertEqual(draw.count("game_render_quad_world_subdivide_type("), 2)
        self.assertIn("draw_emitted_surface_edges", draw)
        self.assertIn("drawtrk3_emit_surface_to_renderer(", building)
        self.assertIn("drawtrk3_emit_surface_to_renderer(", tower)
        self.assertNotIn("game_render_quad_world_subdivide_type(", building)
        self.assertNotIn("game_render_quad_world(", tower)

        for identity in (
            "ROLLER_ED_SURFACE_CLASS_CENTER",
            "ROLLER_ED_SURFACE_CLASS_LEFT_SHOULDER",
            "ROLLER_ED_SURFACE_CLASS_RIGHT_SHOULDER",
            "ROLLER_ED_SURFACE_CLASS_LEFT_WALL",
            "ROLLER_ED_SURFACE_CLASS_RIGHT_WALL",
            "ROLLER_ED_SURFACE_CLASS_ROOF",
            "ROLLER_ED_SURFACE_CLASS_OUTER_WALL_FLOOR",
            "ROLLER_ED_SURFACE_CLASS_LEFT_LOWER_OUTER_WALL",
            "ROLLER_ED_SURFACE_CLASS_RIGHT_LOWER_OUTER_WALL",
            "ROLLER_ED_SURFACE_CLASS_LEFT_UPPER_OUTER_WALL",
            "ROLLER_ED_SURFACE_CLASS_RIGHT_UPPER_OUTER_WALL",
        ):
            self.assertIn(identity, draw)
        self.assertIn("ROLLER_ED_SURFACE_CLASS_SIGN", building)
        self.assertIn("ROLLER_ED_SURFACE_CLASS_BUILDING", building)
        self.assertIn("ROLLER_ED_SURFACE_CLASS_TOWER", tower)

    def test_document_model_and_assets_do_not_gain_geometry_authority(self) -> None:
        for relative_root in ("TrackModel", "TrackAssets"):
            combined = "\n".join(
                path.read_text(encoding="utf-8", errors="replace")
                for path in (ROOT / relative_root).rglob("*")
                if path.is_file()
            )
            self.assertNotIn("tEdSurfaceEmission", combined)
            self.assertNotIn("editor_surface.h", combined)
            self.assertNotIn("world_verts_", combined)


if __name__ == "__main__":
    unittest.main()
