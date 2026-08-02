#ifndef _WHIPLIB_TRACK_GEOMETRY_H
#define _WHIPLIB_TRACK_GEOMETRY_H
//-------------------------------------------------------------------------------------------------
#include "TrackModel.h"
#include "Types.h"
#include "glm.hpp"
#include <vector>
//-------------------------------------------------------------------------------------------------
struct tDerivedTrackChunk
{
  glm::vec3 center;
  glm::vec3 centerStunt;
  glm::vec3 pitchAxis;
  glm::vec3 nextChunkPitched;
  glm::vec3 normal;
  glm::vec3 lLane;
  glm::vec3 rLane;
  glm::vec3 lShoulder;
  glm::vec3 rShoulder;
  glm::vec3 lWall;
  glm::vec3 rWall;
  bool bLWallAttachToLane;
  glm::vec3 lWallBottomAttach;
  bool bRWallAttachToLane;
  glm::vec3 rWallBottomAttach;
  glm::vec3 lFloor;
  glm::vec3 rFloor;
  glm::vec3 lloWall;
  glm::vec3 rloWall;
  bool bLloWallAttachToShoulder;
  glm::vec3 lloWallBottomAttach;
  bool bRloWallAttachToShoulder;
  glm::vec3 rloWallBottomAttach;
  glm::vec3 luoWall;
  glm::vec3 ruoWall;
  glm::vec3 aiLine1;
  glm::vec3 aiLine2;
  glm::vec3 aiLine3;
  glm::vec3 aiLine4;
  glm::vec3 carLine1;
  glm::vec3 carLine2;
  glm::vec3 carLine3;
  glm::vec3 carLine4;
  glm::mat4 yawMat;
  glm::mat4 pitchMat;
  glm::mat4 rollMat;
};
using CDerivedTrackChunkAy = std::vector<tDerivedTrackChunk>;
//-------------------------------------------------------------------------------------------------
// Legacy rendering/export geometry is intentionally derived on demand and never stored in CTrack.
class CTrackGeometry
{
public:
  explicit CTrackGeometry(const CTrackModel &Track);

  const CDerivedTrackChunkAy &GetChunks() const { return m_chunkGeometryAy; }

private:
  void GetCenter(int i, glm::vec3 prevCenter,
                 glm::vec3 &center, glm::vec3 &pitchAxis, glm::vec3 &nextChunkPitched, glm::vec3 &normal,
                 glm::mat4 &yawMat, glm::mat4 &pitchMat, glm::mat4 &rollMat);
  void GetLane(int i, glm::vec3 center, glm::vec3 pitchAxis, glm::mat4 rollMat,
               glm::vec3 &lane, bool bLeft);
  void GetShoulder(int i, glm::vec3 attach, glm::vec3 pitchAxis, glm::mat4 rollMat, glm::vec3 nextChunkPitched,
                   glm::vec3 &shoulder, bool bLeft, bool bIgnoreHeight = false);
  void GetEnvirFloor(int i, glm::vec3 lShoulder, glm::vec3 rShoulder,
                     glm::vec3 &lEnvirFloor, glm::vec3 &rEnvirFloor);
  void GetOWallFloor(int i, glm::vec3 lLane, glm::vec3 rLane, glm::vec3 pitchAxis, glm::vec3 nextChunkPitched,
                     glm::vec3 &lFloor, glm::vec3 &rFloor);
  void GetWall(int i, glm::vec3 bottomAttach, glm::vec3 pitchAxis, glm::mat4 rollMat, glm::vec3 nextChunkPitched,
               glm::vec3 &lloWall, eShapeSection wallSection);
  void GetAILine(int i, glm::vec3 center, glm::vec3 pitchAxis, glm::mat4 rollMat, glm::vec3 nextChunkPitched,
                 glm::vec3 &aiLine, eShapeSection lineSection, int iHeight);

  const CChunkAy &m_chunkAy;
  const tTrackHeader &m_header;
  int m_iAILineHeight;
  CDerivedTrackChunkAy m_chunkGeometryAy;
};
//-------------------------------------------------------------------------------------------------
#endif
