#ifndef TRACKEDITOR_EDITORSIGNMODEL_H
#define TRACKEDITOR_EDITORSIGNMODEL_H

#include "TrackModel.h"

#include <functional>

// E7-S7. Sign and tower data share the same serialized columns, so every
// sign-side range edit goes through this gate rather than relying on a
// disabled widget to protect tower chunks.
class CEditorSignModel
{
public:
  static constexpr int TOWER_TYPE_BASE = 256;
  using tEditOperation = std::function<void(tGeometryChunk &)>;

  static bool IsTower(int iSignType);
  static bool IsSign(int iSignType);
  static bool IsKnownSignIndex(int iSignType, int iSignCount);
  static int ApplyToRange(CChunkAy &chunkAy, int iFrom, int iTo,
                          const tEditOperation &Operation);
};

#endif
