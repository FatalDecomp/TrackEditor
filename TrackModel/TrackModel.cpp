#include "TrackModel.h"
#include "Logging.h"
#include "Unmangler.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
//-------------------------------------------------------------------------------------------------
#define HEADER_ELEMENT_COUNT 4
#define SIGNS_COUNT 2
#define STUNTS_COUNT 10
#define BACKS_COUNT 2
#define LAPS_COUNT 6
#define MAP_COUNT 3
//-------------------------------------------------------------------------------------------------
namespace
{
double ConstrainAngle(double dAngle)
{
  dAngle = std::fmod(dAngle, 360.0);
  if (dAngle < 0.0)
    dAngle += 360.0;
  return dAngle;
}

bool SignTypeCanHaveTexture(int iSignType)
{
  switch (iSignType) {
    case 0:
    case 2:
    case 3:
    case 7:
    case 9:
    case 11:
    case 12:
    case 15:
    case 16:
      return true;
    default:
      return false;
  }
}
}
//-------------------------------------------------------------------------------------------------

void tGeometryChunk::Clear()
{
  *this = {};
}

//-------------------------------------------------------------------------------------------------

void tGeometryChunk::Default()
{
  Clear();

  iLeftShoulderWidth = 1000;
  iLeftLaneWidth = 1000;
  iRightLaneWidth = 1000;
  iRightShoulderWidth = 1000;
  iLength = 1000;
  iAIMaxSpeed = 1000;

  iLeftSurfaceType = 96;
  iCenterSurfaceType = 26;
  iRightSurfaceType = 100;
  iLeftWallType = -1;
  iRightWallType = -1;
  iRoofType = -1;
  iLUOuterWallType = -1;
  iLLOuterWallType = -1;
  iOuterFloorType = -1;
  iRLOuterWallType = -1;
  iRUOuterWallType = -1;
  iEnvironmentFloorType = 267;
  iSignType = -1;

  iNearForward = 32;
  iNearForwardExStart = -1;
  iLeftSubdivDist = 20;
  iCenterSubdivDist = 20;
  iRightSubdivDist = 20;
  iLWallSubdivDist = 20;
  iRWallSubdivDist = 20;
  iRoofSubdivDist = 20;
  iLUOuterWallSubdivDist = 1;
  iLLOuterWallSubdivDist = 20;
  iOuterFloorSubdivDist = 1;
  iRLOuterWallSubdivDist = 20;
  iRUOuterWallSubdivDist = 1;
  iNearBackward = 32;
  iNearBackwardExStart = -1;
}

//-------------------------------------------------------------------------------------------------

CTrackModel::CTrackModel()
{
  ClearData();
}

//-------------------------------------------------------------------------------------------------

void CTrackModel::ClearData()
{
  m_header = {};
  m_header.iFloorDepth = 2048;
  m_chunkAy.clear();
  m_stuntMap.clear();
  m_backsMap.clear();
  m_sTrackFile.clear();
  m_sTextureFile.clear();
  m_sBuildingFile.clear();
  m_raceInfo = {};
}

//-------------------------------------------------------------------------------------------------

