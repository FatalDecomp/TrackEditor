#include "EditorOverlaySettings.h"

#include "DisplaySettingsFlags.h"

namespace
{
//-------------------------------------------------------------------------------------------------
// One row per editor surface toggle pair. The eleven track classes are the
// ones DisplaySettings has always offered a surface and a wireframe checkbox
// for; SIGN follows the single SHOW_SIGNS checkbox, which the legacy editor
// applied to signs as a whole rather than per fill mode.
//-------------------------------------------------------------------------------------------------
struct tSurfaceClassToggle
{
  uint32_t uiSurfaceBit;
  uint32_t uiWireframeBit;
  uint32_t uiSurfaceClass;
};

const tSurfaceClassToggle g_aToggles[] = {
  { SHOW_CENTER_SURF_MODEL,     SHOW_CENTER_WIRE_MODEL,
    ROLLER_ED_SURFACE_CLASS_CENTER },
  { SHOW_LSHOULDER_SURF_MODEL,  SHOW_LSHOULDER_WIRE_MODEL,
    ROLLER_ED_SURFACE_CLASS_LEFT_SHOULDER },
  { SHOW_RSHOULDER_SURF_MODEL,  SHOW_RSHOULDER_WIRE_MODEL,
    ROLLER_ED_SURFACE_CLASS_RIGHT_SHOULDER },
  { SHOW_LWALL_SURF_MODEL,      SHOW_LWALL_WIRE_MODEL,
    ROLLER_ED_SURFACE_CLASS_LEFT_WALL },
  { SHOW_RWALL_SURF_MODEL,      SHOW_RWALL_WIRE_MODEL,
    ROLLER_ED_SURFACE_CLASS_RIGHT_WALL },
  { SHOW_ROOF_SURF_MODEL,       SHOW_ROOF_WIRE_MODEL,
    ROLLER_ED_SURFACE_CLASS_ROOF },
  { SHOW_OWALLFLOOR_SURF_MODEL, SHOW_OWALLFLOOR_WIRE_MODEL,
    ROLLER_ED_SURFACE_CLASS_OUTER_WALL_FLOOR },
  { SHOW_LLOWALL_SURF_MODEL,    SHOW_LLOWALL_WIRE_MODEL,
    ROLLER_ED_SURFACE_CLASS_LEFT_LOWER_OUTER_WALL },
  { SHOW_RLOWALL_SURF_MODEL,    SHOW_RLOWALL_WIRE_MODEL,
    ROLLER_ED_SURFACE_CLASS_RIGHT_LOWER_OUTER_WALL },
  { SHOW_LUOWALL_SURF_MODEL,    SHOW_LUOWALL_WIRE_MODEL,
    ROLLER_ED_SURFACE_CLASS_LEFT_UPPER_OUTER_WALL },
  { SHOW_RUOWALL_SURF_MODEL,    SHOW_RUOWALL_WIRE_MODEL,
    ROLLER_ED_SURFACE_CLASS_RIGHT_UPPER_OUTER_WALL },
  { SHOW_SIGNS,                 0u,
    ROLLER_ED_SURFACE_CLASS_SIGN }
};

//-------------------------------------------------------------------------------------------------
// Feature toggles that are not per surface class. Several of these are
// consumed by later Epic 3A stories; publishing them now costs nothing,
// because the core ignores a flag it has not implemented yet, and it keeps
// this translation in one place.
//-------------------------------------------------------------------------------------------------
struct tFeatureToggle
{
  uint32_t uiShowModelsBit;
  uint32_t uiOverlayFlag;
};

const tFeatureToggle g_aFeatures[] = {
  { SHOW_SELECTION_HIGHLIGHT, ROLLER_ED_OVERLAY_HIGHLIGHT_SELECTION },
  { SHOW_AILINE_MODELS,       ROLLER_ED_OVERLAY_SHOW_AI_LINES },
  { SHOW_ENVIRONMENT,         ROLLER_ED_OVERLAY_SHOW_ENVIRONMENT_FLOOR },
  { SHOW_TEST_CAR,            ROLLER_ED_OVERLAY_SHOW_TEST_CAR },
  { SHOW_AUDIO,               ROLLER_ED_OVERLAY_SHOW_AUDIO_MARKERS },
  { SHOW_STUNTS,              ROLLER_ED_OVERLAY_SHOW_STUNT_MARKERS },
  { SHOW_REF_MODEL,           ROLLER_ED_OVERLAY_SHOW_REFERENCE_MESH }
};
}

