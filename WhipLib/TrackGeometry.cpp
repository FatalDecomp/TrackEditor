#include "TrackGeometry.h"
#include "Texture.h"
#include "gtc/matrix_transform.hpp"
#include "gtx/transform.hpp"
//-------------------------------------------------------------------------------------------------

CTrackGeometry::CTrackGeometry(const CTrackModel &Track)
  : m_chunkAy(Track.m_chunkAy)
  , m_header(Track.m_header)
  , m_iAILineHeight(100)
{
  m_chunkGeometryAy.assign(m_chunkAy.size(), tDerivedTrackChunk{});
  if (m_chunkAy.empty()) {
    return;
  }

  m_chunkGeometryAy[0].center = glm::vec3(0, 0, 0);
  for (uint32 i = 0; i < m_chunkAy.size(); ++i) {
    glm::vec3 nextCenter;
    GetCenter(i, m_chunkGeometryAy[i].center,
              nextCenter,
              m_chunkGeometryAy[i].pitchAxis,
              m_chunkGeometryAy[i].nextChunkPitched,
              m_chunkGeometryAy[i].normal,
              m_chunkGeometryAy[i].yawMat,
              m_chunkGeometryAy[i].pitchMat,
              m_chunkGeometryAy[i].rollMat);
    if (i + 1 < m_chunkAy.size())
      m_chunkGeometryAy[i + 1].center = nextCenter;
  }
  for (uint32 i = 0; i < m_chunkAy.size(); ++i) {
    int iPrevIndex = (int)m_chunkAy.size() - 1;
    if (i > 0)
      iPrevIndex = i - 1;

    glm::mat4 rollMatNoRoll = glm::mat4(1);

    //left lane
    GetLane(i,
            m_chunkGeometryAy[i].center,
            m_chunkGeometryAy[i].pitchAxis,
            m_chunkGeometryAy[i].rollMat,
            m_chunkGeometryAy[i].lLane, true);
    //right lane
    GetLane(i,
            m_chunkGeometryAy[i].center,
            m_chunkGeometryAy[i].pitchAxis,
            m_chunkGeometryAy[i].rollMat,
            m_chunkGeometryAy[i].rLane, false);
    //left shoulder
    GetShoulder(i,
                m_chunkGeometryAy[i].lLane,
                m_chunkGeometryAy[i].pitchAxis,
                m_chunkGeometryAy[i].rollMat,
                m_chunkGeometryAy[i].nextChunkPitched,
                m_chunkGeometryAy[i].lShoulder, true);
    //right shoulder
    GetShoulder(i,
                m_chunkGeometryAy[i].rLane,
                m_chunkGeometryAy[i].pitchAxis,
                m_chunkGeometryAy[i].rollMat,
                m_chunkGeometryAy[i].nextChunkPitched,
                m_chunkGeometryAy[i].rShoulder, false);
    //left wall
    m_chunkGeometryAy[i].bLWallAttachToLane = CTrackModel::GetSignedBitValueFromInt(m_chunkAy[i].iLeftWallType) & SURFACE_FLAG_WALL_31;
    if (m_chunkAy[i].iLeftWallType == -1)
      m_chunkGeometryAy[i].bLWallAttachToLane = m_chunkGeometryAy[iPrevIndex].bLWallAttachToLane;
    m_chunkGeometryAy[i].lWallBottomAttach = m_chunkGeometryAy[i].bLWallAttachToLane ? m_chunkGeometryAy[i].lLane : m_chunkGeometryAy[i].lShoulder;
    GetWall(i,
            m_chunkGeometryAy[i].lWallBottomAttach,
            m_chunkGeometryAy[i].pitchAxis,
            m_chunkGeometryAy[i].rollMat,
            m_chunkGeometryAy[i].nextChunkPitched,
            m_chunkGeometryAy[i].lWall, eShapeSection::LWALL);
    //right wall
    m_chunkGeometryAy[i].bRWallAttachToLane = CTrackModel::GetSignedBitValueFromInt(m_chunkAy[i].iRightWallType) & SURFACE_FLAG_WALL_31;
    if (m_chunkAy[i].iRightWallType == -1)
      m_chunkGeometryAy[i].bRWallAttachToLane = m_chunkGeometryAy[iPrevIndex].bRWallAttachToLane;
    m_chunkGeometryAy[i].rWallBottomAttach = m_chunkGeometryAy[i].bRWallAttachToLane ? m_chunkGeometryAy[i].rLane : m_chunkGeometryAy[i].rShoulder;
    GetWall(i,
            m_chunkGeometryAy[i].rWallBottomAttach,
            m_chunkGeometryAy[i].pitchAxis,
            m_chunkGeometryAy[i].rollMat,
            m_chunkGeometryAy[i].nextChunkPitched,
            m_chunkGeometryAy[i].rWall, eShapeSection::RWALL);
    //outer floor
    glm::vec3 lLaneNoRoll;
    GetLane(i,
            m_chunkGeometryAy[i].center,
            m_chunkGeometryAy[i].pitchAxis,
            rollMatNoRoll, lLaneNoRoll, true);
    glm::vec3 rLaneNoRoll;
    GetLane(i,
            m_chunkGeometryAy[i].center,
            m_chunkGeometryAy[i].pitchAxis,
            rollMatNoRoll, rLaneNoRoll, false);
    GetOWallFloor(i, lLaneNoRoll, rLaneNoRoll,
                  m_chunkGeometryAy[i].pitchAxis,
                  m_chunkGeometryAy[i].nextChunkPitched,
                  m_chunkGeometryAy[i].lFloor,
                  m_chunkGeometryAy[i].rFloor);
    //outer wall roll mat
    glm::mat4 oWallRollMat = m_chunkAy[i].iOuterFloorType < 0 ? m_chunkGeometryAy[i].rollMat : rollMatNoRoll;
    //llowall
    m_chunkGeometryAy[i].lloWallBottomAttach = m_chunkGeometryAy[i].lFloor;
    if (m_chunkAy[i].iOuterFloorType < 0) {
      m_chunkGeometryAy[i].bLloWallAttachToShoulder = true;
      m_chunkGeometryAy[i].lloWallBottomAttach = m_chunkGeometryAy[i].lShoulder;
    }
    GetWall(i,
            m_chunkGeometryAy[i].lloWallBottomAttach,
            m_chunkGeometryAy[i].pitchAxis, oWallRollMat,
            m_chunkGeometryAy[i].nextChunkPitched,
            m_chunkGeometryAy[i].lloWall, eShapeSection::LLOWALL);
    //rlowall
    m_chunkGeometryAy[i].rloWallBottomAttach = m_chunkGeometryAy[i].rFloor;
    if (m_chunkAy[i].iOuterFloorType < 0) {
      m_chunkGeometryAy[i].bRloWallAttachToShoulder = true;
      m_chunkGeometryAy[i].rloWallBottomAttach = m_chunkGeometryAy[i].rShoulder;
    }
    GetWall(i,
            m_chunkGeometryAy[i].rloWallBottomAttach,
            m_chunkGeometryAy[i].pitchAxis, oWallRollMat,
            m_chunkGeometryAy[i].nextChunkPitched,
            m_chunkGeometryAy[i].rloWall, eShapeSection::RLOWALL);
    //luowall
    GetWall(i,
            m_chunkGeometryAy[i].lloWall,
            m_chunkGeometryAy[i].pitchAxis, oWallRollMat,
            m_chunkGeometryAy[i].nextChunkPitched,
            m_chunkGeometryAy[i].luoWall, eShapeSection::LUOWALL);
    //ruowall
    GetWall(i,
            m_chunkGeometryAy[i].rloWall,
            m_chunkGeometryAy[i].pitchAxis, oWallRollMat,
            m_chunkGeometryAy[i].nextChunkPitched,
            m_chunkGeometryAy[i].ruoWall, eShapeSection::RUOWALL);
    //ailines
    GetAILine(i,
              m_chunkGeometryAy[i].center,
              m_chunkGeometryAy[i].pitchAxis,
              m_chunkGeometryAy[i].rollMat,
              m_chunkGeometryAy[i].nextChunkPitched,
              m_chunkGeometryAy[i].aiLine1,
              eShapeSection::AILINE1, m_iAILineHeight);
    GetAILine(i,
              m_chunkGeometryAy[i].center,
              m_chunkGeometryAy[i].pitchAxis,
              m_chunkGeometryAy[i].rollMat,
              m_chunkGeometryAy[i].nextChunkPitched,
              m_chunkGeometryAy[i].aiLine2,
              eShapeSection::AILINE2, m_iAILineHeight);
    GetAILine(i,
              m_chunkGeometryAy[i].center,
              m_chunkGeometryAy[i].pitchAxis,
              m_chunkGeometryAy[i].rollMat,
              m_chunkGeometryAy[i].nextChunkPitched,
              m_chunkGeometryAy[i].aiLine3, eShapeSection::AILINE3, m_iAILineHeight);
    GetAILine(i,
              m_chunkGeometryAy[i].center,
              m_chunkGeometryAy[i].pitchAxis,
              m_chunkGeometryAy[i].rollMat,
              m_chunkGeometryAy[i].nextChunkPitched,
              m_chunkGeometryAy[i].aiLine4,
              eShapeSection::AILINE4, m_iAILineHeight);
    //car positions are ai lines with 0 height
    GetAILine(i,
              m_chunkGeometryAy[i].center,
              m_chunkGeometryAy[i].pitchAxis,
              m_chunkGeometryAy[i].rollMat,
              m_chunkGeometryAy[i].nextChunkPitched,
              m_chunkGeometryAy[i].carLine1,
              eShapeSection::AILINE1, 0);
    GetAILine(i,
              m_chunkGeometryAy[i].center,
              m_chunkGeometryAy[i].pitchAxis,
              m_chunkGeometryAy[i].rollMat,
              m_chunkGeometryAy[i].nextChunkPitched,
              m_chunkGeometryAy[i].carLine2,
              eShapeSection::AILINE2, 0);
    GetAILine(i,
              m_chunkGeometryAy[i].center,
              m_chunkGeometryAy[i].pitchAxis,
              m_chunkGeometryAy[i].rollMat,
              m_chunkGeometryAy[i].nextChunkPitched,
              m_chunkGeometryAy[i].carLine3,
              eShapeSection::AILINE3, 0);
    GetAILine(i,
              m_chunkGeometryAy[i].center,
              m_chunkGeometryAy[i].pitchAxis,
              m_chunkGeometryAy[i].rollMat,
              m_chunkGeometryAy[i].nextChunkPitched,
              m_chunkGeometryAy[i].carLine4,
              eShapeSection::AILINE4, 0);

    m_chunkGeometryAy[i].centerStunt = m_chunkGeometryAy[i].center;
  }
}