bool CTrackModel::LoadTrack(const std::string &sFilename)
{
  ClearData();

  if (sFilename.empty()) {
    Logging::LogMessage("Track filename empty");
    return false;
  }

  m_sTrackFile = sFilename;
  m_sTrackFileFolder = sFilename;
  size_t pos = sFilename.find_last_of('/');
  if (pos == std::string::npos)
    pos = sFilename.find_last_of('\\');
  if (pos != std::string::npos && pos < sFilename.size())
    m_sTrackFileFolder = sFilename.substr(0, pos + 1);

  std::ifstream file(sFilename.c_str(), std::ios::binary);
  if (!file.is_open()) {
    Logging::LogMessage("Failed to open track: %s", sFilename.c_str());
    return false;
  }

  file.seekg(0, file.end);
  const std::streamoff streamLength = file.tellg();
  file.seekg(0, file.beg);
  if (streamLength <= 0) {
    Logging::LogMessage("Track file %s is empty", sFilename.c_str());
    return false;
  }

  const size_t length = static_cast<size_t>(streamLength);
  std::vector<std::uint8_t> fileData(length);
  file.read(reinterpret_cast<char *>(fileData.data()), static_cast<std::streamsize>(length));
  if (!file) {
    Logging::LogMessage("Failed to read track: %s", sFilename.c_str());
    return false;
  }

  bool bSuccess = false;
  const int iUnmangledLength = Unmangler::GetUnmangledLength(fileData.data(), static_cast<int>(length));
  if (iUnmangledLength > 0 && iUnmangledLength < MAX_MANGLED_LENGTH) {
    Logging::LogMessage("Track file %s is mangled", sFilename.c_str());
    std::vector<std::uint8_t> unmangledData(static_cast<size_t>(iUnmangledLength));
    bSuccess = Unmangler::UnmangleFile(fileData.data(), static_cast<int>(length),
                                       unmangledData.data(), iUnmangledLength);
    Logging::LogMessage("%s track file %s", bSuccess ? "Unmangled" : "Failed to unmangle",
                        sFilename.c_str());
    if (bSuccess)
      bSuccess = ProcessTrackData(unmangledData.data(), unmangledData.size());
  } else {
    bSuccess = ProcessTrackData(fileData.data(), fileData.size());
  }

  Logging::LogMessage("%s track file: %s", bSuccess ? "Loaded" : "Failed to load",
                      sFilename.c_str());
  return bSuccess;
}

//-------------------------------------------------------------------------------------------------

bool CTrackModel::IsNumber(const std::string &str) const
{
  if (str.empty())
    return false;
  char *pEnd = nullptr;
  std::strtol(str.c_str(), &pEnd, 10);
  return pEnd && *pEnd == '\0';
}

//-------------------------------------------------------------------------------------------------

