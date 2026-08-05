#ifndef TRACKEDITOR_EDITOROVERLAYSETTINGS_H
#define TRACKEDITOR_EDITOROVERLAYSETTINGS_H

#include "editor_api.h"

#include <cstdint>

//-------------------------------------------------------------------------------------------------
// Translates the editor's own SHOW_* display bitmask into the facade's
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
  // iSelFrom/iSelTo are CTrackPreview's selection bounds; a negative bound
  // means nothing is selected.
  void SetSelectionRange(int iSelFrom, int iSelTo);

  uint32_t GetShowModels() const { return m_uiShowModels; }
  const tEdOverlayState &GetOverlayState() const { return m_Overlay; }

private:
  void Rebuild();

  uint32_t m_uiShowModels;
  // Until the window pushes its display settings, the preview shows what it
  // has always shown: every surface class solid, no wireframe. Deriving that
  // from a zero mask instead would blank the track for the first frames.
  bool m_bHasShowModels;
  int m_iSelFrom;
  int m_iSelTo;
  tEdOverlayState m_Overlay;
};

#endif
