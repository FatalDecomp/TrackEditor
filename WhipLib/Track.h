#ifndef _WHIPLIB_TRACK_H
#define _WHIPLIB_TRACK_H
//-------------------------------------------------------------------------------------------------
#include "TrackModel.h"
#include "glm.hpp"
#include <vector>
#include "Types.h"
//-------------------------------------------------------------------------------------------------
struct tChunkMath
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
  glm::vec3 aiLine1; //visualized AI lines positioned above track
  glm::vec3 aiLine2;
  glm::vec3 aiLine3;
  glm::vec3 aiLine4;
  glm::vec3 carLine1; //actual AI lines at track surface level
  glm::vec3 carLine2;
  glm::vec3 carLine3;
  glm::vec3 carLine4;
  glm::mat4 yawMat;
  glm::mat4 pitchMat;
  glm::mat4 rollMat;
};
typedef std::vector<tChunkMath> CChunkMathAy;
//-------------------------------------------------------------------------------------------------
class CTexture;
class CPalette;
//-------------------------------------------------------------------------------------------------

class CTrack : public CTrackModel
{
public:
  CTrack();
  ~CTrack() override;

  void ClearData() override;
  bool LoadTextures();
  bool ProcessTrackData(const uint8 *pData, size_t length) override;
  void GenerateTrackMath();
  void ResetStunts();
  void UpdateStunts();
  bool ShouldShowChunkSection(int i, eShapeSection section);
  bool HasPitchedStunt();
  bool UseCenterStunt(int i);
  void CollideWithChunk(const glm::vec3 &position, int &iClosestChunk, int &iPrevChunk,
                        glm::vec3 &p0, glm::vec3 &p1, glm::vec3 &p2, glm::vec3 &p3, const glm::vec3 &peg1, const glm::vec3 &peg2);
  void ProjectToTrack(glm::vec3 &position, glm::mat4 &rotationMat, const glm::vec3 &up,
                      glm::vec3 &p0, glm::vec3 &p1, glm::vec3 &p2,
                      const glm::vec3 &peg1, const glm::vec3 &peg2);

  static bool ShouldDrawSurfaceType(int iSurfaceType);

  CChunkMathAy m_chunkMathAy;
  int m_iAILineHeight;

  CPalette *m_pPal;
  CTexture *m_pTex;
  CTexture *m_pBld;

protected:
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

private:
  std::string m_sLastLoadedTex;
  std::string m_sLastLoadedBld;
  std::string m_sLastLoadedPal;
};

//-------------------------------------------------------------------------------------------------
#endif
