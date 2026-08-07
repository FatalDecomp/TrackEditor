#include "EditorOverlaySettings.h"
#include "EditorReferenceMesh.h"
#include "DisplaySettingsFlags.h"

// CTest runs this in the Release configuration, whose NDEBUG would compile
// every assertion below out and leave the test passing by doing nothing.
// tests/track_model_test.cpp guards itself the same way.
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <cstdio>

namespace
{
struct tExpectedClass
{
  uint32_t uiSurfaceBit;
  uint32_t uiWireframeBit;
  uint32_t uiSurfaceClass;
  const char *szName;
};

// The full parity table: every checkbox pair the editor has ever had, and the
// canonical class it now selects.
const tExpectedClass g_aExpected[] = {
  { SHOW_CENTER_SURF_MODEL,     SHOW_CENTER_WIRE_MODEL,
    ROLLER_ED_SURFACE_CLASS_CENTER,                   "center" },
  { SHOW_LSHOULDER_SURF_MODEL,  SHOW_LSHOULDER_WIRE_MODEL,
    ROLLER_ED_SURFACE_CLASS_LEFT_SHOULDER,            "left shoulder" },
  { SHOW_RSHOULDER_SURF_MODEL,  SHOW_RSHOULDER_WIRE_MODEL,
    ROLLER_ED_SURFACE_CLASS_RIGHT_SHOULDER,           "right shoulder" },
  { SHOW_LWALL_SURF_MODEL,      SHOW_LWALL_WIRE_MODEL,
    ROLLER_ED_SURFACE_CLASS_LEFT_WALL,                "left wall" },
  { SHOW_RWALL_SURF_MODEL,      SHOW_RWALL_WIRE_MODEL,
    ROLLER_ED_SURFACE_CLASS_RIGHT_WALL,               "right wall" },
  { SHOW_ROOF_SURF_MODEL,       SHOW_ROOF_WIRE_MODEL,
    ROLLER_ED_SURFACE_CLASS_ROOF,                     "roof" },
  { SHOW_OWALLFLOOR_SURF_MODEL, SHOW_OWALLFLOOR_WIRE_MODEL,
    ROLLER_ED_SURFACE_CLASS_OUTER_WALL_FLOOR,         "outer wall floor" },
  { SHOW_LLOWALL_SURF_MODEL,    SHOW_LLOWALL_WIRE_MODEL,
    ROLLER_ED_SURFACE_CLASS_LEFT_LOWER_OUTER_WALL,    "left lower outer" },
  { SHOW_RLOWALL_SURF_MODEL,    SHOW_RLOWALL_WIRE_MODEL,
    ROLLER_ED_SURFACE_CLASS_RIGHT_LOWER_OUTER_WALL,   "right lower outer" },
  { SHOW_LUOWALL_SURF_MODEL,    SHOW_LUOWALL_WIRE_MODEL,
    ROLLER_ED_SURFACE_CLASS_LEFT_UPPER_OUTER_WALL,    "left upper outer" },
  { SHOW_RUOWALL_SURF_MODEL,    SHOW_RUOWALL_WIRE_MODEL,
    ROLLER_ED_SURFACE_CLASS_RIGHT_UPPER_OUTER_WALL,   "right upper outer" }
};

bool ClassInMask(uint32_t uiMask, uint32_t uiSurfaceClass)
{
  return (uiMask & ROLLER_ED_OVERLAY_CLASS_BIT(uiSurfaceClass)) != 0;
}

// Buildings and towers never had a checkbox and must stay visible.
const uint32_t g_uiAlwaysVisible =
    ROLLER_ED_OVERLAY_CLASS_BIT(ROLLER_ED_SURFACE_CLASS_BUILDING)
    | ROLLER_ED_OVERLAY_CLASS_BIT(ROLLER_ED_SURFACE_CLASS_TOWER);

void test_state_is_a_valid_request()
{
  CEditorOverlaySettings Settings;
  const tEdOverlayState &State = Settings.GetOverlayState();

  assert(State.uiStructSize == sizeof(tEdOverlayState));
  assert(State.uiVersion == ROLLER_ED_OVERLAY_STATE_VERSION);
  // 3 since E3A-S6 appended the test-car selection.
  assert(ROLLER_ED_OVERLAY_STATE_VERSION == 3u);
}

void test_untouched_settings_show_everything_solid()
{
  // The window pushes its display settings after the preview exists. Until it
  // does, the preview must show what it always showed rather than a mask
  // derived from zero, which would blank the track.
  CEditorOverlaySettings Settings;
  const tEdOverlayState &State = Settings.GetOverlayState();

  assert((State.uiFlags & ROLLER_ED_OVERLAY_SHOW_SURFACES) != 0);
  assert((State.uiFlags & ROLLER_ED_OVERLAY_SHOW_WIREFRAME) == 0);
  assert(State.uiSurfaceClassMask == ROLLER_ED_OVERLAY_ALL_SURFACE_CLASSES);
  assert(State.uiWireframeClassMask == 0);
  assert(State.uiFirstSelectedChunk == ROLLER_ED_INVALID_CHUNK_ID);
  assert(State.uiLastSelectedChunk == ROLLER_ED_INVALID_CHUNK_ID);
}

void test_each_checkbox_selects_exactly_its_own_class()
{
  for (const tExpectedClass &Expected : g_aExpected) {
    CEditorOverlaySettings SurfaceOnly;
    CEditorOverlaySettings WireframeOnly;

    SurfaceOnly.SetShowModels(Expected.uiSurfaceBit);
    const tEdOverlayState &Surface = SurfaceOnly.GetOverlayState();
    assert((Surface.uiFlags & ROLLER_ED_OVERLAY_SHOW_SURFACES) != 0);
    assert((Surface.uiFlags & ROLLER_ED_OVERLAY_SHOW_WIREFRAME) == 0);
    assert(Surface.uiSurfaceClassMask
           == (ROLLER_ED_OVERLAY_CLASS_BIT(Expected.uiSurfaceClass)
               | g_uiAlwaysVisible));
    assert(Surface.uiWireframeClassMask == 0);

    WireframeOnly.SetShowModels(Expected.uiWireframeBit);
    const tEdOverlayState &Wire = WireframeOnly.GetOverlayState();
    assert((Wire.uiFlags & ROLLER_ED_OVERLAY_SHOW_WIREFRAME) != 0);
    assert(Wire.uiWireframeClassMask
           == ROLLER_ED_OVERLAY_CLASS_BIT(Expected.uiSurfaceClass));
    // Only buildings and towers remain solid: no track class was checked.
    assert(Wire.uiSurfaceClassMask == g_uiAlwaysVisible);
  }
}

void test_surface_and_wireframe_stay_independent()
{
  CEditorOverlaySettings Settings;

  // Centre solid, roof outlined, right wall both.
  Settings.SetShowModels(SHOW_CENTER_SURF_MODEL | SHOW_ROOF_WIRE_MODEL
                         | SHOW_RWALL_SURF_MODEL | SHOW_RWALL_WIRE_MODEL);
  const tEdOverlayState &State = Settings.GetOverlayState();

  assert(ClassInMask(State.uiSurfaceClassMask,
                     ROLLER_ED_SURFACE_CLASS_CENTER));
  assert(!ClassInMask(State.uiWireframeClassMask,
                      ROLLER_ED_SURFACE_CLASS_CENTER));
  assert(!ClassInMask(State.uiSurfaceClassMask, ROLLER_ED_SURFACE_CLASS_ROOF));
  assert(ClassInMask(State.uiWireframeClassMask, ROLLER_ED_SURFACE_CLASS_ROOF));
  assert(ClassInMask(State.uiSurfaceClassMask,
                     ROLLER_ED_SURFACE_CLASS_RIGHT_WALL));
  assert(ClassInMask(State.uiWireframeClassMask,
                     ROLLER_ED_SURFACE_CLASS_RIGHT_WALL));
  assert(!ClassInMask(State.uiSurfaceClassMask,
                      ROLLER_ED_SURFACE_CLASS_LEFT_WALL));
  assert(!ClassInMask(State.uiWireframeClassMask,
                      ROLLER_ED_SURFACE_CLASS_LEFT_WALL));
  assert((State.uiFlags & ROLLER_ED_OVERLAY_SHOW_SURFACES) != 0);
  assert((State.uiFlags & ROLLER_ED_OVERLAY_SHOW_WIREFRAME) != 0);
}

void test_signs_follow_their_single_checkbox()
{
  CEditorOverlaySettings Off;
  CEditorOverlaySettings On;

  Off.SetShowModels(SHOW_CENTER_SURF_MODEL);
  assert(!ClassInMask(Off.GetOverlayState().uiSurfaceClassMask,
                      ROLLER_ED_SURFACE_CLASS_SIGN));

  On.SetShowModels(SHOW_CENTER_SURF_MODEL | SHOW_SIGNS);
  assert(ClassInMask(On.GetOverlayState().uiSurfaceClassMask,
                     ROLLER_ED_SURFACE_CLASS_SIGN));
  // Signs have one checkbox, not a fill/outline pair.
  assert(!ClassInMask(On.GetOverlayState().uiWireframeClassMask,
                      ROLLER_ED_SURFACE_CLASS_SIGN));
}

void test_buildings_and_towers_are_always_drawn()
{
  CEditorOverlaySettings Settings;

  Settings.SetShowModels(0);
  const tEdOverlayState &State = Settings.GetOverlayState();
  assert(State.uiSurfaceClassMask == g_uiAlwaysVisible);
  // Even with every box cleared, the master stays on for them.
  assert((State.uiFlags & ROLLER_ED_OVERLAY_SHOW_SURFACES) != 0);
  assert((State.uiFlags & ROLLER_ED_OVERLAY_SHOW_WIREFRAME) == 0);
}

void test_feature_toggles_map_to_overlay_flags()
{
  struct tCase
  {
    uint32_t uiShowModelsBit;
    uint32_t uiOverlayFlag;
  };
  static const tCase aCases[] = {
    { SHOW_SELECTION_HIGHLIGHT, ROLLER_ED_OVERLAY_HIGHLIGHT_SELECTION },
    { SHOW_AILINE_MODELS,       ROLLER_ED_OVERLAY_SHOW_AI_LINES },
    { SHOW_TEST_CAR,            ROLLER_ED_OVERLAY_SHOW_TEST_CAR },
    { SHOW_AUDIO,               ROLLER_ED_OVERLAY_SHOW_AUDIO_MARKERS },
    { SHOW_STUNTS,              ROLLER_ED_OVERLAY_SHOW_STUNT_MARKERS },
    { SHOW_REF_MODEL,           ROLLER_ED_OVERLAY_SHOW_REFERENCE_MESH }
  };

  for (const tCase &Case : aCases) {
    CEditorOverlaySettings Settings;
    Settings.SetShowModels(Case.uiShowModelsBit);
    assert((Settings.GetOverlayState().uiFlags & Case.uiOverlayFlag) != 0);

    CEditorOverlaySettings Without;
    Without.SetShowModels(SHOW_CENTER_SURF_MODEL);
    assert((Without.GetOverlayState().uiFlags & Case.uiOverlayFlag) == 0);
  }
}

void test_no_undefined_flag_or_class_bit_is_ever_published()
{
  // The facade refuses an unknown flag or class bit outright, so a mask this
  // translation produces must never contain one -- including for the two
  // legacy bits (reference-model wireframe, and any future bit) that have no
  // overlay flag yet.
  for (uint32_t uiBit = 0; uiBit < 32u; uiBit++) {
    CEditorOverlaySettings Settings;
    Settings.SetShowModels(1u << uiBit);
    const tEdOverlayState &State = Settings.GetOverlayState();

    assert((State.uiSurfaceClassMask
            & ~static_cast<uint32_t>(ROLLER_ED_OVERLAY_ALL_SURFACE_CLASSES))
           == 0);
    assert((State.uiWireframeClassMask
            & ~static_cast<uint32_t>(ROLLER_ED_OVERLAY_ALL_SURFACE_CLASSES))
           == 0);
    assert((State.uiFlags & ~static_cast<uint32_t>(
                ROLLER_ED_OVERLAY_SHOW_SURFACES
                | ROLLER_ED_OVERLAY_SHOW_WIREFRAME
                | ROLLER_ED_OVERLAY_HIGHLIGHT_SELECTION
                | ROLLER_ED_OVERLAY_SHOW_AI_LINES
                | ROLLER_ED_OVERLAY_SHOW_CENTER_LINE
                | ROLLER_ED_OVERLAY_SHOW_AUDIO_MARKERS
                | ROLLER_ED_OVERLAY_SHOW_STUNT_MARKERS
                | ROLLER_ED_OVERLAY_SHOW_TEST_CAR
                | ROLLER_ED_OVERLAY_SHOW_REFERENCE_MESH
                | ROLLER_ED_OVERLAY_TEST_CAR_MILLION_PLUS
                | ROLLER_ED_OVERLAY_TEST_CAR_ADVANCED)) == 0);
  }
}

void test_selection_range_uses_the_sentinel_for_no_selection()
{
  CEditorOverlaySettings Settings;

  Settings.SetShowModels(SHOW_CENTER_SURF_MODEL | SHOW_SELECTION_HIGHLIGHT);
  Settings.SetSelectionRange(4, 9);
  assert(Settings.GetOverlayState().uiFirstSelectedChunk == 4u);
  assert(Settings.GetOverlayState().uiLastSelectedChunk == 9u);

  // CTrackPreview uses -1 for "nothing selected"; the facade uses its own
  // sentinel, and the two must not be confused for chunk zero.
  Settings.SetSelectionRange(-1, -1);
  assert(Settings.GetOverlayState().uiFirstSelectedChunk
         == ROLLER_ED_INVALID_CHUNK_ID);
  assert(Settings.GetOverlayState().uiLastSelectedChunk
         == ROLLER_ED_INVALID_CHUNK_ID);

  // A reversed range is passed through; the core orders it.
  Settings.SetSelectionRange(12, 3);
  assert(Settings.GetOverlayState().uiFirstSelectedChunk == 12u);
  assert(Settings.GetOverlayState().uiLastSelectedChunk == 3u);

  // Changing the display mask must not disturb the selection.
  Settings.SetShowModels(SHOW_ROOF_WIRE_MODEL);
  assert(Settings.GetOverlayState().uiFirstSelectedChunk == 12u);
  assert(Settings.GetOverlayState().uiLastSelectedChunk == 3u);
}

// E3A-S6 -------------------------------------------------------------------

// Every car model the combo box offers, and the ROLLER design it selects. The
// X and Y variants of a car are one design: WhipLib's own GetCoords returned
// identical geometry for both, so the pair was always a texture distinction.
struct tExpectedCar
{
  eWhipModel model;
  uint32_t uiDesign;
  bool bAdvanced;
  const char *szName;
};

const tExpectedCar g_aExpectedCars[] = {
  { eWhipModel::CAR_F1WACK, 12u, false, "f1wack" },
  { eWhipModel::CAR_XAUTO, 0u, false, "xauto" },
  { eWhipModel::CAR_XDESILVA, 1u, false, "xdesilva" },
  { eWhipModel::CAR_XPULSE, 2u, false, "xpulse" },
  { eWhipModel::CAR_XGLOBAL, 3u, false, "xglobal" },
  { eWhipModel::CAR_XMILLION, 4u, false, "xmillion" },
  { eWhipModel::CAR_XMISSION, 5u, false, "xmission" },
  { eWhipModel::CAR_XZIZIN, 6u, false, "xzizin" },
  { eWhipModel::CAR_XREISE, 7u, false, "xreise" },
  { eWhipModel::CAR_YAUTO, 0u, true, "yauto" },
  { eWhipModel::CAR_YDESILVA, 1u, true, "ydesilva" },
  { eWhipModel::CAR_YPULSE, 2u, true, "ypulse" },
  { eWhipModel::CAR_YGLOBAL, 3u, true, "yglobal" },
  { eWhipModel::CAR_YMILLION, 4u, true, "ymillion" },
  { eWhipModel::CAR_YMISSION, 5u, true, "ymission" },
  { eWhipModel::CAR_YZIZIN, 6u, true, "yzizin" },
  { eWhipModel::CAR_YREISE, 7u, true, "yreise" },
  { eWhipModel::CAR_DEATH, 13u, false, "death" }
};

void test_every_car_model_maps_to_a_design_in_range()
{
  for (const tExpectedCar &Expected : g_aExpectedCars) {
    const uint32_t uiDesign =
        CEditorOverlaySettings::CarDesignForModel(Expected.model);

    assert(uiDesign == Expected.uiDesign);
    // The Y variants share their twin's plan but use the advanced-cars skin.
    assert(CEditorOverlaySettings::IsAdvancedModel(Expected.model)
           == Expected.bAdvanced);
    // The facade refuses the whole overlay push for an out-of-range design,
    // which would take every other toggle down with it.
    assert(uiDesign < ROLLER_ED_TEST_CAR_DESIGN_COUNT);
    (void)Expected.szName;
  }
}

void test_a_non_car_model_still_yields_a_valid_design()
{
  // The combo box only offers cars, so this is a caller bug -- but it must
  // not be one that fails the entire overlay.
  const uint32_t uiDesign =
      CEditorOverlaySettings::CarDesignForModel(eWhipModel::SIGN_TREE);

  assert(uiDesign < ROLLER_ED_TEST_CAR_DESIGN_COUNT);
  assert(!CEditorOverlaySettings::IsAdvancedModel(eWhipModel::SIGN_TREE));
}

void test_the_y_variants_publish_the_advanced_skin()
{
  CEditorOverlaySettings Settings;

  Settings.SetShowModels(SHOW_CENTER_SURF_MODEL | SHOW_TEST_CAR);
  Settings.SetTestCar(eWhipModel::CAR_XZIZIN, eShapeSection::AILINE1, false);
  assert(Settings.GetOverlayState().uiTestCarDesign == 6u);
  assert((Settings.GetOverlayState().uiFlags
          & ROLLER_ED_OVERLAY_TEST_CAR_ADVANCED) == 0);

  // Same plan, different skin: the design must not move with it.
  Settings.SetTestCar(eWhipModel::CAR_YZIZIN, eShapeSection::AILINE1, false);
  assert(Settings.GetOverlayState().uiTestCarDesign == 6u);
  assert((Settings.GetOverlayState().uiFlags
          & ROLLER_ED_OVERLAY_TEST_CAR_ADVANCED) != 0);

  // F1WACK and DEATH have no Y variant and never ask for the skin.
  Settings.SetTestCar(eWhipModel::CAR_DEATH, eShapeSection::AILINE1, false);
  assert((Settings.GetOverlayState().uiFlags
          & ROLLER_ED_OVERLAY_TEST_CAR_ADVANCED) == 0);
}

void test_both_ai_line_spellings_reach_the_same_index()
{
  // AILINE* is the visualized line and CARLINE* the surface-level one; ROLLER
  // derives the car's position from localdata[].fAILine1..4 either way.
  assert(CEditorOverlaySettings::AiLineForSection(eShapeSection::AILINE1) == 0u);
  assert(CEditorOverlaySettings::AiLineForSection(eShapeSection::AILINE2) == 1u);
  assert(CEditorOverlaySettings::AiLineForSection(eShapeSection::AILINE3) == 2u);
  assert(CEditorOverlaySettings::AiLineForSection(eShapeSection::AILINE4) == 3u);
  assert(CEditorOverlaySettings::AiLineForSection(eShapeSection::CARLINE1) == 0u);
  assert(CEditorOverlaySettings::AiLineForSection(eShapeSection::CARLINE4) == 3u);
  // Anything else is a caller bug and must still be in range.
  assert(CEditorOverlaySettings::AiLineForSection(eShapeSection::ROOF)
         < ROLLER_ED_TEST_CAR_AI_LINE_COUNT);
}

void test_the_test_car_selection_reaches_the_overlay()
{
  CEditorOverlaySettings Settings;

  Settings.SetShowModels(SHOW_CENTER_SURF_MODEL | SHOW_TEST_CAR);
  Settings.SetTestCar(eWhipModel::CAR_XMILLION, eShapeSection::AILINE3, false);
  assert((Settings.GetOverlayState().uiFlags
          & ROLLER_ED_OVERLAY_SHOW_TEST_CAR) != 0);
  assert(Settings.GetOverlayState().uiTestCarDesign == 4u);
  assert(Settings.GetOverlayState().uiTestCarAiLine == 2u);
  assert((Settings.GetOverlayState().uiFlags
          & ROLLER_ED_OVERLAY_TEST_CAR_MILLION_PLUS) == 0);

  // Million Plus is a modifier flag, not a separate overlay.
  Settings.SetTestCar(eWhipModel::CAR_XMILLION, eShapeSection::AILINE3, true);
  assert((Settings.GetOverlayState().uiFlags
          & ROLLER_ED_OVERLAY_TEST_CAR_MILLION_PLUS) != 0);
  assert((Settings.GetOverlayState().uiFlags
          & ROLLER_ED_OVERLAY_SHOW_TEST_CAR) != 0);
}

void test_the_car_selection_survives_the_show_car_checkbox()
{
  // Unticking Test Car must not lose which car was chosen, or reticking it
  // would silently reset the combo box.
  CEditorOverlaySettings Settings;

  Settings.SetTestCar(eWhipModel::CAR_DEATH, eShapeSection::AILINE4, true);
  Settings.SetShowModels(SHOW_CENTER_SURF_MODEL);
  assert((Settings.GetOverlayState().uiFlags
          & ROLLER_ED_OVERLAY_SHOW_TEST_CAR) == 0);
  assert(Settings.GetOverlayState().uiTestCarDesign == 13u);
  assert(Settings.GetOverlayState().uiTestCarAiLine == 3u);

  Settings.SetShowModels(SHOW_CENTER_SURF_MODEL | SHOW_TEST_CAR);
  assert(Settings.GetOverlayState().uiTestCarDesign == 13u);
  assert(Settings.GetOverlayState().uiTestCarAiLine == 3u);
}

void test_an_untouched_translator_publishes_a_valid_car()
{
  // The state is pushed before the window ever calls UpdateCar, so the
  // defaults have to pass the facade's range check on their own.
  CEditorOverlaySettings Settings;

  assert(Settings.GetOverlayState().uiTestCarDesign
         < ROLLER_ED_TEST_CAR_DESIGN_COUNT);
  assert(Settings.GetOverlayState().uiTestCarAiLine
         < ROLLER_ED_TEST_CAR_AI_LINE_COUNT);
}

// E3A-S7 -------------------------------------------------------------------

void test_an_empty_reference_mesh_draws_nothing()
{
  CEditorReferenceMesh Mesh;

  assert(!Mesh.HasMesh());
  const tEdReferenceMesh State = Mesh.GetMesh();
  assert(State.uiStructSize == sizeof(tEdReferenceMesh));
  assert(State.uiVersion == ROLLER_ED_REFERENCE_MESH_VERSION);
  // AD-13: a null vertex pointer is how the host says "clear it", and the
  // facade must be able to validate the struct either way.
  assert(State.pVertices == nullptr);
  assert(State.uiVertexCount == 0);
}

void test_geometry_is_copied_and_clearable()
{
  tEdReferenceVertex aVertices[3] = {};
  const uint32_t auiIndices[3] = { 0u, 1u, 2u };
  CEditorReferenceMesh Mesh;

  aVertices[0].fPosition[0] = 5.0f;
  Mesh.SetGeometry(aVertices, 3, auiIndices, 3);
  assert(Mesh.HasMesh());
  assert(Mesh.VertexCount() == 3);
  assert(Mesh.IndexCount() == 3);

  // The caller's array is not retained.
  aVertices[0].fPosition[0] = 99.0f;
  assert(Mesh.GetMesh().pVertices[0].fPosition[0] == 5.0f);

  Mesh.SetGeometry(nullptr, 0, nullptr, 0);
  assert(!Mesh.HasMesh());
  assert(Mesh.IndexCount() == 0);
}

void test_a_missing_index_array_means_a_plain_triangle_list()
{
  tEdReferenceVertex aVertices[3] = {};
  CEditorReferenceMesh Mesh;

  Mesh.SetGeometry(aVertices, 3, nullptr, 0);
  const tEdReferenceMesh State = Mesh.GetMesh();
  // AD-13: null indices mean non-indexed, and the core synthesizes them.
  assert(State.puiIndices == nullptr);
  assert(State.uiIndexCount == 0);
  assert(State.uiVertexCount == 3);
}

void test_the_transform_reaches_the_mesh()
{
  tEdReferenceVertex aVertices[3] = {};
  CEditorReferenceMesh Mesh;

  Mesh.SetGeometry(aVertices, 3, nullptr, 0);
  Mesh.SetTransform(10.0, 20.0, 30.0, 1, 2, 3, 4.0);
  const tEdReferenceMesh State = Mesh.GetMesh();
  assert(State.fRotation[0] == 10.0f);
  assert(State.fRotation[1] == 20.0f);
  assert(State.fRotation[2] == 30.0f);
  assert(State.fPosition[0] == 1.0f);
  assert(State.fPosition[1] == 2.0f);
  assert(State.fPosition[2] == 3.0f);
  // The dialog offers one scale box, so the mesh scales uniformly.
  assert(State.fScale[0] == 4.0f);
  assert(State.fScale[1] == 4.0f);
  assert(State.fScale[2] == 4.0f);
}

void test_a_zero_scale_does_not_collapse_the_mesh()
{
  tEdReferenceVertex aVertices[3] = {};
  CEditorReferenceMesh Mesh;

  Mesh.SetGeometry(aVertices, 3, nullptr, 0);
  Mesh.SetTransform(0.0, 0.0, 0.0, 0, 0, 0, 0.0);
  assert(Mesh.GetMesh().fScale[0] == 1.0f);
}

void test_wireframe_is_a_mesh_flag_not_an_overlay_flag()
{
  tEdReferenceVertex aVertices[3] = {};
  CEditorReferenceMesh Mesh;

  Mesh.SetGeometry(aVertices, 3, nullptr, 0);
  assert((Mesh.GetMesh().uiFlags & ROLLER_ED_REFERENCE_WIREFRAME) == 0);
  // Normals come from the importer, so the core does not regenerate them.
  assert((Mesh.GetMesh().uiFlags & ROLLER_ED_REFERENCE_HAS_NORMALS) != 0);

  Mesh.SetWireframe(true);
  assert((Mesh.GetMesh().uiFlags & ROLLER_ED_REFERENCE_WIREFRAME) != 0);
  assert((Mesh.GetMesh().uiFlags & ROLLER_ED_REFERENCE_HAS_NORMALS) != 0);

  // It is not one of the SHOW_* overlay flags: the reference mesh's own
  // wireframe lives on the mesh, unlike the per-class surface wireframes.
  CEditorOverlaySettings Settings;
  Settings.SetShowModels(SHOW_REF_MODEL | SHOW_REF_WIRE_MODEL);
  assert((Settings.GetOverlayState().uiFlags
          & ROLLER_ED_OVERLAY_SHOW_REFERENCE_MESH) != 0);
}
}