bool CTrackModel::ProcessTrackData(const std::uint8_t *pData, size_t length)
{
  if (!pData || length == 0)
    return false;

  bool bSuccess = true;
  int iChunkLine = 0;
  tGeometryChunk currChunk;
  eFileSection section = eFileSection::HEADER;

  const std::string fileText(reinterpret_cast<const char *>(pData), length);
  std::stringstream ssFile(fileText);
  std::string sLine;
  const std::string paddingToken(4, static_cast<char>(0xff));
  while (std::getline(ssFile, sLine, '\n')) {
    std::vector<std::string> lineAy;
    std::stringstream ssLine(sLine);
    std::string sSubStr;
    while (std::getline(ssLine, sSubStr, ' ')) {
      if (!sSubStr.empty() && sSubStr.back() == '\r')
        sSubStr.pop_back();
      if (!sSubStr.empty() && sSubStr.find(paddingToken) == std::string::npos)
        lineAy.push_back(sSubStr);
    }

    switch (section) {
      case eFileSection::HEADER:
        if (lineAy.size() == HEADER_ELEMENT_COUNT) {
          m_header.iNumChunks = std::stoi(lineAy[0]);
          m_header.iHeaderUnk1 = std::stoi(lineAy[1]);
          m_header.iHeaderUnk2 = std::stoi(lineAy[2]);
          m_header.iFloorDepth = std::stoi(lineAy[3]);
          section = eFileSection::GEOMETRY;
        }
        break;

      case eFileSection::GEOMETRY:
        if (iChunkLine == 0) {
          if (lineAy.empty()) {
            break;
          } else if (lineAy.size() == SIGNS_COUNT) {
            if (m_chunkAy.size() != static_cast<size_t>(m_header.iNumChunks)) {
              Logging::LogMessage(
                  "Warning loading file: number of chunks loaded (%d) does not match header (%d)",
                  static_cast<int>(m_chunkAy.size()), m_header.iNumChunks);
            }
            section = eFileSection::SIGNS;
            ProcessSign(lineAy, section);
            break;
          }

          currChunk.Clear();
          currChunk.iSignTexture = -1;
          if (lineAy.size() > 0) currChunk.iLeftShoulderWidth = std::stoi(lineAy[0]);
          if (lineAy.size() > 1) currChunk.iLeftLaneWidth = std::stoi(lineAy[1]);
          if (lineAy.size() > 2) currChunk.iRightLaneWidth = std::stoi(lineAy[2]);
          if (lineAy.size() > 3) currChunk.iRightShoulderWidth = std::stoi(lineAy[3]);
          if (lineAy.size() > 4) currChunk.iLeftShoulderHeight = std::stoi(lineAy[4]);
          if (lineAy.size() > 5) currChunk.iRightShoulderHeight = std::stoi(lineAy[5]);
          if (lineAy.size() > 6) currChunk.iLength = std::stoi(lineAy[6]);
          if (lineAy.size() > 7) currChunk.dYaw = ConstrainAngle(std::stod(lineAy[7]));
          if (lineAy.size() > 8) currChunk.dPitch = ConstrainAngle(std::stod(lineAy[8]));
          if (lineAy.size() > 9) currChunk.dRoll = ConstrainAngle(std::stod(lineAy[9]));
          if (lineAy.size() > 10) currChunk.iAILine1 = std::stoi(lineAy[10]);
          if (lineAy.size() > 11) currChunk.iAILine2 = std::stoi(lineAy[11]);
          if (lineAy.size() > 12) currChunk.iAILine3 = std::stoi(lineAy[12]);
          if (lineAy.size() > 13) currChunk.iAILine4 = std::stoi(lineAy[13]);
          if (lineAy.size() > 14) currChunk.iTrackGrip = std::stoi(lineAy[14]);
          if (lineAy.size() > 15) currChunk.iLeftShoulderGrip = std::stoi(lineAy[15]);
          if (lineAy.size() > 16) currChunk.iRightShoulderGrip = std::stoi(lineAy[16]);
          if (lineAy.size() > 17) currChunk.iAIMaxSpeed = std::stoi(lineAy[17]);
          if (lineAy.size() > 18) currChunk.iGroundHeight = std::stoi(lineAy[18]);
          if (lineAy.size() > 19) currChunk.iAudioAboveTrigger = std::stoi(lineAy[19]);
          if (lineAy.size() > 20) currChunk.iAudioTriggerSpeed = std::stoi(lineAy[20]);
          if (lineAy.size() > 21) currChunk.iAudioBelowTrigger = std::stoi(lineAy[21]);
          ++iChunkLine;
        } else if (iChunkLine == 1) {
          if (lineAy.size() > 0) currChunk.iLeftSurfaceType = std::stoi(lineAy[0]);
          if (lineAy.size() > 1) currChunk.iCenterSurfaceType = std::stoi(lineAy[1]);
          if (lineAy.size() > 2) currChunk.iRightSurfaceType = std::stoi(lineAy[2]);
          if (lineAy.size() > 3) currChunk.iLeftWallType = std::stoi(lineAy[3]);
          if (lineAy.size() > 4) currChunk.iRightWallType = std::stoi(lineAy[4]);
          if (lineAy.size() > 5) currChunk.iRoofType = std::stoi(lineAy[5]);
          if (lineAy.size() > 6) currChunk.iLUOuterWallType = std::stoi(lineAy[6]);
          if (lineAy.size() > 7) currChunk.iLLOuterWallType = std::stoi(lineAy[7]);
          if (lineAy.size() > 8) currChunk.iOuterFloorType = std::stoi(lineAy[8]);
          if (lineAy.size() > 9) currChunk.iRLOuterWallType = std::stoi(lineAy[9]);
          if (lineAy.size() > 10) currChunk.iRUOuterWallType = std::stoi(lineAy[10]);
          if (lineAy.size() > 11) currChunk.iEnvironmentFloorType = std::stoi(lineAy[11]);
          if (lineAy.size() > 12) currChunk.iSignType = std::stoi(lineAy[12]);
          if (lineAy.size() > 13) currChunk.iSignHorizOffset = std::stoi(lineAy[13]);
          if (lineAy.size() > 14) currChunk.iSignVertOffset = std::stoi(lineAy[14]);
          if (lineAy.size() > 15) currChunk.dSignYaw = ConstrainAngle(std::stod(lineAy[15]));
          if (lineAy.size() > 16) currChunk.dSignPitch = ConstrainAngle(std::stod(lineAy[16]));
          if (lineAy.size() > 17) currChunk.dSignRoll = ConstrainAngle(std::stod(lineAy[17]));
          ++iChunkLine;
        } else {
          if (lineAy.size() > 0) currChunk.iLUOuterWallHOffset = std::stoi(lineAy[0]);
          if (lineAy.size() > 1) currChunk.iLLOuterWallHOffset = std::stoi(lineAy[1]);
          if (lineAy.size() > 2) currChunk.iLOuterFloorHOffset = std::stoi(lineAy[2]);
          if (lineAy.size() > 3) currChunk.iROuterFloorHOffset = std::stoi(lineAy[3]);
          if (lineAy.size() > 4) currChunk.iRLOuterWallHOffset = std::stoi(lineAy[4]);
          if (lineAy.size() > 5) currChunk.iRUOuterWallHOffset = std::stoi(lineAy[5]);
          if (lineAy.size() > 6) currChunk.iLUOuterWallHeight = std::stoi(lineAy[6]);
          if (lineAy.size() > 7) currChunk.iLLOuterWallHeight = std::stoi(lineAy[7]);
          if (lineAy.size() > 8) currChunk.iLOuterFloorHeight = std::stoi(lineAy[8]);
          if (lineAy.size() > 9) currChunk.iROuterFloorHeight = std::stoi(lineAy[9]);
          if (lineAy.size() > 10) currChunk.iRLOuterWallHeight = std::stoi(lineAy[10]);
          if (lineAy.size() > 11) currChunk.iRUOuterWallHeight = std::stoi(lineAy[11]);
          if (lineAy.size() > 12) currChunk.iRoofHeight = std::stoi(lineAy[12]);
          if (lineAy.size() > 13) currChunk.iNearForward = std::stoi(lineAy[13]);
          if (lineAy.size() > 14) currChunk.iNearForwardExStart = std::stoi(lineAy[14]);
          if (lineAy.size() > 15) currChunk.iNearForwardEx = std::stoi(lineAy[15]);
          if (lineAy.size() > 16) currChunk.iLeftSubdivDist = std::stoi(lineAy[16]);
          if (lineAy.size() > 17) currChunk.iCenterSubdivDist = std::stoi(lineAy[17]);
          if (lineAy.size() > 18) currChunk.iRightSubdivDist = std::stoi(lineAy[18]);
          if (lineAy.size() > 19) currChunk.iLWallSubdivDist = std::stoi(lineAy[19]);
          if (lineAy.size() > 20) currChunk.iRWallSubdivDist = std::stoi(lineAy[20]);
          if (lineAy.size() > 21) currChunk.iRoofSubdivDist = std::stoi(lineAy[21]);
          if (lineAy.size() > 22) currChunk.iLUOuterWallSubdivDist = std::stoi(lineAy[22]);
          if (lineAy.size() > 23) currChunk.iLLOuterWallSubdivDist = std::stoi(lineAy[23]);
          if (lineAy.size() > 24) currChunk.iOuterFloorSubdivDist = std::stoi(lineAy[24]);
          if (lineAy.size() > 25) currChunk.iRLOuterWallSubdivDist = std::stoi(lineAy[25]);
          if (lineAy.size() > 26) currChunk.iRUOuterWallSubdivDist = std::stoi(lineAy[26]);
          if (lineAy.size() > 27) currChunk.iNearBackward = std::stoi(lineAy[27]);
          if (lineAy.size() > 28) currChunk.iNearBackwardExStart = std::stoi(lineAy[28]);
          if (lineAy.size() > 29) currChunk.iNearBackwardEx = std::stoi(lineAy[29]);
          m_chunkAy.push_back(currChunk);
          iChunkLine = 0;
        }
        break;

      case eFileSection::SIGNS:
        if (lineAy.size() == SIGNS_COUNT) {
          ProcessSign(lineAy, section);
        } else {
          Logging::LogMessage("Error loading file: signs section ended before anticipated");
          bSuccess = false;
        }
        break;

      case eFileSection::STUNTS:
        if (lineAy.empty()) {
          break;
        } else if (lineAy.size() == 1 && lineAy[0] == "-1") {
          section = eFileSection::TEXTURE;
        } else if (lineAy.size() == STUNTS_COUNT) {
          const int iGeometryIndex = std::stoi(lineAy[0]);
          tStunt &stunt = m_stuntMap[iGeometryIndex];
          stunt = {};
          stunt.iChunkCount = std::stoi(lineAy[1]);
          stunt.iNumTicks = std::stoi(lineAy[2]);
          stunt.iTickStartIdx = std::stoi(lineAy[3]);
          stunt.iTimingGroup = std::stoi(lineAy[4]);
          stunt.iHeight = std::stoi(lineAy[5]);
          stunt.iTimeBulging = std::stoi(lineAy[6]);
          stunt.iTimeFlat = std::stoi(lineAy[7]);
          stunt.iRampSideLength = std::stoi(lineAy[8]);
          stunt.iFlags = std::stoi(lineAy[9]);
        } else {
          Logging::LogMessage("Error loading file: stunts section ended before anticipated");
          bSuccess = false;
        }
        break;

      case eFileSection::TEXTURE:
        if (lineAy.empty()) {
          break;
        } else if (lineAy.size() == 1) {
          if (lineAy[0] == "-1") {
            section = eFileSection::TRACK_NUM;
          } else {
            const size_t separator = lineAy[0].find(':');
            if (separator != std::string::npos) {
              const std::string type = lineAy[0].substr(0, separator);
              const std::string filename = lineAy[0].substr(separator + 1);
              if (type == "TEX")
                m_sTextureFile = filename;
              else if (type == "BLD")
                m_sBuildingFile = filename;
            }
          }
        } else if (lineAy.size() == BACKS_COUNT) {
          m_backsMap[std::stoi(lineAy[0])] = std::stoi(lineAy[1]);
        } else {
          Logging::LogMessage("Error loading file: texture section ended before anticipated");
          bSuccess = false;
        }
        break;

      case eFileSection::TRACK_NUM:
        if (lineAy.size() == 1 && IsNumber(lineAy[0])) {
          m_raceInfo.iTrackNumber = std::stoi(lineAy[0]);
          section = eFileSection::LAPS;
        }
        break;

      case eFileSection::LAPS:
        if (lineAy.size() == LAPS_COUNT) {
          m_raceInfo.iImpossibleLaps = std::stoi(lineAy[0]);
          m_raceInfo.iHardLaps = std::stoi(lineAy[1]);
          m_raceInfo.iTrickyLaps = std::stoi(lineAy[2]);
          m_raceInfo.iMediumLaps = std::stoi(lineAy[3]);
          m_raceInfo.iEasyLaps = std::stoi(lineAy[4]);
          m_raceInfo.iGirlieLaps = std::stoi(lineAy[5]);
          section = eFileSection::MAP;
        }
        break;

      case eFileSection::MAP:
        if (lineAy.size() == MAP_COUNT) {
          m_raceInfo.dTrackMapSize = std::stod(lineAy[0]);
          m_raceInfo.iTrackMapFidelity = std::stoi(lineAy[1]);
          m_raceInfo.dPreviewSize = std::stod(lineAy[2]);
        }
        section = eFileSection::END;
        break;

      case eFileSection::END:
        break;
    }
  }

  return bSuccess;
}

