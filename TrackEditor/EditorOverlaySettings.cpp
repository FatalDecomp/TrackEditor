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

//-------------------------------------------------------------------------------------------------
// E3A-S6. ROLLER's CAR_DESIGN_* index for each editor car model.
//
// The values mirror ROLLER's types.h, which is an internal header the editor
// does not see -- editor_api.h publishes ROLLER_ED_TEST_CAR_DESIGN_COUNT but
// not the names. The test asserts every entry is in range and that the table
// covers every model, so a change to the size of ROLLER's table is caught;
// a reordering of it would not be, which is why the values are spelled out
// here with their names rather than computed.
//
// The X and Y model pairs collapse onto one design. WhipLib's own GetCoords
// already returned identical geometry for CAR_XAUTO and CAR_YAUTO: the pair
// is a texture-variant distinction, which ROLLER expresses through the
// design's carType and its advanced-cars texture switch, not a second plan.
//-------------------------------------------------------------------------------------------------
struct tCarModelDesign
{
  eWhipModel model;
  uint32_t uiDesign;
};

const uint32_t ROLLER_CAR_DESIGN_AUTO = 0u;
const uint32_t ROLLER_CAR_DESIGN_DESILVA = 1u;
const uint32_t ROLLER_CAR_DESIGN_PULSE = 2u;
const uint32_t ROLLER_CAR_DESIGN_GLOBAL = 3u;
const uint32_t ROLLER_CAR_DESIGN_MILLION = 4u;
const uint32_t ROLLER_CAR_DESIGN_MISSION = 5u;
const uint32_t ROLLER_CAR_DESIGN_ZIZIN = 6u;
const uint32_t ROLLER_CAR_DESIGN_REISE = 7u;
const uint32_t ROLLER_CAR_DESIGN_F1WACK = 12u;
const uint32_t ROLLER_CAR_DESIGN_DEATH = 13u;

const tCarModelDesign g_aCarDesigns[] = {
  { eWhipModel::CAR_F1WACK,   ROLLER_CAR_DESIGN_F1WACK },
  { eWhipModel::CAR_XAUTO,    ROLLER_CAR_DESIGN_AUTO },
  { eWhipModel::CAR_XDESILVA, ROLLER_CAR_DESIGN_DESILVA },
  { eWhipModel::CAR_XPULSE,   ROLLER_CAR_DESIGN_PULSE },
  { eWhipModel::CAR_XGLOBAL,  ROLLER_CAR_DESIGN_GLOBAL },
  { eWhipModel::CAR_XMILLION, ROLLER_CAR_DESIGN_MILLION },
  { eWhipModel::CAR_XMISSION, ROLLER_CAR_DESIGN_MISSION },
  { eWhipModel::CAR_XZIZIN,   ROLLER_CAR_DESIGN_ZIZIN },
  { eWhipModel::CAR_XREISE,   ROLLER_CAR_DESIGN_REISE },
  { eWhipModel::CAR_YAUTO,    ROLLER_CAR_DESIGN_AUTO },
  { eWhipModel::CAR_YDESILVA, ROLLER_CAR_DESIGN_DESILVA },
  { eWhipModel::CAR_YPULSE,   ROLLER_CAR_DESIGN_PULSE },
  { eWhipModel::CAR_YGLOBAL,  ROLLER_CAR_DESIGN_GLOBAL },
  { eWhipModel::CAR_YMILLION, ROLLER_CAR_DESIGN_MILLION },
  { eWhipModel::CAR_YMISSION, ROLLER_CAR_DESIGN_MISSION },
  { eWhipModel::CAR_YZIZIN,   ROLLER_CAR_DESIGN_ZIZIN },
  { eWhipModel::CAR_YREISE,   ROLLER_CAR_DESIGN_REISE },
  { eWhipModel::CAR_DEATH,    ROLLER_CAR_DESIGN_DEATH }
};

const tFeatureToggle g_aFeatures[] = {
  { SHOW_SELECTION_HIGHLIGHT, ROLLER_ED_OVERLAY_HIGHLIGHT_SELECTION },
  { SHOW_AILINE_MODELS,       ROLLER_ED_OVERLAY_SHOW_AI_LINES },
  { SHOW_CENTER_LINE,         ROLLER_ED_OVERLAY_SHOW_CENTER_LINE },
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
  // The same model CTrackPreview has always defaulted to, so the first frame
  // after a tick of Test Car shows what the legacy editor showed.
  , m_carModel(eWhipModel::CAR_XZIZIN)
  , m_carAILine(eShapeSection::AILINE1)
  , m_bMillionPlus(false)
  , m_Overlay()
{
  Rebuild();
}

//-------------------------------------------------------------------------------------------------

uint32_t CEditorOverlaySettings::CarDesignForModel(eWhipModel carModel)
{
  for (const tCarModelDesign &Design : g_aCarDesigns) {
    if (Design.model == carModel)
      return Design.uiDesign;
  }
  // A sign or building model is not a car. The combo box only offers cars, so
  // this is a caller bug; answering with a valid design keeps it out of the
  // facade's range check, where it would fail the whole overlay push and take
  // every other toggle down with it.
  return ROLLER_CAR_DESIGN_AUTO;
}

//-------------------------------------------------------------------------------------------------

uint32_t CEditorOverlaySettings::AiLineForSection(eShapeSection aiLine)
{
  switch (aiLine) {
    case eShapeSection::AILINE1: case eShapeSection::CARLINE1: return 0u;
    case eShapeSection::AILINE2: case eShapeSection::CARLINE2: return 1u;
    case eShapeSection::AILINE3: case eShapeSection::CARLINE3: return 2u;
    case eShapeSection::AILINE4: case eShapeSection::CARLINE4: return 3u;
    default: break;
  }
  // The visualized AILINE* and the surface-level CARLINE* are the same four
  // lines; ROLLER derives the car's position from localdata[].fAILine1..4
  // either way, so both spellings land on the same index.
  return 0u;
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

void CEditorOverlaySettings::SetTestCar(eWhipModel carModel,
                                        eShapeSection aiLine,
                                        bool bMillionPlus)
{
  m_carModel = carModel;
  m_carAILine = aiLine;
  m_bMillionPlus = bMillionPlus;
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
  // Million Plus is a modifier on the car rather than an overlay of its own,
  // so it is published as a flag and only means anything alongside
  // SHOW_TEST_CAR. It is the checkbox the legacy editor persisted under the
  // key "wrong_way": the car faces back down the track.
  if (m_bMillionPlus)
    m_Overlay.uiFlags |= ROLLER_ED_OVERLAY_TEST_CAR_MILLION_PLUS;
  m_Overlay.uiTestCarDesign = CarDesignForModel(m_carModel);
  m_Overlay.uiTestCarAiLine = AiLineForSection(m_carAILine);
}
