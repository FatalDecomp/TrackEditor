#ifndef TRACKEDITOR_EDITOROVERLAYSETTINGS_H
#define TRACKEDITOR_EDITOROVERLAYSETTINGS_H

#include "Types.h"
#include "editor_api.h"

#include <cstdint>

//-------------------------------------------------------------------------------------------------
// Translates the editor's own persisted SHOW_* display words into the facade's
// tEdOverlayState (E3A-S2).
//
// The two do not correspond one to one on purpose. The legacy mask names a
// pair of bits per editor model -- "left wall surface", "left wall wireframe"
// -- because the deleted WhipLib renderer built one GL model per surface
// class. ROLLER instead publishes a canonical eRollerEdSurfaceClass on every
// emitted surface, so the same choice becomes a master switch plus one class
// bit. The legacy bit layout stops here; nothing downstream sees it.
//
// This class owns no Qt types and calls no RollerEd_* function, so the same
// translation is used by the UI and by the tests without a render worker.
//-------------------------------------------------------------------------------------------------

class CEditorOverlaySettings
{
public:
  CEditorOverlaySettings();

  // uiShowModels is the editor's DisplaySettings.h SHOW_* bitmask.
  void SetShowModels(uint32_t uiShowModels);
  // uiShowFeatures is the extensible show_features word. It is intentionally
  // separate because every bit in the legacy show_models word is allocated.
  void SetShowFeatures(uint32_t uiShowFeatures);
  // iSelFrom/iSelTo are CTrackPreview's selection bounds; a negative bound
  // means nothing is selected. The car also stands on iSelFrom, which is
  // where the legacy editor drew it, so no separate call sets its chunk.
  void SetSelectionRange(int iSelFrom, int iSelTo);
  // The UpdateCar(eWhipModel, eShapeSection, bool) triple, unchanged (E3A-S6).
  void SetTestCar(eWhipModel carModel, eShapeSection aiLine, bool bMillionPlus);
  // True draws the closing segment from the last chunk back to chunk zero.
  void SetAttachLast(bool bAttachLast);

  uint32_t GetShowModels() const { return m_uiShowModels; }
  uint32_t GetShowFeatures() const { return m_uiShowFeatures; }
  const tEdOverlayState &GetOverlayState() const { return m_Overlay; }

  // ROLLER's own CAR_DESIGN_* index for an editor model, and the 0-based AI
  // line index for an eShapeSection. Public so the parity table can be tested
  // without reaching into Rebuild().
  static uint32_t CarDesignForModel(eWhipModel carModel);
  static uint32_t AiLineForSection(eShapeSection aiLine);
  // True for the Y model variants, which share a plan with their X twin but
  // use ROLLER's advanced-cars skin: the second texture bank plus the palette
  // remap that recolours parts like the mirrors.
  static bool IsAdvancedModel(eWhipModel carModel);

private:
  void Rebuild();

  uint32_t m_uiShowModels;
  uint32_t m_uiShowFeatures;
  // Until the window pushes its display settings, the preview shows what it
  // has always shown: every surface class solid, no wireframe. Deriving that
  // from a zero mask instead would blank the track for the first frames.
  bool m_bHasShowModels;
  bool m_bAttachLast;
  int m_iSelFrom;
  int m_iSelTo;
  eWhipModel m_carModel;
  eShapeSection m_carAILine;
  bool m_bMillionPlus;
  tEdOverlayState m_Overlay;
};

#endif
