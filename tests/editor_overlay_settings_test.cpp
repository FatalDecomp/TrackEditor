#include "EditorOverlaySettings.h"
#include "DisplaySettingsFlags.h"

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

void test_state_is_a_valid_v2_request()
{
  CEditorOverlaySettings Settings;
  const tEdOverlayState &State = Settings.GetOverlayState();

  assert(State.uiStructSize == sizeof(tEdOverlayState));
  assert(State.uiVersion == ROLLER_ED_OVERLAY_STATE_VERSION);
  assert(ROLLER_ED_OVERLAY_STATE_VERSION == 2u);
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
    { SHOW_ENVIRONMENT,         ROLLER_ED_OVERLAY_SHOW_ENVIRONMENT_FLOOR },
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
                | ROLLER_ED_OVERLAY_SHOW_ENVIRONMENT_FLOOR
                | ROLLER_ED_OVERLAY_SHOW_AUDIO_MARKERS
                | ROLLER_ED_OVERLAY_SHOW_STUNT_MARKERS
                | ROLLER_ED_OVERLAY_SHOW_TEST_CAR
                | ROLLER_ED_OVERLAY_SHOW_REFERENCE_MESH)) == 0);
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
}

int main()
{
  test_state_is_a_valid_v2_request();
  test_untouched_settings_show_everything_solid();
  test_each_checkbox_selects_exactly_its_own_class();
  test_surface_and_wireframe_stay_independent();
  test_signs_follow_their_single_checkbox();
  test_buildings_and_towers_are_always_drawn();
  test_feature_toggles_map_to_overlay_flags();
  test_no_undefined_flag_or_class_bit_is_ever_published();
  test_selection_range_uses_the_sentinel_for_no_selection();
  std::puts("editor overlay settings translation tests passed");
  return 0;
}
