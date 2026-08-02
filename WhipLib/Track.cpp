#include "Track.h"
#include "Texture.h"
//-------------------------------------------------------------------------------------------------

CTrack::CTrack() = default;

//-------------------------------------------------------------------------------------------------

CTrack::~CTrack() = default;

//-------------------------------------------------------------------------------------------------

bool CTrack::ShouldDrawSurfaceType(int iSurfaceType)
{
  if (iSurfaceType == -1 || iSurfaceType == 0)
    return false;
  uint32 uiSurfaceType = CTrack::GetSignedBitValueFromInt(iSurfaceType);
  if (uiSurfaceType & SURFACE_FLAG_SKIP_RENDER)
    return false;
  return true;
}

//-------------------------------------------------------------------------------------------------

bool CTrack::ShouldShowChunkSection(int i, eShapeSection section)
{
  if ((section == eShapeSection::CENTER)
      && !ShouldDrawSurfaceType(m_chunkAy[i].iCenterSurfaceType))
    return false;
  if (section == eShapeSection::LSHOULDER
      && !ShouldDrawSurfaceType(m_chunkAy[i].iLeftSurfaceType))
    return false;
  if (section == eShapeSection::RSHOULDER
      && !ShouldDrawSurfaceType(m_chunkAy[i].iRightSurfaceType))
    return false;
  if (section == eShapeSection::LWALL
      && !ShouldDrawSurfaceType(m_chunkAy[i].iLeftWallType))
    return false;
  if (section == eShapeSection::RWALL
      && !ShouldDrawSurfaceType(m_chunkAy[i].iRightWallType))
    return false;
  if (section == eShapeSection::ROOF
      && (!ShouldDrawSurfaceType(m_chunkAy[i].iRoofType)
          || m_chunkAy[i].iLeftWallType == -1
          || m_chunkAy[i].iRightWallType == -1
          || (!ShouldDrawSurfaceType(m_chunkAy[i].iLeftWallType) && !ShouldDrawSurfaceType(m_chunkAy[i].iRightWallType))))
    return false;
  if (section == eShapeSection::OWALLFLOOR
      && (m_chunkAy[i].iOuterFloorType == -2
          || !ShouldDrawSurfaceType(m_chunkAy[i].iOuterFloorType)))
    return false;
  if (section == eShapeSection::LLOWALL
      && (!ShouldDrawSurfaceType(m_chunkAy[i].iLLOuterWallType)
          || m_chunkAy[i].iOuterFloorType == -1
          || (m_chunkAy[i].iOuterFloorType == -2
              && !ShouldDrawSurfaceType(m_chunkAy[i].iLeftSurfaceType)
              && !ShouldDrawSurfaceType(m_chunkAy[i].iCenterSurfaceType))))
    return false;
  if (section == eShapeSection::RLOWALL
      && (!ShouldDrawSurfaceType(m_chunkAy[i].iRLOuterWallType)
          || m_chunkAy[i].iOuterFloorType == -1
          || (m_chunkAy[i].iOuterFloorType == -2
              && !ShouldDrawSurfaceType(m_chunkAy[i].iRightSurfaceType)
              && !ShouldDrawSurfaceType(m_chunkAy[i].iCenterSurfaceType))))
    return false;
  if (section == eShapeSection::LUOWALL
      && (!ShouldDrawSurfaceType(m_chunkAy[i].iLUOuterWallType)
          || m_chunkAy[i].iOuterFloorType == -1
          || m_chunkAy[i].iLLOuterWallType == -1))
    return false;
  if (section == eShapeSection::RUOWALL
      && (!ShouldDrawSurfaceType(m_chunkAy[i].iRUOuterWallType)
          || m_chunkAy[i].iOuterFloorType == -1
          || m_chunkAy[i].iRLOuterWallType == -1))
    return false;
  return true;
}

//-------------------------------------------------------------------------------------------------

bool CTrack::HasPitchedStunt()
{
  CStuntMap::iterator it = m_stuntMap.begin();
  for (; it != m_stuntMap.end(); ++it) {
    int iStart = it->first - it->second.iChunkCount;
    int iEnd = it->first + it->second.iChunkCount;

    if (iStart < 0)
      iStart = 0;
    if (iEnd > (int)m_chunkAy.size() - 1)
      iEnd = (int)m_chunkAy.size() - 1;

    for (int i = iStart; i <= iEnd; ++i) {
      if (m_chunkAy[i].dPitch > 5.0 && m_chunkAy[i].dPitch < 355.0)
        return true;
    }
  }

  return false;
}
