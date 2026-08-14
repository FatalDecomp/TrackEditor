#include "EditorTowerModel.h"

#include <algorithm>

//-------------------------------------------------------------------------------------------------

bool CEditorTowerModel::IsTower(int iSignType)
{
  return iSignType >= TOWER_TYPE_BASE;
}

//-------------------------------------------------------------------------------------------------

bool CEditorTowerModel::IsSign(int iSignType)
{
  return iSignType >= 0 && iSignType < TOWER_TYPE_BASE;
}

//-------------------------------------------------------------------------------------------------

eEditorTowerMode CEditorTowerModel::DecodeMode(int iSignType)
{
  switch (iSignType & 0xFF0F) {
    case 0x101: return eEditorTowerMode::TRACK_SURFACE_TWO_BACK;
    case 0x103: return eEditorTowerMode::FOLLOW_NEAR;
    case 0x104: return eEditorTowerMode::OVERHEAD_FOLLOW;
    case 0x105: return eEditorTowerMode::FOLLOW_AT_DISTANCE;
    default:    return eEditorTowerMode::STATIC;
  }
}

//-------------------------------------------------------------------------------------------------

int CEditorTowerModel::DecodeZoom(int iSignType)
{
  if (!IsTower(iSignType))
    return 0;
  const int iZoom = (iSignType - TOWER_TYPE_BASE) / 16;
  return iZoom >= 1 && iZoom <= MAX_CANONICAL_ZOOM ? iZoom : 0;
}

//-------------------------------------------------------------------------------------------------

int CEditorTowerModel::ModeNibble(eEditorTowerMode mode)
{
  switch (mode) {
    case eEditorTowerMode::TRACK_SURFACE_TWO_BACK: return 1;
    case eEditorTowerMode::FOLLOW_NEAR:            return 3;
    case eEditorTowerMode::OVERHEAD_FOLLOW:        return 4;
    case eEditorTowerMode::FOLLOW_AT_DISTANCE:     return 5;
    case eEditorTowerMode::STATIC:                  break;
  }
  return 0;
}

//-------------------------------------------------------------------------------------------------

int CEditorTowerModel::Encode(eEditorTowerMode mode, int iZoom)
{
  const int iCanonicalZoom = std::clamp(iZoom, 0, MAX_CANONICAL_ZOOM);
  return TOWER_TYPE_BASE + 16 * iCanonicalZoom + ModeNibble(mode);
}

//-------------------------------------------------------------------------------------------------

bool CEditorTowerModel::IsCanonical(int iSignType)
{
  return IsTower(iSignType)
      && Encode(DecodeMode(iSignType), DecodeZoom(iSignType)) == iSignType;
}

//-------------------------------------------------------------------------------------------------

bool CEditorTowerModel::UsesHorizontalOffset(eEditorTowerMode mode)
{
  return mode != eEditorTowerMode::TRACK_SURFACE_TWO_BACK
      && mode != eEditorTowerMode::OVERHEAD_FOLLOW;
}

//-------------------------------------------------------------------------------------------------

bool CEditorTowerModel::UsesVerticalOffset(eEditorTowerMode mode)
{
  return mode != eEditorTowerMode::TRACK_SURFACE_TWO_BACK;
}

//-------------------------------------------------------------------------------------------------

int CEditorTowerModel::VerticalOffsetScale(eEditorTowerMode mode)
{
  return mode == eEditorTowerMode::OVERHEAD_FOLLOW ? 128 : 32;
}

//-------------------------------------------------------------------------------------------------

bool CEditorTowerModel::IsValidRange(const CChunkAy &chunkAy,
                                     int iFrom, int iTo)
{
  return iFrom >= 0 && iTo >= iFrom
      && iTo < static_cast<int>(chunkAy.size());
}

//-------------------------------------------------------------------------------------------------

int CEditorTowerModel::CountTowers(const CChunkAy &chunkAy)
{
  return static_cast<int>(std::count_if(
      chunkAy.begin(), chunkAy.end(), [](const tGeometryChunk &Chunk) {
        return IsTower(Chunk.iSignType);
      }));
}

//-------------------------------------------------------------------------------------------------