//-------------------------------------------------------------------------------------------------

void CTrackGeometry::GetCenter(int i, glm::vec3 prevCenter,
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

void CTrackGeometry::GetLane(int i, glm::vec3 center, glm::vec3 pitchAxis, glm::mat4 rollMat,
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

void CTrackGeometry::GetShoulder(int i, glm::vec3 lLane, glm::vec3 pitchAxis, glm::mat4 rollMat, glm::vec3 nextChunkPitched,
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

void CTrackGeometry::GetEnvirFloor([[maybe_unused]] int i, glm::vec3 lShoulder, glm::vec3 rShoulder,
                               glm::vec3 &lEnvirFloor, glm::vec3 &rEnvirFloor)
{
  float fEnvirFloorDepth = (float)m_header.iFloorDepth * -1.0f;
  lEnvirFloor = lShoulder;
  rEnvirFloor = rShoulder;
  lEnvirFloor.y = fEnvirFloorDepth;
  rEnvirFloor.y = fEnvirFloorDepth;
}

//-------------------------------------------------------------------------------------------------

void CTrackGeometry::GetOWallFloor(int i, glm::vec3 lLane, glm::vec3 rLane, glm::vec3 pitchAxis,
                               [[maybe_unused]] glm::vec3 nextChunkPitched,
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

void CTrackGeometry::GetWall(int i, glm::vec3 bottomAttach, glm::vec3 pitchAxis, glm::mat4 rollMat, glm::vec3 nextChunkPitched,
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

void CTrackGeometry::GetAILine(int i, glm::vec3 center, glm::vec3 pitchAxis, glm::mat4 rollMat, glm::vec3 nextChunkPitched,
                           glm::vec3 &aiLine, eShapeSection lineSection, int iHeight)
{
  glm::mat4 translateMat = glm::translate(center);
  float fLen = 0.0f;
  // assert(0) in the default case compiles out of Release, so this was read
  // uninitialized for any non-AILINE section.
  int iUseAILine = 0;
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
