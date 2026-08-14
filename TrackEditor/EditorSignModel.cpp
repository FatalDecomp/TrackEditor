#include "EditorSignModel.h"

bool CEditorSignModel::IsTower(int iSignType)
{
  return iSignType >= TOWER_TYPE_BASE;
}

bool CEditorSignModel::IsSign(int iSignType)
{
  return iSignType >= 0 && iSignType < TOWER_TYPE_BASE;
}

bool CEditorSignModel::IsKnownSignIndex(int iSignType, int iSignCount)
{
  return iSignType >= 0 && iSignType < iSignCount;
}

int CEditorSignModel::ApplyToRange(CChunkAy &chunkAy, int iFrom, int iTo,
                                   const tEditOperation &Operation)
{
  if (!Operation || iFrom < 0 || iTo < iFrom
      || iTo >= static_cast<int>(chunkAy.size())) {
    return 0;
  }

  int iEdited = 0;
  for (int i = iFrom; i <= iTo; ++i) {
    if (IsTower(chunkAy[i].iSignType))
      continue;
    Operation(chunkAy[i]);
    ++iEdited;
  }
  return iEdited;
}
