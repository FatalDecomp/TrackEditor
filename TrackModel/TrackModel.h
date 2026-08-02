#ifndef _TRACKEDITOR_TRACKMODEL_H
#define _TRACKEDITOR_TRACKMODEL_H
//-------------------------------------------------------------------------------------------------
#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <vector>
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
struct tGeometryChunk
{
  void Clear();
  void Default();

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

  //additional serialized sign data
  int iSignTexture;
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
class CTrackModel
{
public:
  CTrackModel();
  virtual ~CTrackModel() = default;

  virtual void ClearData();
  bool LoadTrack(const std::string &sFilename);
  virtual bool ProcessTrackData(const std::uint8_t *pData, size_t length);
  void GetTrackData(std::vector<std::uint8_t> &data);

  static unsigned int GetSignedBitValueFromInt(int iValue);
  static int GetIntValueFromSignedBit(unsigned int uiValue);

  tTrackHeader m_header;
  CChunkAy m_chunkAy;
  CStuntMap m_stuntMap;
  CSignMap m_backsMap;
  std::string m_sTrackFile;
  std::string m_sTrackFileFolder;
  std::string m_sTextureFile;
  std::string m_sBuildingFile;
  tRaceInfo m_raceInfo;

protected:
  bool IsNumber(const std::string &str) const;
  void ProcessSign(const std::vector<std::string> &lineAy, eFileSection &section);
  void WriteToVector(std::vector<std::uint8_t> &data, const char *szText) const;
  void GenerateChunkString(const tGeometryChunk &chunk, char *szBuf, int iSize) const;
};
//-------------------------------------------------------------------------------------------------
struct tTrackHistory
{
  std::string sDescription;
  std::vector<std::uint8_t> byteAy;
};
typedef std::vector<tTrackHistory> CHistoryAy;
//-------------------------------------------------------------------------------------------------
class CTrackHistory
{
public:
  CTrackHistory();

  void Clear();
  void Save(CTrackModel &track, const std::string &sDescription, size_t maxEntries);
  bool Undo(CTrackModel &track);
  bool Redo(CTrackModel &track);

  size_t GetEntryCount() const { return m_historyAy.size(); }
  int GetCurrentIndex() const { return m_iHistoryIndex; }
  const tTrackHistory *GetCurrentEntry() const;

private:
  bool Restore(CTrackModel &track);

  CHistoryAy m_historyAy;
  int m_iHistoryIndex;
};
//-------------------------------------------------------------------------------------------------
#endif