//-------------------------------------------------------------------------------------------------

unsigned int CTrackModel::GetSignedBitValueFromInt(int iValue)
{
  const bool bNegative = iValue < 0;
  unsigned int uiRetVal = static_cast<unsigned int>(std::abs(iValue));
  if (bNegative)
    uiRetVal |= 0x80000000;
  return uiRetVal;
}

//-------------------------------------------------------------------------------------------------

int CTrackModel::GetIntValueFromSignedBit(unsigned int uiValue)
{
  const bool bNegative = (uiValue & 0x80000000) != 0;
  uiValue &= ~0x80000000;
  int iRetVal = static_cast<int>(uiValue);
  if (bNegative)
    iRetVal *= -1;
  return iRetVal;
}

//-------------------------------------------------------------------------------------------------

void CTrackModel::ProcessSign(const std::vector<std::string> &lineAy, eFileSection &section)
{
  const int iVal0 = std::stoi(lineAy[0]);
  const int iVal1 = std::stoi(lineAy[1]);
  if (iVal0 == -1 || iVal1 == -1) {
    section = eFileSection::STUNTS;
    return;
  }

  int iSignable = 0;
  size_t iChunk = 0;
  while (iChunk < m_chunkAy.size()) {
    if (m_chunkAy[iChunk].iSignType >= 0 && m_chunkAy[iChunk].iSignType < 256) {
      if (iSignable == iVal0)
        break;
      ++iSignable;
    }
    ++iChunk;
  }
  if (iChunk < m_chunkAy.size())
    m_chunkAy[iChunk].iSignTexture = iVal1;
}

