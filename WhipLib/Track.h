#ifndef _WHIPLIB_TRACK_H
#define _WHIPLIB_TRACK_H
//-------------------------------------------------------------------------------------------------
#include "glm.hpp"
#include <vector>
#include <map>
#include <string>
#include "Types.h"
//-------------------------------------------------------------------------------------------------
#define STUNT_LENGTH_100_PERCENT 1024
//-------------------------------------------------------------------------------------------------
struct tTrackHeader
{
  int iNumChunks;
  int iHeaderUnk1;
  int iHeaderUnk2;
  int iFloorDepth;
};
//-------------------------------------------------------------------------------------------------
struct tStunt
{
  int iChunkCount;
  int iNumTicks;
  int iTickStartIdx;
  int iTimingGroup;
  int iHeight;
  int iTimeBulging;
  int iTimeFlat;
  int iRampSideLength;
  int iFlags;
  int iTickCurrIdx;
};
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
//-------------------------------------------------------------------------------------------------
struct tGeometryChunk
{
  void Clear();

  //line 1
  int iLeftShoulderWidth;
  int iLeftLaneWidth;
  int iRightLaneWidth;
  int iRightShoulderWidth;
  int iLeftShoulderHeight;
  int iRightShoulderHeight;
  int iLength;
  double dYaw;
  double dPitch;
  double dRoll;
  int iAILine1;
  int iAILine2;
  int iAILine3;
  int iAILine4;
  int iTrackGrip;
  int iLeftShoulderGrip;
  int iRightShoulderGrip;
  int iAIMaxSpeed;
  int iGroundHeight;
  int iAudioAboveTrigger;
  int iAudioTriggerSpeed;
  int iAudioBelowTrigger;
  //line 2
  int iLeftSurfaceType;
  int iCenterSurfaceType;
  int iRightSurfaceType;
  int iLeftWallType;
  int iRightWallType;
  int iRoofType;
  int iLUOuterWallType;
  int iLLOuterWallType;
  int iOuterFloorType;
  int iRLOuterWallType;
  int iRUOuterWallType;
  int iEnvironmentFloorType;
  int iSignType;
  int iSignHorizOffset;
  int iSignVertOffset;
  double dSignYaw;
  double dSignPitch;
  double dSignRoll;
  //line 3
  int iLUOuterWallHOffset;
  int iLLOuterWallHOffset;
  int iLOuterFloorHOffset;
  int iROuterFloorHOffset;
  int iRLOuterWallHOffset;
  int iRUOuterWallHOffset;
  int iLUOuterWallHeight;
  int iLLOuterWallHeight;
  int iLOuterFloorHeight;
  int iROuterFloorHeight;
  int iRLOuterWallHeight;
  int iRUOuterWallHeight;
  int iRoofHeight;
  int iNearForward;
  int iNearForwardExStart;
  int iNearForwardEx;
  int iLeftSubdivDist;
  int iCenterSubdivDist;
  int iRightSubdivDist;
  int iLWallSubdivDist;
  int iRWallSubdivDist;
  int iRoofSubdivDist;
  int iLUOuterWallSubdivDist;
  int iLLOuterWallSubdivDist;
  int iOuterFloorSubdivDist;
  int iRLOuterWallSubdivDist;
  int iRUOuterWallSubdivDist;
  int iNearBackward;
  int iNearBackwardExStart;
  int iNearBackwardEx;

  //additional data
  int iSignTexture;

  //math
  tChunkMath math;
};
typedef std::vector<tGeometryChunk> CChunkAy;
//-------------------------------------------------------------------------------------------------
typedef std::map<int, int> CSignMap;
typedef std::map<int, tStunt> CStuntMap;
//-------------------------------------------------------------------------------------------------
struct tRaceInfo
{
  int iTrackNumber;
  int iImpossibleLaps;
  int iHardLaps;
  int iTrickyLaps;
  int iMediumLaps;
  int iEasyLaps;
  int iGirlieLaps;
  double dTrackMapSize;
  int iTrackMapFidelity;
  double dPreviewSize;
};
//-------------------------------------------------------------------------------------------------
enum class eFileSection
{
  HEADER = 0,
  GEOMETRY,
  SIGNS,
  STUNTS,
  TEXTURE,
  TRACK_NUM,
  LAPS,
  MAP,
  END
};
//-------------------------------------------------------------------------------------------------
class CTexture;
class CPalette;
//-------------------------------------------------------------------------------------------------

class CTrack
{
public:
  CTrack();
  ~CTrack();

  void ClearData();
  bool LoadTrack(const std::string &sFilename);
  bool LoadTextures();
  bool ProcessTrackData(const uint8 *pData, size_t length);
  void GetTrackData(std::vector<uint8> &data);
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

  static unsigned int GetSignedBitValueFromInt(int iValue);
  static int GetIntValueFromSignedBit(unsigned int uiValue);
  static bool ShouldDrawSurfaceType(int iSurfaceType);

  tTrackHeader m_header;
  CChunkAy m_chunkAy;
  CStuntMap m_stuntMap;
  CSignMap m_backsMap;
  std::string m_sTrackFile;
  std::string m_sTrackFileFolder;
  std::string m_sTextureFile;
  std::string m_sBuildingFile;
  tRaceInfo m_raceInfo;
  int m_iAILineHeight;

  CPalette *m_pPal;
  CTexture *m_pTex;
  CTexture *m_pBld;

protected:
  bool IsNumber(const std::string &str);
  void ProcessSign(const std::vector<std::string> &lineAy, eFileSection &section);
  void WriteToVector(std::vector<uint8> &data, const char *szText);
  void GenerateChunkString(tGeometryChunk &chunk, char *szBuf, int iSize);

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