int main()
{
  test_state_is_a_valid_request();
  test_untouched_settings_show_everything_solid();
  test_each_checkbox_selects_exactly_its_own_class();
  test_surface_and_wireframe_stay_independent();
  test_signs_follow_their_single_checkbox();
  test_buildings_and_towers_are_always_drawn();
  test_feature_toggles_map_to_overlay_flags();
  test_no_undefined_flag_or_class_bit_is_ever_published();
  test_selection_range_uses_the_sentinel_for_no_selection();
  test_every_car_model_maps_to_a_design_in_range();
  test_a_non_car_model_still_yields_a_valid_design();
  test_the_y_variants_publish_the_advanced_skin();
  test_both_ai_line_spellings_reach_the_same_index();
  test_the_test_car_selection_reaches_the_overlay();
  test_the_car_selection_survives_the_show_car_checkbox();
  test_an_untouched_translator_publishes_a_valid_car();
  test_an_empty_reference_mesh_draws_nothing();
  test_geometry_is_copied_and_clearable();
  test_a_missing_index_array_means_a_plain_triangle_list();
  test_the_transform_reaches_the_mesh();
  test_a_zero_scale_does_not_collapse_the_mesh();
  test_wireframe_is_a_mesh_flag_not_an_overlay_flag();
  std::puts("editor overlay settings translation tests passed");
  return 0;
}