//-------------------------------------------------------------------------------------------------

void CTrackModel::GetTrackData(std::vector<std::uint8_t> &data)
{
  char szBuf[1024];
  std::snprintf(szBuf, sizeof(szBuf), " %4d %6d %6d %6d\r\n\r\n\r\n",
                static_cast<int>(m_chunkAy.size()), m_header.iHeaderUnk1,
                m_header.iHeaderUnk2, m_header.iFloorDepth);
  WriteToVector(data, szBuf);

  CSignMap signMap;
  int iSignIndex = 0;
  for (tGeometryChunk &chunk : m_chunkAy) {
    chunk.dYaw = ConstrainAngle(chunk.dYaw);
    chunk.dPitch = ConstrainAngle(chunk.dPitch);
    chunk.dRoll = ConstrainAngle(chunk.dRoll);
    chunk.dSignYaw = ConstrainAngle(chunk.dSignYaw);
    chunk.dSignPitch = ConstrainAngle(chunk.dSignPitch);
    chunk.dSignRoll = ConstrainAngle(chunk.dSignRoll);

    char szGenerate[1024];
    GenerateChunkString(chunk, szGenerate, sizeof(szGenerate));
    WriteToVector(data, szGenerate);
    WriteToVector(data, "\r\n");
    if (chunk.iSignType >= 0 && chunk.iSignType < 256) {
      if (SignTypeCanHaveTexture(chunk.iSignType) && chunk.iSignTexture > 0) {
        signMap[iSignIndex] = chunk.iSignTexture;
      }
      ++iSignIndex;
    }
  }

  for (const auto &sign : signMap) {
    std::snprintf(szBuf, sizeof(szBuf), " %4d %6d\r\n", sign.first, sign.second);
    WriteToVector(data, szBuf);
  }
  std::snprintf(szBuf, sizeof(szBuf), " %4d %6d\r\n", -1, -1);
  WriteToVector(data, szBuf);

  for (const auto &stunt : m_stuntMap) {
    std::snprintf(szBuf, sizeof(szBuf),
                  " %4d %6d %6d %6d %6d %6d %6d %6d %6d %6d\r\n",
                  stunt.first, stunt.second.iChunkCount, stunt.second.iNumTicks,
                  stunt.second.iTickStartIdx, stunt.second.iTimingGroup,
                  stunt.second.iHeight, stunt.second.iTimeBulging,
                  stunt.second.iTimeFlat, stunt.second.iRampSideLength,
                  stunt.second.iFlags);
    WriteToVector(data, szBuf);
  }
  WriteToVector(data, "\r\n");
  std::snprintf(szBuf, sizeof(szBuf), " %4d\r\n\r\n", -1);
  WriteToVector(data, szBuf);

  WriteToVector(data, "TEX:");
  WriteToVector(data, m_sTextureFile.c_str());
  WriteToVector(data, "\r\nBLD:");
  WriteToVector(data, m_sBuildingFile.c_str());
  WriteToVector(data, "\r\nBACKS:\r\n");
  for (const auto &back : m_backsMap) {
    std::snprintf(szBuf, sizeof(szBuf), "%d %d\r\n", back.first, back.second);
    WriteToVector(data, szBuf);
  }
  std::snprintf(szBuf, sizeof(szBuf), " %4d\r\n\r\n", -1);
  WriteToVector(data, szBuf);

  if (!(m_raceInfo.iTrackNumber == 0
        && m_raceInfo.iImpossibleLaps == 0
        && m_raceInfo.iHardLaps == 0
        && m_raceInfo.iTrickyLaps == 0
        && m_raceInfo.iMediumLaps == 0
        && m_raceInfo.iEasyLaps == 0
        && m_raceInfo.iGirlieLaps == 0
        && m_raceInfo.dTrackMapSize == 0
        && m_raceInfo.iTrackMapFidelity == 0
        && m_raceInfo.dPreviewSize == 0)) {
    std::snprintf(szBuf, sizeof(szBuf), "%d\r\n", m_raceInfo.iTrackNumber);
    WriteToVector(data, szBuf);
    std::snprintf(szBuf, sizeof(szBuf), "%4d %4d %4d %4d %4d %4d\r\n",
                  m_raceInfo.iImpossibleLaps, m_raceInfo.iHardLaps,
                  m_raceInfo.iTrickyLaps, m_raceInfo.iMediumLaps,
                  m_raceInfo.iEasyLaps, m_raceInfo.iGirlieLaps);
    WriteToVector(data, szBuf);
    std::snprintf(szBuf, sizeof(szBuf), "%.2lf %4d %.2lf\r\n\r\n",
                  m_raceInfo.dTrackMapSize, m_raceInfo.iTrackMapFidelity,
                  m_raceInfo.dPreviewSize);
    WriteToVector(data, szBuf);
  }
}