//-------------------------------------------------------------------------------------------------

CEditorOverlaySettings::CEditorOverlaySettings()
  : m_uiShowModels(0)
  , m_bHasShowModels(false)
  , m_iSelFrom(-1)
  , m_iSelTo(-1)
  , m_Overlay()
{
  Rebuild();
}

//-------------------------------------------------------------------------------------------------

void CEditorOverlaySettings::SetShowModels(uint32_t uiShowModels)
{
  m_uiShowModels = uiShowModels;
  m_bHasShowModels = true;
  Rebuild();
}

//-------------------------------------------------------------------------------------------------

void CEditorOverlaySettings::SetSelectionRange(int iSelFrom, int iSelTo)
{
  m_iSelFrom = iSelFrom;
  m_iSelTo = iSelTo;
  Rebuild();
}

//-------------------------------------------------------------------------------------------------

void CEditorOverlaySettings::Rebuild()
{
  uint32_t uiFlags = 0;
  uint32_t uiSurfaceClassMask = 0;
  uint32_t uiWireframeClassMask = 0;

  if (!m_bHasShowModels)
    uiSurfaceClassMask = ROLLER_ED_OVERLAY_ALL_SURFACE_CLASSES;

  for (const tSurfaceClassToggle &Toggle : g_aToggles) {
    if ((m_uiShowModels & Toggle.uiSurfaceBit) != 0)
      uiSurfaceClassMask |= ROLLER_ED_OVERLAY_CLASS_BIT(Toggle.uiSurfaceClass);
    if (Toggle.uiWireframeBit != 0
        && (m_uiShowModels & Toggle.uiWireframeBit) != 0)
      uiWireframeClassMask |= ROLLER_ED_OVERLAY_CLASS_BIT(Toggle.uiSurfaceClass);
  }

  // Buildings and towers have never had a checkbox: the legacy editor always
  // drew them. Keeping their class bits set preserves that, and leaves the
  // door open for a checkbox later without another ABI change.
  uiSurfaceClassMask |=
      ROLLER_ED_OVERLAY_CLASS_BIT(ROLLER_ED_SURFACE_CLASS_BUILDING)
      | ROLLER_ED_OVERLAY_CLASS_BIT(ROLLER_ED_SURFACE_CLASS_TOWER);

  // The masters are derived, not stored: unchecking every surface box means
  // the same thing as switching surfaces off, and it keeps the per-class
  // choices intact for when a box is checked again.
  if (uiSurfaceClassMask != 0)
    uiFlags |= ROLLER_ED_OVERLAY_SHOW_SURFACES;
  if (uiWireframeClassMask != 0)
    uiFlags |= ROLLER_ED_OVERLAY_SHOW_WIREFRAME;

  for (const tFeatureToggle &Feature : g_aFeatures) {
    if ((m_uiShowModels & Feature.uiShowModelsBit) != 0)
      uiFlags |= Feature.uiOverlayFlag;
  }

  m_Overlay = tEdOverlayState();
  m_Overlay.uiStructSize = sizeof(m_Overlay);
  m_Overlay.uiVersion = ROLLER_ED_OVERLAY_STATE_VERSION;
  m_Overlay.uiFlags = uiFlags;
  m_Overlay.uiFirstSelectedChunk = m_iSelFrom < 0
      ? ROLLER_ED_INVALID_CHUNK_ID : static_cast<uint32_t>(m_iSelFrom);
  m_Overlay.uiLastSelectedChunk = m_iSelTo < 0
      ? ROLLER_ED_INVALID_CHUNK_ID : static_cast<uint32_t>(m_iSelTo);
  m_Overlay.uiSurfaceClassMask = uiSurfaceClassMask;
  m_Overlay.uiWireframeClassMask = uiWireframeClassMask;
}
