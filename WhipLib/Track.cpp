#include "Track.h"
#include <assert.h>
#include "Texture.h"
#include "Palette.h"
#include "glm.hpp"
#include "gtc/matrix_transform.hpp"
#include "gtx/transform.hpp"
#include "gtx/quaternion.hpp"
#include "MathHelpers.h"
#if defined(IS_WINDOWS)
  #include "Windows.h"
#endif
//-------------------------------------------------------------------------------------------------
#if defined(_DEBUG) && defined(IS_WINDOWS)
#define new new(_CLIENT_BLOCK, __FILE__, __LINE__)
#endif
//-------------------------------------------------------------------------------------------------

CTrack::CTrack()
  : m_iAILineHeight(100)
  , m_pPal(NULL)
  , m_pTex(NULL)
  , m_pBld(NULL)
{
  ClearData();
}

//-------------------------------------------------------------------------------------------------

CTrack::~CTrack()
{
  if (m_pTex) {
    delete m_pTex;
    m_pTex = NULL;
  }
  if (m_pBld) {
    delete m_pBld;
    m_pBld = NULL;
  }
  if (m_pPal) {
    delete m_pPal;
    m_pPal = NULL;
  }
}

//-------------------------------------------------------------------------------------------------

void CTrack::ClearData()
{
  CTrackModel::ClearData();
  m_chunkMathAy.clear();
  m_sLastLoadedTex = "";
  m_sLastLoadedBld = "";
  m_sLastLoadedPal = "";
}

//-------------------------------------------------------------------------------------------------

bool CTrack::ProcessTrackData(const uint8 *pData, size_t length)
{
  const bool bSuccess = CTrackModel::ProcessTrackData(pData, length);
  GenerateTrackMath();
  return bSuccess;
}

//-------------------------------------------------------------------------------------------------

bool CTrack::LoadTextures()
{
  bool bSuccess = true;

  std::string sPal = m_sTrackFileFolder + "PALETTE.PAL";
  std::string sTex = m_sTrackFileFolder + m_sTextureFile;
  std::string sBld = m_sTrackFileFolder + m_sBuildingFile;

  if (m_sLastLoadedPal.compare(sPal) != 0) {
    if (m_pPal) {
      delete m_pPal;
      m_pPal = NULL;
    }
    m_pPal = new CPalette;
    bSuccess &= m_pPal->LoadPalette(sPal);
    if (bSuccess)
      m_sLastLoadedPal = sPal;
  }

  if (m_sLastLoadedTex.compare(sTex) != 0) {
    if (m_pTex) {
      delete m_pTex;
      m_pTex = NULL;
    }
    m_pTex = new CTexture;
    bSuccess &= m_pTex->LoadTexture(sTex, m_pPal);
    if (bSuccess)
      m_sLastLoadedTex = sTex;
  }

  if (m_sLastLoadedBld.compare(sBld) != 0) {
    if (m_pBld) {
      delete m_pBld;
      m_pBld = NULL;
    }
    m_pBld = new CTexture;
    bSuccess &= m_pBld->LoadTexture(sBld, m_pPal);
    if (bSuccess)
      m_sLastLoadedBld = sBld;
  }

  return bSuccess;
}

//-------------------------------------------------------------------------------------------------