//-------------------------------------------------------------------------------------------------

void CTrackModel::WriteToVector(std::vector<std::uint8_t> &data, const char *szText) const
{
  const size_t length = std::strlen(szText);
  data.insert(data.end(), szText, szText + length);
}

//-------------------------------------------------------------------------------------------------

void CTrackModel::GenerateChunkString(const tGeometryChunk &chunk, char *szBuf, int iSize) const
{
  std::snprintf(szBuf, iSize,
           "%5d %6d %6d %6d %6d %6d %6d %11.5lf %11.5lf %11.5lf %5d %5d %5d %5d %3d %3d %3d %4d %5d %3d %3d %3d\r\n"
           "%6d %6d %6d %6d %6d %6d %6d %6d %6d %6d %6d %6d %4d %6d %6d %6.1lf %6.1lf %6.1lf\r\n"
           "%5d %6d %6d %6d %6d %6d %6d %6d %6d %6d %6d %6d %6d"
           " %3d %3d %3d %d %d %d %d %d %d %d %d %d %d %d %3d %3d %3d\r\n",
           chunk.iLeftShoulderWidth, chunk.iLeftLaneWidth, chunk.iRightLaneWidth,
           chunk.iRightShoulderWidth, chunk.iLeftShoulderHeight,
           chunk.iRightShoulderHeight, chunk.iLength, chunk.dYaw, chunk.dPitch,
           chunk.dRoll, chunk.iAILine1, chunk.iAILine2, chunk.iAILine3,
           chunk.iAILine4, chunk.iTrackGrip, chunk.iLeftShoulderGrip,
           chunk.iRightShoulderGrip, chunk.iAIMaxSpeed, chunk.iGroundHeight,
           chunk.iAudioAboveTrigger, chunk.iAudioTriggerSpeed,
           chunk.iAudioBelowTrigger, chunk.iLeftSurfaceType,
           chunk.iCenterSurfaceType, chunk.iRightSurfaceType,
           chunk.iLeftWallType, chunk.iRightWallType, chunk.iRoofType,
           chunk.iLUOuterWallType, chunk.iLLOuterWallType,
           chunk.iOuterFloorType, chunk.iRLOuterWallType,
           chunk.iRUOuterWallType, chunk.iEnvironmentFloorType,
           chunk.iSignType, chunk.iSignHorizOffset, chunk.iSignVertOffset,
           chunk.dSignYaw, chunk.dSignPitch, chunk.dSignRoll,
           chunk.iLUOuterWallHOffset, chunk.iLLOuterWallHOffset,
           chunk.iLOuterFloorHOffset, chunk.iROuterFloorHOffset,
           chunk.iRLOuterWallHOffset, chunk.iRUOuterWallHOffset,
           chunk.iLUOuterWallHeight, chunk.iLLOuterWallHeight,
           chunk.iLOuterFloorHeight, chunk.iROuterFloorHeight,
           chunk.iRLOuterWallHeight, chunk.iRUOuterWallHeight,
           chunk.iRoofHeight, chunk.iNearForward, chunk.iNearForwardExStart,
           chunk.iNearForwardEx, chunk.iLeftSubdivDist,
           chunk.iCenterSubdivDist, chunk.iRightSubdivDist,
           chunk.iLWallSubdivDist, chunk.iRWallSubdivDist,
           chunk.iRoofSubdivDist, chunk.iLUOuterWallSubdivDist,
           chunk.iLLOuterWallSubdivDist, chunk.iOuterFloorSubdivDist,
           chunk.iRLOuterWallSubdivDist, chunk.iRUOuterWallSubdivDist,
           chunk.iNearBackward, chunk.iNearBackwardExStart,
           chunk.iNearBackwardEx);
}