int CEditorTowerModel::AddTowers(CChunkAy &chunkAy, int iFrom, int iTo)
{
  if (!IsValidRange(chunkAy, iFrom, iTo))
    return 0;

  int iTowerCount = CountTowers(chunkAy);
  int iChanged = 0;
  for (int i = iFrom; i <= iTo && iTowerCount < TOWER_LIMIT; ++i) {
    // -1 is the only empty sign column. Existing signs and towers are both
    // owned by their respective dock and are never silently replaced.
    if (chunkAy[i].iSignType != -1)
      continue;
    chunkAy[i].iSignType = Encode(eEditorTowerMode::STATIC, 0);
    ++iTowerCount;
    ++iChanged;
  }
  return iChanged;
}

//-------------------------------------------------------------------------------------------------

int CEditorTowerModel::DeleteTowers(CChunkAy &chunkAy, int iFrom, int iTo)
{
  if (!IsValidRange(chunkAy, iFrom, iTo))
    return 0;

  int iChanged = 0;
  for (int i = iFrom; i <= iTo; ++i) {
    if (!IsTower(chunkAy[i].iSignType))
      continue;
    chunkAy[i].iSignType = -1;
    ++iChanged;
  }
  return iChanged;
}

//-------------------------------------------------------------------------------------------------

int CEditorTowerModel::SetMode(CChunkAy &chunkAy, int iFrom, int iTo,
                               eEditorTowerMode mode)
{
  if (!IsValidRange(chunkAy, iFrom, iTo))
    return 0;

  int iChanged = 0;
  for (int i = iFrom; i <= iTo; ++i) {
    if (!IsTower(chunkAy[i].iSignType))
      continue;
    const int iNewType = Encode(mode, DecodeZoom(chunkAy[i].iSignType));
    if (chunkAy[i].iSignType == iNewType)
      continue;
    chunkAy[i].iSignType = iNewType;
    ++iChanged;
  }
  return iChanged;
}

//-------------------------------------------------------------------------------------------------

int CEditorTowerModel::SetZoom(CChunkAy &chunkAy, int iFrom, int iTo,
                               int iZoom)
{
  if (!IsValidRange(chunkAy, iFrom, iTo))
    return 0;

  int iChanged = 0;
  for (int i = iFrom; i <= iTo; ++i) {
    if (!IsTower(chunkAy[i].iSignType))
      continue;
    const int iNewType = Encode(DecodeMode(chunkAy[i].iSignType), iZoom);
    if (chunkAy[i].iSignType == iNewType)
      continue;
    chunkAy[i].iSignType = iNewType;
    ++iChanged;
  }
  return iChanged;
}

//-------------------------------------------------------------------------------------------------

int CEditorTowerModel::SetHorizontalOffset(CChunkAy &chunkAy,
                                            int iFrom, int iTo, int iOffset)
{
  if (!IsValidRange(chunkAy, iFrom, iTo))
    return 0;

  int iChanged = 0;
  for (int i = iFrom; i <= iTo; ++i) {
    if (!IsTower(chunkAy[i].iSignType)
        || chunkAy[i].iSignHorizOffset == iOffset)
      continue;
    chunkAy[i].iSignHorizOffset = iOffset;
    ++iChanged;
  }
  return iChanged;
}

//-------------------------------------------------------------------------------------------------

int CEditorTowerModel::SetVerticalOffset(CChunkAy &chunkAy,
                                          int iFrom, int iTo, int iOffset)
{
  if (!IsValidRange(chunkAy, iFrom, iTo))
    return 0;

  int iChanged = 0;
  for (int i = iFrom; i <= iTo; ++i) {
    if (!IsTower(chunkAy[i].iSignType)
        || chunkAy[i].iSignVertOffset == iOffset)
      continue;
    chunkAy[i].iSignVertOffset = iOffset;
    ++iChanged;
  }
  return iChanged;
}

//-------------------------------------------------------------------------------------------------

int CEditorTowerModel::SetRawType(CChunkAy &chunkAy, int iFrom, int iTo,
                                  int iSignType)
{
  if (!IsValidRange(chunkAy, iFrom, iTo) || !IsTower(iSignType))
    return 0;

  int iChanged = 0;
  for (int i = iFrom; i <= iTo; ++i) {
    if (!IsTower(chunkAy[i].iSignType)
        || chunkAy[i].iSignType == iSignType)
      continue;
    chunkAy[i].iSignType = iSignType;
    ++iChanged;
  }
  return iChanged;
}