bool CTrack::ShouldDrawSurfaceType(int iSurfaceType)
{
  if (iSurfaceType == -1 || iSurfaceType == 0)
    return false;
  uint32 uiSurfaceType = CTrack::GetSignedBitValueFromInt(iSurfaceType);
  if (uiSurfaceType & SURFACE_FLAG_SKIP_RENDER)
    return false;
  //if (!(uiSurfaceType & SURFACE_FLAG_TRANSPARENT)
  //    && !(uiSurfaceType & SURFACE_FLAG_APPLY_TEXTURE))
  //  return false;
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


//-------------------------------------------------------------------------------------------------

void CTrack::GenerateTrackMath()
{
  m_chunkMathAy.assign(m_chunkAy.size(), tChunkMath{});
  if (m_chunkAy.empty()) {
    return;
  }

  ResetStunts();

  m_chunkMathAy[0].center = glm::vec3(0, 0, 0);
  for (uint32 i = 0; i < m_chunkAy.size(); ++i) {
    glm::vec3 nextCenter;
    GetCenter(i, m_chunkMathAy[i].center,
              nextCenter,
              m_chunkMathAy[i].pitchAxis,
              m_chunkMathAy[i].nextChunkPitched,
              m_chunkMathAy[i].normal,
              m_chunkMathAy[i].yawMat,
              m_chunkMathAy[i].pitchMat,
              m_chunkMathAy[i].rollMat);
    if (i + 1 < m_chunkAy.size())
      m_chunkMathAy[i + 1].center = nextCenter;
  }
  for (uint32 i = 0; i < m_chunkAy.size(); ++i) {
    int iPrevIndex = (int)m_chunkAy.size() - 1;
    if (i > 0)
      iPrevIndex = i - 1;

    glm::mat4 rollMatNoRoll = glm::mat4(1);

    //left lane
    GetLane(i,
            m_chunkMathAy[i].center,
            m_chunkMathAy[i].pitchAxis,
            m_chunkMathAy[i].rollMat,
            m_chunkMathAy[i].lLane, true);
    //right lane
    GetLane(i,
            m_chunkMathAy[i].center,
            m_chunkMathAy[i].pitchAxis,
            m_chunkMathAy[i].rollMat,
            m_chunkMathAy[i].rLane, false);
    //left shoulder
    GetShoulder(i,
                m_chunkMathAy[i].lLane,
                m_chunkMathAy[i].pitchAxis,
                m_chunkMathAy[i].rollMat,
                m_chunkMathAy[i].nextChunkPitched,
                m_chunkMathAy[i].lShoulder, true);
    //right shoulder
    GetShoulder(i,
                m_chunkMathAy[i].rLane,
                m_chunkMathAy[i].pitchAxis,
                m_chunkMathAy[i].rollMat,
                m_chunkMathAy[i].nextChunkPitched,
                m_chunkMathAy[i].rShoulder, false);
    //left wall
    m_chunkMathAy[i].bLWallAttachToLane = CTrack::GetSignedBitValueFromInt(m_chunkAy[i].iLeftWallType) & SURFACE_FLAG_WALL_31;
    if (m_chunkAy[i].iLeftWallType == -1)
      m_chunkMathAy[i].bLWallAttachToLane = m_chunkMathAy[iPrevIndex].bLWallAttachToLane;
    m_chunkMathAy[i].lWallBottomAttach = m_chunkMathAy[i].bLWallAttachToLane ? m_chunkMathAy[i].lLane : m_chunkMathAy[i].lShoulder;
    GetWall(i,
            m_chunkMathAy[i].lWallBottomAttach,
            m_chunkMathAy[i].pitchAxis,
            m_chunkMathAy[i].rollMat,
            m_chunkMathAy[i].nextChunkPitched,
            m_chunkMathAy[i].lWall, eShapeSection::LWALL);
    //right wall
    m_chunkMathAy[i].bRWallAttachToLane = CTrack::GetSignedBitValueFromInt(m_chunkAy[i].iRightWallType) & SURFACE_FLAG_WALL_31;
    if (m_chunkAy[i].iRightWallType == -1)
      m_chunkMathAy[i].bRWallAttachToLane = m_chunkMathAy[iPrevIndex].bRWallAttachToLane;
    m_chunkMathAy[i].rWallBottomAttach = m_chunkMathAy[i].bRWallAttachToLane ? m_chunkMathAy[i].rLane : m_chunkMathAy[i].rShoulder;
    GetWall(i,
            m_chunkMathAy[i].rWallBottomAttach,
            m_chunkMathAy[i].pitchAxis,
            m_chunkMathAy[i].rollMat,
            m_chunkMathAy[i].nextChunkPitched,
            m_chunkMathAy[i].rWall, eShapeSection::RWALL);
    //outer floor
    glm::vec3 lLaneNoRoll;
    GetLane(i,
            m_chunkMathAy[i].center,
            m_chunkMathAy[i].pitchAxis,
            rollMatNoRoll, lLaneNoRoll, true);
    glm::vec3 rLaneNoRoll;
    GetLane(i,
            m_chunkMathAy[i].center,
            m_chunkMathAy[i].pitchAxis,
            rollMatNoRoll, rLaneNoRoll, false);
    GetOWallFloor(i, lLaneNoRoll, rLaneNoRoll,
                  m_chunkMathAy[i].pitchAxis,
                  m_chunkMathAy[i].nextChunkPitched,
                  m_chunkMathAy[i].lFloor,
                  m_chunkMathAy[i].rFloor);
    //outer wall roll mat
    glm::mat4 oWallRollMat = m_chunkAy[i].iOuterFloorType < 0 ? m_chunkMathAy[i].rollMat : rollMatNoRoll;
    //llowall
    m_chunkMathAy[i].lloWallBottomAttach = m_chunkMathAy[i].lFloor;
    if (m_chunkAy[i].iOuterFloorType < 0) {
      m_chunkMathAy[i].bLloWallAttachToShoulder = true;
      m_chunkMathAy[i].lloWallBottomAttach = m_chunkMathAy[i].lShoulder;
    }
    GetWall(i,
            m_chunkMathAy[i].lloWallBottomAttach,
            m_chunkMathAy[i].pitchAxis, oWallRollMat,
            m_chunkMathAy[i].nextChunkPitched,
            m_chunkMathAy[i].lloWall, eShapeSection::LLOWALL);
    //rlowall
    m_chunkMathAy[i].rloWallBottomAttach = m_chunkMathAy[i].rFloor;
    if (m_chunkAy[i].iOuterFloorType < 0) {
      m_chunkMathAy[i].bRloWallAttachToShoulder = true;
      m_chunkMathAy[i].rloWallBottomAttach = m_chunkMathAy[i].rShoulder;
    }
    GetWall(i,
            m_chunkMathAy[i].rloWallBottomAttach,
            m_chunkMathAy[i].pitchAxis, oWallRollMat,
            m_chunkMathAy[i].nextChunkPitched,
            m_chunkMathAy[i].rloWall, eShapeSection::RLOWALL);
    //luowall
    GetWall(i,
            m_chunkMathAy[i].lloWall,
            m_chunkMathAy[i].pitchAxis, oWallRollMat,
            m_chunkMathAy[i].nextChunkPitched,
            m_chunkMathAy[i].luoWall, eShapeSection::LUOWALL);
    //ruowall
    GetWall(i,
            m_chunkMathAy[i].rloWall,
            m_chunkMathAy[i].pitchAxis, oWallRollMat,
            m_chunkMathAy[i].nextChunkPitched,
            m_chunkMathAy[i].ruoWall, eShapeSection::RUOWALL);
    //ailines
    GetAILine(i,
              m_chunkMathAy[i].center,
              m_chunkMathAy[i].pitchAxis,
              m_chunkMathAy[i].rollMat,
              m_chunkMathAy[i].nextChunkPitched,
              m_chunkMathAy[i].aiLine1,
              eShapeSection::AILINE1, m_iAILineHeight);
    GetAILine(i,
              m_chunkMathAy[i].center,
              m_chunkMathAy[i].pitchAxis,
              m_chunkMathAy[i].rollMat,
              m_chunkMathAy[i].nextChunkPitched,
              m_chunkMathAy[i].aiLine2,
              eShapeSection::AILINE2, m_iAILineHeight);
    GetAILine(i,
              m_chunkMathAy[i].center,
              m_chunkMathAy[i].pitchAxis,
              m_chunkMathAy[i].rollMat,
              m_chunkMathAy[i].nextChunkPitched,
              m_chunkMathAy[i].aiLine3, eShapeSection::AILINE3, m_iAILineHeight);
    GetAILine(i,
              m_chunkMathAy[i].center,
              m_chunkMathAy[i].pitchAxis,
              m_chunkMathAy[i].rollMat,
              m_chunkMathAy[i].nextChunkPitched,
              m_chunkMathAy[i].aiLine4,
              eShapeSection::AILINE4, m_iAILineHeight);
    //car positions are ai lines with 0 height
    GetAILine(i,
              m_chunkMathAy[i].center,
              m_chunkMathAy[i].pitchAxis,
              m_chunkMathAy[i].rollMat,
              m_chunkMathAy[i].nextChunkPitched,
              m_chunkMathAy[i].carLine1,
              eShapeSection::AILINE1, 0);
    GetAILine(i,
              m_chunkMathAy[i].center,
              m_chunkMathAy[i].pitchAxis,
              m_chunkMathAy[i].rollMat,
              m_chunkMathAy[i].nextChunkPitched,
              m_chunkMathAy[i].carLine2,
              eShapeSection::AILINE2, 0);
    GetAILine(i,
              m_chunkMathAy[i].center,
              m_chunkMathAy[i].pitchAxis,
              m_chunkMathAy[i].rollMat,
              m_chunkMathAy[i].nextChunkPitched,
              m_chunkMathAy[i].carLine3,
              eShapeSection::AILINE3, 0);
    GetAILine(i,
              m_chunkMathAy[i].center,
              m_chunkMathAy[i].pitchAxis,
              m_chunkMathAy[i].rollMat,
              m_chunkMathAy[i].nextChunkPitched,
              m_chunkMathAy[i].carLine4,
              eShapeSection::AILINE4, 0);

    m_chunkMathAy[i].centerStunt = m_chunkMathAy[i].center;
  }
}

//-------------------------------------------------------------------------------------------------

void CTrack::ResetStunts()
{
  CStuntMap::iterator it = m_stuntMap.begin();
  for (; it != m_stuntMap.end(); ++it) {
    it->second.iTickCurrIdx = it->second.iTickStartIdx;
  }
}

//-------------------------------------------------------------------------------------------------

void CTrack::UpdateStunts()
{
  CStuntMap::iterator it = m_stuntMap.begin();
  for (; it != m_stuntMap.end(); ++it) {
    int iStart = it->first - it->second.iChunkCount + 1;
    int iEnd = it->first + it->second.iChunkCount;

    if (iStart < 0)
      iStart = 0;
    if (iEnd > (int)m_chunkAy.size() - 1)
      iEnd = (int)m_chunkAy.size() - 1;

    int iHeight = 0;
    int iLengthPercent = STUNT_LENGTH_100_PERCENT; //100%
    int iDifference = it->second.iRampSideLength - STUNT_LENGTH_100_PERCENT;
    if (it->second.iTickCurrIdx < it->second.iNumTicks) {
      iHeight = it->second.iHeight * it->second.iTickCurrIdx;
      float fTickPercent = (float)it->second.iTickCurrIdx / (float)it->second.iNumTicks;
      iLengthPercent = (int)((float)STUNT_LENGTH_100_PERCENT + (float)iDifference * fTickPercent);
    } else if (it->second.iTickCurrIdx < it->second.iNumTicks + it->second.iTimeBulging) {
      iHeight = it->second.iHeight * it->second.iNumTicks;
      iLengthPercent = it->second.iRampSideLength;
    } else if (it->second.iTickCurrIdx < it->second.iNumTicks + it->second.iTimeBulging + it->second.iNumTicks) {
      int iTickUseIdx = (it->second.iNumTicks - (it->second.iTickCurrIdx - it->second.iNumTicks - it->second.iTimeBulging));
      iHeight = it->second.iHeight * iTickUseIdx;
      float fTickPercent = (float)iTickUseIdx / (float)it->second.iNumTicks;
      iLengthPercent = (int)((float)STUNT_LENGTH_100_PERCENT + (float)iDifference * fTickPercent);
    } else if (it->second.iTickCurrIdx < it->second.iNumTicks + it->second.iTimeBulging + it->second.iNumTicks + it->second.iTimeFlat) {
      iHeight = 0;
      iLengthPercent = STUNT_LENGTH_100_PERCENT;
    } else {
      it->second.iTickCurrIdx = 0;
    }
    it->second.iTickCurrIdx++;
    float fTheta = atan((float)iHeight / (float)m_chunkAy[iStart].iLength);
    
    //ramp before stunt
    for (int i = iStart; i < it->first + 1; ++i) {
      int iPrevIndex = (int)m_chunkAy.size() - 1;
      if (i > 0)
        iPrevIndex = i - 1;
      glm::vec3 prevCenter = (i == iStart) ? m_chunkMathAy[iPrevIndex].center : m_chunkMathAy[iPrevIndex].centerStunt;
      glm::vec3 nextChunkBase = glm::vec3(0, 0, 1);
      glm::mat4 yawMat = m_chunkMathAy[i].yawMat;
      glm::vec3 nextChunkYawed = glm::vec3(yawMat * glm::vec4(nextChunkBase, 1.0f));
      glm::vec3 pitchAxis = glm::normalize(glm::cross(nextChunkYawed, glm::vec3(0.0f, 1.0f, 0.0f)));
      glm::mat4 pitchMat = glm::rotate(fTheta, pitchAxis);
      glm::vec3 nextChunkPitched = glm::vec3(pitchMat * glm::vec4(nextChunkYawed, 1.0f));
      glm::mat4 translateMat = glm::mat4(1);
      if (i > 0)
        translateMat = glm::translate(prevCenter);
      float fLen = (float)m_chunkAy[i].iLength * ((float)iLengthPercent / (float)STUNT_LENGTH_100_PERCENT);
      glm::mat4 scaleMat = glm::scale(glm::vec3(fLen, fLen, fLen));
      m_chunkMathAy[i].centerStunt = glm::vec3(translateMat * scaleMat * glm::vec4(nextChunkPitched, 1.0f));
      glm::vec3 lLaneStunt;
      glm::vec3 rLaneStunt;
      GetLane(i, m_chunkMathAy[i].centerStunt, pitchAxis, m_chunkMathAy[i].rollMat, lLaneStunt, true);
      GetLane(i, m_chunkMathAy[i].centerStunt, pitchAxis, m_chunkMathAy[i].rollMat, rLaneStunt, false);
      if (it->second.iFlags & STUNT_FLAG_LLANE)
        m_chunkMathAy[i].lLane = lLaneStunt;
      if (it->second.iFlags & STUNT_FLAG_RLANE)
        m_chunkMathAy[i].rLane = rLaneStunt;
      if (it->second.iFlags & STUNT_FLAG_LSHOULDER)
        GetShoulder(i, lLaneStunt,
                    m_chunkMathAy[i].pitchAxis,
                    m_chunkMathAy[i].rollMat,
                    m_chunkMathAy[i].nextChunkPitched,
                    m_chunkMathAy[i].lShoulder, true);
      if (it->second.iFlags & STUNT_FLAG_RSHOULDER)
        GetShoulder(i, rLaneStunt,
                    m_chunkMathAy[i].pitchAxis,
                    m_chunkMathAy[i].rollMat,
                    m_chunkMathAy[i].nextChunkPitched,
                    m_chunkMathAy[i].rShoulder, false);
      m_chunkMathAy[i].lWallBottomAttach = m_chunkMathAy[i].bLWallAttachToLane ? m_chunkMathAy[i].lLane : m_chunkMathAy[i].lShoulder;
      if (it->second.iFlags & STUNT_FLAG_LWALL)
        GetWall(i,
                m_chunkMathAy[i].lWallBottomAttach,
                m_chunkMathAy[i].pitchAxis,
                m_chunkMathAy[i].rollMat,
                m_chunkMathAy[i].nextChunkPitched,
                m_chunkMathAy[i].lWall, eShapeSection::LWALL);
      m_chunkMathAy[i].rWallBottomAttach = m_chunkMathAy[i].bRWallAttachToLane ? m_chunkMathAy[i].rLane : m_chunkMathAy[i].rShoulder;
      if (it->second.iFlags & STUNT_FLAG_RWALL)
        GetWall(i,
                m_chunkMathAy[i].rWallBottomAttach,
                m_chunkMathAy[i].pitchAxis,
                m_chunkMathAy[i].rollMat,
                m_chunkMathAy[i].nextChunkPitched,
                m_chunkMathAy[i].rWall, eShapeSection::RWALL);
      //ailines
      GetAILine(i,
                m_chunkMathAy[i].centerStunt,
                m_chunkMathAy[i].pitchAxis,
                m_chunkMathAy[i].rollMat,
                m_chunkMathAy[i].nextChunkPitched,
                m_chunkMathAy[i].aiLine1,
                eShapeSection::AILINE1, m_iAILineHeight);
      GetAILine(i,
                m_chunkMathAy[i].centerStunt,
                m_chunkMathAy[i].pitchAxis,
                m_chunkMathAy[i].rollMat,
                m_chunkMathAy[i].nextChunkPitched,
                m_chunkMathAy[i].aiLine2,
                eShapeSection::AILINE2, m_iAILineHeight);
      GetAILine(i,
                m_chunkMathAy[i].centerStunt,
                m_chunkMathAy[i].pitchAxis,
                m_chunkMathAy[i].rollMat,
                m_chunkMathAy[i].nextChunkPitched,
                m_chunkMathAy[i].aiLine3, eShapeSection::AILINE3, m_iAILineHeight);
      GetAILine(i,
                m_chunkMathAy[i].centerStunt,
                m_chunkMathAy[i].pitchAxis,
                m_chunkMathAy[i].rollMat,
                m_chunkMathAy[i].nextChunkPitched,
                m_chunkMathAy[i].aiLine4,
                eShapeSection::AILINE4, m_iAILineHeight);
      //car positions are ai lines with 0 height
      GetAILine(i,
                m_chunkMathAy[i].centerStunt,
                m_chunkMathAy[i].pitchAxis,
                m_chunkMathAy[i].rollMat,
                m_chunkMathAy[i].nextChunkPitched,
                m_chunkMathAy[i].carLine1,
                eShapeSection::AILINE1, 0);
      GetAILine(i,
                m_chunkMathAy[i].centerStunt,
                m_chunkMathAy[i].pitchAxis,
                m_chunkMathAy[i].rollMat,
                m_chunkMathAy[i].nextChunkPitched,
                m_chunkMathAy[i].carLine2,
                eShapeSection::AILINE2, 0);
      GetAILine(i,
                m_chunkMathAy[i].centerStunt,
                m_chunkMathAy[i].pitchAxis,
                m_chunkMathAy[i].rollMat,
                m_chunkMathAy[i].nextChunkPitched,
                m_chunkMathAy[i].carLine3,
                eShapeSection::AILINE3, 0);
      GetAILine(i,
                m_chunkMathAy[i].centerStunt,
                m_chunkMathAy[i].pitchAxis,
                m_chunkMathAy[i].rollMat,
                m_chunkMathAy[i].nextChunkPitched,
                m_chunkMathAy[i].carLine4,
                eShapeSection::AILINE4, 0);
    }
    //ramp after stunt
    for (int i = iEnd; i > it->first; --i) {
      int iPrevIndex = 0;
      if (i < (int)m_chunkAy.size() - 1)
        iPrevIndex = i + 1;
      glm::vec3 prevCenter = (i == iEnd) ? m_chunkMathAy[iPrevIndex].center : m_chunkMathAy[iPrevIndex].centerStunt;
      glm::vec3 nextChunkBase = glm::vec3(0, 0, 1);
      glm::mat4 yawMat = glm::rotate(glm::radians((float)m_chunkAy[i].dYaw + 180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
      glm::vec3 nextChunkYawed = glm::vec3(yawMat * glm::vec4(nextChunkBase, 1.0f));
      glm::vec3 pitchAxis = glm::normalize(glm::cross(nextChunkYawed, glm::vec3(0.0f, 1.0f, 0.0f)));
      glm::mat4 pitchMat = glm::rotate(fTheta, pitchAxis);
      glm::vec3 nextChunkPitched = glm::vec3(pitchMat * glm::vec4(nextChunkYawed, 1.0f));
      glm::mat4 translateMat = glm::mat4(1);
      if (i > 0)
        translateMat = glm::translate(prevCenter);
      float fLen = (float)m_chunkAy[i].iLength * ((float)it->second.iRampSideLength / 1024.0f);
      glm::mat4 scaleMat = glm::scale(glm::vec3(fLen, fLen, fLen));
      m_chunkMathAy[i].centerStunt = glm::vec3(translateMat * scaleMat * glm::vec4(nextChunkPitched, 1.0f));
      glm::vec3 lLaneStunt;
      glm::vec3 rLaneStunt;
      GetLane(i, m_chunkMathAy[i].centerStunt, pitchAxis, m_chunkMathAy[i].rollMat, lLaneStunt, false);
      GetLane(i, m_chunkMathAy[i].centerStunt, pitchAxis, m_chunkMathAy[i].rollMat, rLaneStunt, true);
      if (it->second.iFlags & STUNT_FLAG_LLANE)
        m_chunkMathAy[i].lLane = lLaneStunt;
      if (it->second.iFlags & STUNT_FLAG_RLANE)
        m_chunkMathAy[i].rLane = rLaneStunt;
      if (it->second.iFlags & STUNT_FLAG_LSHOULDER)
        GetShoulder(i, lLaneStunt,
                    m_chunkMathAy[i].pitchAxis,
                    m_chunkMathAy[i].rollMat,
                    m_chunkMathAy[i].nextChunkPitched,
                    m_chunkMathAy[i].lShoulder, true);
      if (it->second.iFlags & STUNT_FLAG_RSHOULDER)
        GetShoulder(i,
                    rLaneStunt,
                    m_chunkMathAy[i].pitchAxis,
                    m_chunkMathAy[i].rollMat,
                    m_chunkMathAy[i].nextChunkPitched,
                    m_chunkMathAy[i].rShoulder, false);
      m_chunkMathAy[i].lWallBottomAttach = m_chunkMathAy[i].bLWallAttachToLane ? m_chunkMathAy[i].lLane : m_chunkMathAy[i].lShoulder;
      if (it->second.iFlags & STUNT_FLAG_LWALL)
        GetWall(i,
                m_chunkMathAy[i].lWallBottomAttach,
                m_chunkMathAy[i].pitchAxis,
                m_chunkMathAy[i].rollMat,
                m_chunkMathAy[i].nextChunkPitched,
                m_chunkMathAy[i].lWall, eShapeSection::LWALL);
      m_chunkMathAy[i].rWallBottomAttach = m_chunkMathAy[i].bRWallAttachToLane ? m_chunkMathAy[i].rLane : m_chunkMathAy[i].rShoulder;
      if (it->second.iFlags & STUNT_FLAG_RWALL)
        GetWall(i,
                m_chunkMathAy[i].rWallBottomAttach,
                m_chunkMathAy[i].pitchAxis,
                m_chunkMathAy[i].rollMat,
                m_chunkMathAy[i].nextChunkPitched,
                m_chunkMathAy[i].rWall, eShapeSection::RWALL);
      //ailines
      GetAILine(i,
                m_chunkMathAy[i].centerStunt,
                m_chunkMathAy[i].pitchAxis,
                m_chunkMathAy[i].rollMat,
                m_chunkMathAy[i].nextChunkPitched,
                m_chunkMathAy[i].aiLine1,
                eShapeSection::AILINE1, m_iAILineHeight);
      GetAILine(i,
                m_chunkMathAy[i].centerStunt,
                m_chunkMathAy[i].pitchAxis,
                m_chunkMathAy[i].rollMat,
                m_chunkMathAy[i].nextChunkPitched,
                m_chunkMathAy[i].aiLine2,
                eShapeSection::AILINE2, m_iAILineHeight);
      GetAILine(i,
                m_chunkMathAy[i].centerStunt,
                m_chunkMathAy[i].pitchAxis,
                m_chunkMathAy[i].rollMat,
                m_chunkMathAy[i].nextChunkPitched,
                m_chunkMathAy[i].aiLine3, eShapeSection::AILINE3, m_iAILineHeight);
      GetAILine(i,
                m_chunkMathAy[i].centerStunt,
                m_chunkMathAy[i].pitchAxis,
                m_chunkMathAy[i].rollMat,
                m_chunkMathAy[i].nextChunkPitched,
                m_chunkMathAy[i].aiLine4,
                eShapeSection::AILINE4, m_iAILineHeight);
      //car positions are ai lines with 0 height
      GetAILine(i,
                m_chunkMathAy[i].centerStunt,
                m_chunkMathAy[i].pitchAxis,
                m_chunkMathAy[i].rollMat,
                m_chunkMathAy[i].nextChunkPitched,
                m_chunkMathAy[i].carLine1,
                eShapeSection::AILINE1, 0);
      GetAILine(i,
                m_chunkMathAy[i].centerStunt,
                m_chunkMathAy[i].pitchAxis,
                m_chunkMathAy[i].rollMat,
                m_chunkMathAy[i].nextChunkPitched,
                m_chunkMathAy[i].carLine2,
                eShapeSection::AILINE2, 0);
      GetAILine(i,
                m_chunkMathAy[i].centerStunt,
                m_chunkMathAy[i].pitchAxis,
                m_chunkMathAy[i].rollMat,
                m_chunkMathAy[i].nextChunkPitched,
                m_chunkMathAy[i].carLine3,
                eShapeSection::AILINE3, 0);
      GetAILine(i,
                m_chunkMathAy[i].centerStunt,
                m_chunkMathAy[i].pitchAxis,
                m_chunkMathAy[i].rollMat,
                m_chunkMathAy[i].nextChunkPitched,
                m_chunkMathAy[i].carLine4,
                eShapeSection::AILINE4, 0);
    }
  }
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

//-------------------------------------------------------------------------------------------------

bool CTrack::UseCenterStunt(int i)
{
  bool bUseCenterStunt = false;
  CStuntMap::iterator it = m_stuntMap.begin();
  for (; it != m_stuntMap.end(); ++it) {
    int iStart = it->first - it->second.iChunkCount;
    int iEnd = it->first + it->second.iChunkCount;

    if (iStart < 0)
      iStart = 0;
    if (iEnd > (int)m_chunkAy.size() - 1)
      iEnd = (int)m_chunkAy.size() - 1;

    if (i >= iStart && i <= iEnd) {
      //chunk is in stunt
      bUseCenterStunt |= (it->second.iFlags & STUNT_FLAG_LLANE || it->second.iFlags & STUNT_FLAG_RLANE);
    }
  }
  return bUseCenterStunt;
}

//-------------------------------------------------------------------------------------------------

bool MatrixContainsNan(const glm::mat4 &mat)
{
  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      if (glm::isnan(mat[i][j]))
        return true;
    }
  }
  return false;
}

//-------------------------------------------------------------------------------------------------

void CTrack::CollideWithChunk(const glm::vec3 &position, int &iClosestChunk, int &iPrevChunk,
                              glm::vec3 &p0, glm::vec3 &p1, glm::vec3 &p2, glm::vec3 &p3, const glm::vec3 &peg1, const glm::vec3 &peg2)
{
  glm::vec3 rayOrig = peg1;
  glm::vec3 rayVec = peg2 - peg1;

  iClosestChunk = -1;
  iPrevChunk = -1;
  for (int i = 0; i < (int)m_chunkAy.size(); ++i) {
    int iPrevIndex = (int)m_chunkAy.size() - 1;
    if (i > 0)
      iPrevIndex = i - 1;

    //center
    if (MathHelpers::RayCollisionTriangle(rayOrig, rayVec, m_chunkMathAy[iPrevIndex].lLane, m_chunkMathAy[i].rLane, m_chunkMathAy[i].lLane)) {
      iClosestChunk = i;
      iPrevChunk = iPrevIndex;
      p0 = m_chunkMathAy[iPrevIndex].lLane;
      p1 = m_chunkMathAy[i].rLane;
      p2 = m_chunkMathAy[i].lLane;
      p3 = m_chunkMathAy[iPrevIndex].rLane;
    }
    if (MathHelpers::RayCollisionTriangle(rayOrig, rayVec, m_chunkMathAy[iPrevIndex].rLane, m_chunkMathAy[iPrevIndex].lLane, m_chunkMathAy[i].rLane)) {
      iClosestChunk = i;
      iPrevChunk = iPrevIndex;
      p0 = m_chunkMathAy[iPrevIndex].lLane;
      p1 = m_chunkMathAy[iPrevIndex].rLane;
      p2 = m_chunkMathAy[i].rLane;
      p3 = m_chunkMathAy[i].lLane;
    }
    
    //lshoulder
    if (MathHelpers::RayCollisionTriangle(rayOrig, rayVec, m_chunkMathAy[iPrevIndex].lShoulder, m_chunkMathAy[i].lLane, m_chunkMathAy[i].lShoulder)) {
      iClosestChunk = i;
      iPrevChunk = iPrevIndex;
      p0 = m_chunkMathAy[iPrevIndex].lShoulder;
      p1 = m_chunkMathAy[i].lLane;
      p2 = m_chunkMathAy[i].lShoulder;
      p3 = m_chunkMathAy[iPrevIndex].lLane;
    }
    if (MathHelpers::RayCollisionTriangle(rayOrig, rayVec, m_chunkMathAy[iPrevIndex].lShoulder, m_chunkMathAy[iPrevIndex].lLane, m_chunkMathAy[i].lLane)) {
      iClosestChunk = i;
      iPrevChunk = iPrevIndex;
      p0 = m_chunkMathAy[iPrevIndex].lShoulder;
      p1 = m_chunkMathAy[iPrevIndex].lLane;
      p2 = m_chunkMathAy[i].lLane;
      p3 = m_chunkMathAy[i].lShoulder;
    }

    //rShoulder
    if (MathHelpers::RayCollisionTriangle(rayOrig, rayVec, m_chunkMathAy[iPrevIndex].rLane, m_chunkMathAy[i].rShoulder, m_chunkMathAy[i].rLane)) {
      iClosestChunk = i;
      iPrevChunk = iPrevIndex;
      p0 = m_chunkMathAy[iPrevIndex].rLane;
      p1 = m_chunkMathAy[i].rShoulder;
      p2 = m_chunkMathAy[i].rLane;
      p3 = m_chunkMathAy[iPrevIndex].rShoulder;
    }
    if (MathHelpers::RayCollisionTriangle(rayOrig, rayVec, m_chunkMathAy[iPrevIndex].rLane, m_chunkMathAy[iPrevIndex].rShoulder, m_chunkMathAy[i].rShoulder)) {
      iClosestChunk = i;
      iPrevChunk = iPrevIndex;
      p0 = m_chunkMathAy[iPrevIndex].rLane;
      p1 = m_chunkMathAy[iPrevIndex].rShoulder;
      p2 = m_chunkMathAy[i].rShoulder;
      p3 = m_chunkMathAy[i].rLane;
    }
  }
}

//-------------------------------------------------------------------------------------------------

void CTrack::ProjectToTrack(glm::vec3 &position, glm::mat4 &rotationMat, const glm::vec3 &up, glm::vec3 &p0, glm::vec3 &p1, glm::vec3 &p2, const glm::vec3 &peg1, const glm::vec3 &peg2)
{
  int iClosestChunk = 0;
  int iPrevChunk = 0;
  glm::vec3 p3;

  //project entity pos to plane
  CollideWithChunk(position, iClosestChunk, iPrevChunk, p0, p1, p2, p3, peg1, peg2);
  if (iClosestChunk < 0)
    return;

  position = MathHelpers::ProjectPointOntoPlane(position, p0, p1, p2);

  //get plane normal
  glm::vec3 sub1 = p1 - p0;
  glm::vec3 sub2 = p2 - p0;
  glm::vec3 normal1 = glm::normalize(glm::cross(sub1, sub2));
  //get normal of second plane
  glm::vec3 sub3 = p1 - p3;
  glm::vec3 sub4 = p2 - p3;
  glm::vec3 normal2 = glm::normalize(glm::cross(sub3, sub4));
  //normal of chunk is blended normal of two planes
  glm::vec3 normal = glm::mix(normal1, normal2, 0.5);

  //find rotation axis
  glm::vec3 rotationAxis = glm::normalize(glm::cross(normal, up));
  if (glm::any(glm::isnan(rotationAxis))) {
    //track normal and entity up vector are the same
    return;
  }
  float fCosTheta = glm::dot(normal, glm::normalize(up));
  if (fCosTheta > 1.0f) {
    //floating point error results in un-normalized track normal?
    //close enough to 1.0f we can assume track normal and entity up vector are close enough to the same
    return;
  }
  //find angle amount to rotate
  float fAngleRads = glm::acos(fCosTheta);
  //rotate around axis
  rotationMat = rotationMat * glm::rotate(fAngleRads, rotationAxis);
  return;
}

//-------------------------------------------------------------------------------------------------

void CTrack::GetCenter(int i, glm::vec3 prevCenter,
                           glm::vec3 &center, glm::vec3 &pitchAxis, glm::vec3 &nextChunkPitched, glm::vec3 &normal,
                           glm::mat4 &yawMat, glm::mat4 &pitchMat, glm::mat4 &rollMat)
{
  glm::vec3 nextChunkBase = glm::vec3(0, 0, 1);

  yawMat = glm::rotate(glm::radians((float)m_chunkAy[i].dYaw), glm::vec3(0.0f, 1.0f, 0.0f));
  glm::vec3 nextChunkYawed = glm::vec3(yawMat * glm::vec4(nextChunkBase, 1.0f));
  pitchAxis = glm::normalize(glm::cross(nextChunkYawed, glm::vec3(0.0f, 1.0f, 0.0f)));

  pitchMat = glm::rotate(glm::radians((float)m_chunkAy[i].dPitch), pitchAxis);
  nextChunkPitched = glm::vec3(pitchMat * glm::vec4(nextChunkYawed, 1.0f));

  glm::mat4 translateMat = glm::mat4(1);
  if (i > 0)
    translateMat = glm::translate(prevCenter);
  //center
  float fLen = (float)m_chunkAy[i].iLength;
  glm::mat4 scaleMat = glm::scale(glm::vec3(fLen, fLen, fLen));
  center = glm::vec3(translateMat * scaleMat * glm::vec4(nextChunkPitched, 1.0f));
  rollMat = glm::rotate(glm::radians((float)m_chunkAy[i].dRoll * -1.0f), glm::normalize(nextChunkPitched));
  glm::vec3 pitchAxisRolled = rollMat * glm::vec4(pitchAxis, 1.0f);
  normal = glm::normalize(glm::cross(pitchAxisRolled, nextChunkPitched));
}

//-------------------------------------------------------------------------------------------------

void CTrack::GetLane(int i, glm::vec3 center, glm::vec3 pitchAxis, glm::mat4 rollMat,
                          glm::vec3 &lane, bool bLeft)
{
  glm::mat4 translateMat = glm::translate(center); //translate to centerline
  float fLen;
  if (bLeft)
    fLen = (float)(m_chunkAy[i].iLeftLaneWidth) * -1.0f;
  else
    fLen = (float)(m_chunkAy[i].iRightLaneWidth);
  glm::mat4 scaleMat = glm::scale(glm::vec3(fLen, fLen, fLen));
  lane = glm::vec3(translateMat * scaleMat * rollMat * glm::vec4(pitchAxis, 1.0f));
}

//-------------------------------------------------------------------------------------------------

void CTrack::GetShoulder(int i, glm::vec3 lLane, glm::vec3 pitchAxis, glm::mat4 rollMat, glm::vec3 nextChunkPitched,
                              glm::vec3 &shoulder, bool bLeft, bool bIgnoreHeight)
{
  glm::mat4 translateMat = glm::translate(lLane); //translate to end of left lane
  float fLen = 0.0f;
  float fHeight = 0.0f;
  if (bLeft) {
    fLen = (float)m_chunkAy[i].iLeftShoulderWidth * -1.0f;
    if (!bIgnoreHeight)
      fHeight = (float)m_chunkAy[i].iLeftShoulderHeight * -1.0f;
  } else {
    fLen = (float)m_chunkAy[i].iRightShoulderWidth;
    if (!bIgnoreHeight)
      fHeight = (float)m_chunkAy[i].iRightShoulderHeight * -1.0f;
  }
  glm::mat4 scaleMatWidth = glm::scale(glm::vec3(fLen, fLen, fLen));
  glm::mat4 scaleMatHeight = glm::scale(glm::vec3(fHeight, fHeight, fHeight));
  glm::vec3 widthVec = glm::vec3(scaleMatWidth * rollMat * glm::vec4(pitchAxis, 1.0f));
  glm::vec3 normal = glm::normalize(glm::cross(nextChunkPitched, pitchAxis));
  glm::vec3 heightVec = glm::vec3(scaleMatHeight * rollMat * glm::vec4(normal, 1.0f));
  glm::vec3 shoulderVec = widthVec + heightVec;
  shoulder = glm::vec3(translateMat * glm::vec4(shoulderVec, 1.0f));
}

//-------------------------------------------------------------------------------------------------

void CTrack::GetEnvirFloor(int i, glm::vec3 lShoulder, glm::vec3 rShoulder,
                               glm::vec3 &lEnvirFloor, glm::vec3 &rEnvirFloor)
{
  float fEnvirFloorDepth = (float)m_header.iFloorDepth * -1.0f;
  lEnvirFloor = lShoulder;
  rEnvirFloor = rShoulder;
  lEnvirFloor.y = fEnvirFloorDepth;
  rEnvirFloor.y = fEnvirFloorDepth;
}

//-------------------------------------------------------------------------------------------------

void CTrack::GetOWallFloor(int i, glm::vec3 lLane, glm::vec3 rLane, glm::vec3 pitchAxis, glm::vec3 nextChunkPitched,
                               glm::vec3 &lFloor, glm::vec3 &rFloor)
{
  glm::mat4 translateMatL = glm::translate(lLane);
  glm::mat4 translateMatR = glm::translate(rLane);
  float fEnvirFloorDepth = (float)m_header.iFloorDepth * -1.0f;
  float fLOFloorHeight = (float)m_chunkAy[i].iLOuterFloorHeight * 1.0f;
  float fROFloorHeight = (float)m_chunkAy[i].iROuterFloorHeight * 1.0f;
  float fROFloorOffset = (float)m_chunkAy[i].iROuterFloorHOffset * 1.0f;
  float fLOFloorOffset = (float)m_chunkAy[i].iLOuterFloorHOffset * -1.0f;

  glm::mat4 scaleMatRWidth = glm::scale(glm::vec3(fROFloorOffset, fROFloorOffset, fROFloorOffset));
  glm::vec3 rWidthVec = glm::vec3(scaleMatRWidth * glm::vec4(pitchAxis, 1.0f));
  glm::mat4 scaleMatLWidth = glm::scale(glm::vec3(fLOFloorOffset, fLOFloorOffset, fLOFloorOffset));
  glm::vec3 lWidthVec = glm::vec3(scaleMatLWidth * glm::vec4(pitchAxis, 1.0f));

  lFloor = glm::vec3(translateMatL * glm::vec4(lWidthVec, 1.0f));;
  rFloor = glm::vec3(translateMatR * glm::vec4(rWidthVec, 1.0f));;
  lFloor.y = fEnvirFloorDepth + fLOFloorHeight;
  rFloor.y = fEnvirFloorDepth + fROFloorHeight;
}

//-------------------------------------------------------------------------------------------------

void CTrack::GetWall(int i, glm::vec3 bottomAttach, glm::vec3 pitchAxis, glm::mat4 rollMat, glm::vec3 nextChunkPitched,
                            glm::vec3 &lloWall, eShapeSection wallSection)
{
  glm::mat4 translateMat = glm::translate(bottomAttach);
  float fHOffset = 0.0f;
  float fHeight = 0.0f;
  switch (wallSection) {
    case eShapeSection::LWALL:
      if (m_chunkAy[i].iLeftWallType != -1)
        fHeight = (float)m_chunkAy[i].iRoofHeight * -1.0f;
    case eShapeSection::RWALL:
      if (m_chunkAy[i].iRightWallType != -1)
        fHeight = (float)m_chunkAy[i].iRoofHeight * -1.0f;
      break;
    case eShapeSection::LLOWALL:
      fHOffset = (float)m_chunkAy[i].iLLOuterWallHOffset * -1.0f;
      fHeight = (float)m_chunkAy[i].iLLOuterWallHeight * -1.0f;
      break;
    case eShapeSection::RLOWALL:
      fHOffset = (float)m_chunkAy[i].iRLOuterWallHOffset;
      fHeight = (float)m_chunkAy[i].iRLOuterWallHeight * -1.0f;
      break;
    case eShapeSection::LUOWALL:
      fHOffset = (float)m_chunkAy[i].iLUOuterWallHOffset * -1.0f;
      fHeight = (float)m_chunkAy[i].iLUOuterWallHeight * -1.0f;
      break;
    case eShapeSection::RUOWALL:
      fHOffset = (float)m_chunkAy[i].iRUOuterWallHOffset;
      fHeight = (float)m_chunkAy[i].iRUOuterWallHeight * -1.0f;
      break;
    default:
      assert(0); //only wall sections should use this function
  }
  glm::mat4 scaleMatWidth = glm::scale(glm::vec3(fHOffset, fHOffset, fHOffset));
  glm::mat4 scaleMatHeight = glm::scale(glm::vec3(fHeight, fHeight, fHeight));
  glm::vec3 widthVec = glm::vec3(scaleMatWidth * rollMat * glm::vec4(pitchAxis, 1.0f));
  glm::vec3 normal = glm::normalize(glm::cross(nextChunkPitched, pitchAxis));
  glm::vec3 heightVec = glm::vec3(scaleMatHeight * rollMat * glm::vec4(normal, 1.0f));
  glm::vec3 wallVec = widthVec + heightVec;
  lloWall = glm::vec3(translateMat * glm::vec4(wallVec, 1.0f));
}

//-------------------------------------------------------------------------------------------------

void CTrack::GetAILine(int i, glm::vec3 center, glm::vec3 pitchAxis, glm::mat4 rollMat, glm::vec3 nextChunkPitched,
                           glm::vec3 &aiLine, eShapeSection lineSection, int iHeight)
{
  glm::mat4 translateMat = glm::translate(center);
  float fLen = 0.0f;
  int iUseAILine;
  switch (lineSection) {
    case eShapeSection::AILINE1:
      iUseAILine = m_chunkAy[i].iAILine1;
      break;
    case eShapeSection::AILINE2:
      iUseAILine = m_chunkAy[i].iAILine2;
      break;
    case eShapeSection::AILINE3:
      iUseAILine = m_chunkAy[i].iAILine3;
      break;
    case eShapeSection::AILINE4:
      iUseAILine = m_chunkAy[i].iAILine4;
      break;
    default:
      assert(0);
  }
  int iShoulderHeight = 0;
  if (iUseAILine > 0 && iUseAILine > m_chunkAy[i].iLeftLaneWidth) {
    //ai line must be on left shoulder
    float fTheta = atan((float)m_chunkAy[i].iLeftShoulderHeight / (float)m_chunkAy[i].iLeftShoulderWidth);
    int iLengthIntoShoulder = abs(iUseAILine) - m_chunkAy[i].iLeftLaneWidth;
    iShoulderHeight = (int)(tan(fTheta) * (float)iLengthIntoShoulder);
  }
  if (iUseAILine < 0 && abs(iUseAILine) > m_chunkAy[i].iRightLaneWidth) {
    //ai line must be on right shoulder
    float fTheta = atan((float)m_chunkAy[i].iRightShoulderHeight / (float)m_chunkAy[i].iRightShoulderWidth);
    int iLengthIntoShoulder = abs(iUseAILine) - m_chunkAy[i].iRightLaneWidth;
    iShoulderHeight = (int)(tan(fTheta) * (float)iLengthIntoShoulder);
  }

  fLen = (float)iUseAILine * -1.0f;
  float fHeight = (float)(iHeight + iShoulderHeight) * -1.0f;

  glm::mat4 scaleMatWidth = glm::scale(glm::vec3(fLen, fLen, fLen));
  glm::mat4 scaleMatHeight = glm::scale(glm::vec3(fHeight, fHeight, fHeight));
  glm::vec3 widthVec = glm::vec3(scaleMatWidth * rollMat * glm::vec4(pitchAxis, 1.0f));
  glm::vec3 normal = glm::normalize(glm::cross(nextChunkPitched, pitchAxis));
  glm::vec3 heightVec = glm::vec3(scaleMatHeight * rollMat * glm::vec4(normal, 1.0f));
  glm::vec3 lineVec = widthVec + heightVec;
  aiLine = glm::vec3(translateMat * glm::vec4(lineVec, 1.0f));
}

//-------------------------------------------------------------------------------------------------