//-------------------------------------------------------------------------------------------------

CTrackHistory::CTrackHistory()
  : m_iHistoryIndex(-1)
{
}

//-------------------------------------------------------------------------------------------------

void CTrackHistory::Clear()
{
  m_historyAy.clear();
  m_iHistoryIndex = -1;
}

//-------------------------------------------------------------------------------------------------

void CTrackHistory::Save(CTrackModel &track, const std::string &sDescription, size_t maxEntries)
{
  if (maxEntries == 0) {
    Clear();
    return;
  }

  if (m_iHistoryIndex + 1 < static_cast<int>(m_historyAy.size()))
    m_historyAy.erase(m_historyAy.begin() + m_iHistoryIndex + 1, m_historyAy.end());

  tTrackHistory history;
  history.sDescription = sDescription;
  track.GetTrackData(history.byteAy);
  m_historyAy.push_back(std::move(history));

  if (m_historyAy.size() > maxEntries) {
    const size_t removeCount = m_historyAy.size() - maxEntries;
    m_historyAy.erase(m_historyAy.begin(), m_historyAy.begin() + removeCount);
  }
  m_iHistoryIndex = static_cast<int>(m_historyAy.size()) - 1;
}

//-------------------------------------------------------------------------------------------------

bool CTrackHistory::Undo(CTrackModel &track)
{
  if (m_historyAy.empty())
    return false;
  m_iHistoryIndex = std::max(0, m_iHistoryIndex - 1);
  return Restore(track);
}

//-------------------------------------------------------------------------------------------------

bool CTrackHistory::Redo(CTrackModel &track)
{
  if (m_historyAy.empty())
    return false;
  m_iHistoryIndex = std::min(static_cast<int>(m_historyAy.size()) - 1,
                             m_iHistoryIndex + 1);
  return Restore(track);
}

//-------------------------------------------------------------------------------------------------

const tTrackHistory *CTrackHistory::GetCurrentEntry() const
{
  if (m_iHistoryIndex < 0 || m_iHistoryIndex >= static_cast<int>(m_historyAy.size()))
    return nullptr;
  return &m_historyAy[static_cast<size_t>(m_iHistoryIndex)];
}

//-------------------------------------------------------------------------------------------------

bool CTrackHistory::Restore(CTrackModel &track)
{
  const tTrackHistory *pHistory = GetCurrentEntry();
  if (!pHistory)
    return false;
  track.ClearData();
  return track.ProcessTrackData(pHistory->byteAy.data(), pHistory->byteAy.size());
}
//-------------------------------------------------------------------------------------------------
