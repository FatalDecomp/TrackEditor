#include "Track.h"
#include "Unmangler.h"
#include <assert.h>
#include <fstream>
#include <sstream>
#include <string.h>
#include "Texture.h"
#include "Palette.h"
#include "glm.hpp"
#include "gtc/matrix_transform.hpp"
#include "gtx/transform.hpp"
#include "gtx/quaternion.hpp"
#include "Logging.h"
#include "SignType.h"
#include "MathHelpers.h"
#if defined(IS_WINDOWS)
  #include "Windows.h"
#endif
//-------------------------------------------------------------------------------------------------
#if defined(_DEBUG) && defined(IS_WINDOWS)
#define new new(_CLIENT_BLOCK, __FILE__, __LINE__)
#endif
//-------------------------------------------------------------------------------------------------
#define HEADER_ELEMENT_COUNT 4
#define SIGNS_COUNT 2
#define STUNTS_COUNT 10
#define BACKS_COUNT 2
#define LAPS_COUNT 6
#define MAP_COUNT 3
//-------------------------------------------------------------------------------------------------

void tGeometryChunk::Clear()
{
  //line 1
  iLeftShoulderWidth = 0;
  iLeftLaneWidth = 0;
  iRightLaneWidth = 0;
  iRightShoulderWidth = 0;
  iLeftShoulderHeight = 0;
  iRightShoulderHeight = 0;
  iLength = 0;
  dYaw = 0.0;
  dPitch = 0.0;
  dRoll = 0.0;
  iAILine1 = 0;
  iAILine2 = 0;
  iAILine3 = 0;
  iAILine4 = 0;
  iTrackGrip = 0;
  iLeftShoulderGrip = 0;
  iRightShoulderGrip = 0;
  iAIMaxSpeed = 0;
  iGroundHeight = 0;
  iAudioAboveTrigger = 0;
  iAudioTriggerSpeed = 0;
  iAudioBelowTrigger = 0;
  //line 2
  iLeftSurfaceType = 0;
  iCenterSurfaceType = 0;
  iRightSurfaceType = 0;
  iLeftWallType = 0;
  iRightWallType = 0;
  iRoofType = 0;
  iLUOuterWallType = 0;
  iLLOuterWallType = 0;
  iOuterFloorType = 0;
  iRLOuterWallType = 0;
  iRUOuterWallType = 0;
  iEnvironmentFloorType = 0;
  iSignType = 0;
  iSignHorizOffset = 0;
  iSignVertOffset = 0;
  dSignYaw = 0.0;
  dSignPitch = 0.0;
  dSignRoll = 0.0;
  //line 3
  iLUOuterWallHOffset = 0;
  iLLOuterWallHOffset = 0;
  iLOuterFloorHOffset = 0;
  iROuterFloorHOffset = 0;
  iRLOuterWallHOffset = 0;
  iRUOuterWallHOffset = 0;
  iLUOuterWallHeight = 0;
  iLLOuterWallHeight = 0;
  iLOuterFloorHeight = 0;
  iROuterFloorHeight = 0;
  iRLOuterWallHeight = 0;
  iRUOuterWallHeight = 0;
  iRoofHeight = 0;
  iNearForward = 0;
  iNearForwardExStart = 0;
  iNearForwardEx = 0;
  iLeftSubdivDist = 0;
  iCenterSubdivDist = 0;
  iRightSubdivDist = 0;
  iLWallSubdivDist = 0;
  iRWallSubdivDist = 0;
  iRoofSubdivDist = 0;
  iLUOuterWallSubdivDist = 0;
  iLLOuterWallSubdivDist = 0;
  iOuterFloorSubdivDist = 0;
  iRLOuterWallSubdivDist = 0;
  iRUOuterWallSubdivDist = 0;
  iNearBackward = 0;
  iNearBackwardExStart = 0;
  iNearBackwardEx = 0;

  //additional data
  iSignTexture = 0;
}

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
  memset(&m_header, 0, sizeof(m_header));
  m_header.iFloorDepth = 2048;
  m_chunkAy.clear();
  m_sTrackFile = "";
  m_sTextureFile = "";
  m_sBuildingFile = "";
  m_sLastLoadedTex = "";
  m_sLastLoadedBld = "";
  m_sLastLoadedPal = "";
  memset(&m_raceInfo, 0, sizeof(m_raceInfo));
}

//-------------------------------------------------------------------------------------------------

bool CTrack::LoadTrack(const std::string &sFilename)
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

  //open file
  std::ifstream file(sFilename.c_str(), std::ios::binary);
  if (!file.is_open()) {
    Logging::LogMessage("Failed to open track: %s", sFilename.c_str());
    return false;
  }

  file.seekg(0, file.end);
  size_t length = file.tellg();
  file.seekg(0, file.beg);
  if (length <= 0) {
    Logging::LogMessage("Track file %s is empty", sFilename.c_str());
    return false;
  }

  //read file
  char *szBuf = new char[length];
  memset(szBuf, 0, length);
  file.read(szBuf, length);

  bool bSuccess = false;
  int iUnmangledLength = Unmangler::GetUnmangledLength((uint8 *)szBuf, (int)length);
  //unmangle
  if (iUnmangledLength > 0 && iUnmangledLength < MAX_MANGLED_LENGTH) {
    Logging::LogMessage("Track file %s is mangled", sFilename.c_str());
    uint8 *szUnmangledData = new uint8[iUnmangledLength];
    bSuccess = Unmangler::UnmangleFile((uint8 *)szBuf, (int)length, szUnmangledData, iUnmangledLength);
    Logging::LogMessage("%s track file %s", bSuccess ? "Unmangled" : "Failed to unmangle", sFilename.c_str());

    if (bSuccess)
      bSuccess = ProcessTrackData(szUnmangledData, (size_t)iUnmangledLength);

    delete[] szUnmangledData;
  } else {
    bSuccess = ProcessTrackData((uint8 *)szBuf, length);
  }

  //cleanup
  delete[] szBuf;
  file.close();

  Logging::LogMessage("%s track file: %s", bSuccess ? "Loaded" : "Failed to load", sFilename.c_str());

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

bool CTrack::IsNumber(const std::string &str)
{
  char *ptr;
  strtol(str.c_str(), &ptr, 10);
  return *ptr == '\0';
}

//-------------------------------------------------------------------------------------------------

bool CTrack::ProcessTrackData(const uint8 *pData, size_t length)
{
  bool bSuccess = true;
  int iChunkLine = 0;
  struct tGeometryChunk currChunk;
  eFileSection section = eFileSection::HEADER;

  std::stringstream ssFile((char*)pData);
  while (ssFile.good()) {
    std::string sLine;
    getline(ssFile, sLine, '\n');

    //get value array
    std::vector<std::string> lineAy;
    std::stringstream ssLine(sLine);
    while (ssLine.good()) {
      std::string sSubStr;
      getline(ssLine, sSubStr, ' ');

      //strip \r off
      size_t pos = sSubStr.find('\r');
      if (pos != std::string::npos)
        sSubStr = sSubStr.substr(0, pos);

      if (!sSubStr.empty() && sSubStr.find("ýýýý") == std::string::npos)
        lineAy.push_back(sSubStr);
    }

    switch (section) {
      case eFileSection::HEADER:
        //must be first thing in file
        if (lineAy.size() == HEADER_ELEMENT_COUNT) {
          //found header
          m_header.iNumChunks  = std::stoi(lineAy[0]);
          m_header.iHeaderUnk1 = std::stoi(lineAy[1]);
          m_header.iHeaderUnk2 = std::stoi(lineAy[2]);
          m_header.iFloorDepth = std::stoi(lineAy[3]);
          section = eFileSection::GEOMETRY;
        }
        break;
      case eFileSection::GEOMETRY:
        if (iChunkLine == 0) {
          if (lineAy.empty()) {
            //do nothing
          } else if (lineAy.size() == SIGNS_COUNT) {
            if (m_chunkAy.size() != m_header.iNumChunks) {
              Logging::LogMessage("Warning loading file: number of chunks loaded (%d) does not match header (%d)", (int)m_chunkAy.size(), m_header.iNumChunks);
            }
            // reached next section
            // there is no defined end to geometry chunks
            // more chunks than the count at the top of the file is allowed
            // so we must detect the beginning of the next section
            section = eFileSection::SIGNS;
            ProcessSign(lineAy, section);
          } else {
            //start new chunk
            currChunk.Clear();
            currChunk.iSignTexture = -1;
            //process line 1
            if (lineAy.size() > 0) currChunk.iLeftShoulderWidth    = std::stoi(lineAy[0]);
            if (lineAy.size() > 1) currChunk.iLeftLaneWidth        = std::stoi(lineAy[1]);
            if (lineAy.size() > 2) currChunk.iRightLaneWidth       = std::stoi(lineAy[2]);
            if (lineAy.size() > 3) currChunk.iRightShoulderWidth   = std::stoi(lineAy[3]);
            if (lineAy.size() > 4) currChunk.iLeftShoulderHeight   = std::stoi(lineAy[4]);
            if (lineAy.size() > 5) currChunk.iRightShoulderHeight  = std::stoi(lineAy[5]);
            if (lineAy.size() > 6) currChunk.iLength               = std::stoi(lineAy[6]);
            if (lineAy.size() > 7) currChunk.dYaw                  = MathHelpers::ConstrainAngle(std::stod(lineAy[7]));
            if (lineAy.size() > 8) currChunk.dPitch                = MathHelpers::ConstrainAngle(std::stod(lineAy[8]));
            if (lineAy.size() > 9) currChunk.dRoll                 = MathHelpers::ConstrainAngle(std::stod(lineAy[9]));
            if (lineAy.size() > 10) currChunk.iAILine1              = std::stoi(lineAy[10]);
            if (lineAy.size() > 11) currChunk.iAILine2              = std::stoi(lineAy[11]);
            if (lineAy.size() > 12) currChunk.iAILine3              = std::stoi(lineAy[12]);
            if (lineAy.size() > 13) currChunk.iAILine4              = std::stoi(lineAy[13]);
            if (lineAy.size() > 14) currChunk.iTrackGrip            = std::stoi(lineAy[14]);
            if (lineAy.size() > 15) currChunk.iLeftShoulderGrip     = std::stoi(lineAy[15]);
            if (lineAy.size() > 16) currChunk.iRightShoulderGrip    = std::stoi(lineAy[16]);
            if (lineAy.size() > 17) currChunk.iAIMaxSpeed           = std::stoi(lineAy[17]);
            if (lineAy.size() > 18) currChunk.iGroundHeight         = std::stoi(lineAy[18]);
            if (lineAy.size() > 19) currChunk.iAudioAboveTrigger    = std::stoi(lineAy[19]);
            if (lineAy.size() > 20) currChunk.iAudioTriggerSpeed    = std::stoi(lineAy[20]);
            if (lineAy.size() > 21) currChunk.iAudioBelowTrigger    = std::stoi(lineAy[21]);
            //inc chunk index
            ++iChunkLine;
          }
          //TODO: what about invalid number of chunks?
        } else if (iChunkLine == 1) {
          //process line 2
          if (lineAy.size() > 0) currChunk.iLeftSurfaceType      = std::stoi(lineAy[0]);
          if (lineAy.size() > 1) currChunk.iCenterSurfaceType    = std::stoi(lineAy[1]);
          if (lineAy.size() > 2) currChunk.iRightSurfaceType     = std::stoi(lineAy[2]);
          if (lineAy.size() > 3) currChunk.iLeftWallType         = std::stoi(lineAy[3]);
          if (lineAy.size() > 4) currChunk.iRightWallType        = std::stoi(lineAy[4]);
          if (lineAy.size() > 5) currChunk.iRoofType             = std::stoi(lineAy[5]);
          if (lineAy.size() > 6) currChunk.iLUOuterWallType      = std::stoi(lineAy[6]);
          if (lineAy.size() > 7) currChunk.iLLOuterWallType      = std::stoi(lineAy[7]);
          if (lineAy.size() > 8) currChunk.iOuterFloorType       = std::stoi(lineAy[8]);
          if (lineAy.size() > 9) currChunk.iRLOuterWallType      = std::stoi(lineAy[9]);
          if (lineAy.size() > 10) currChunk.iRUOuterWallType      = std::stoi(lineAy[10]);
          if (lineAy.size() > 11) currChunk.iEnvironmentFloorType = std::stoi(lineAy[11]);
          if (lineAy.size() > 12) currChunk.iSignType             = std::stoi(lineAy[12]);
          if (lineAy.size() > 13) currChunk.iSignHorizOffset      = std::stoi(lineAy[13]);
          if (lineAy.size() > 14) currChunk.iSignVertOffset       = std::stoi(lineAy[14]);
          if (lineAy.size() > 15) currChunk.dSignYaw              = MathHelpers::ConstrainAngle(std::stod(lineAy[15]));
          if (lineAy.size() > 16) currChunk.dSignPitch            = MathHelpers::ConstrainAngle(std::stod(lineAy[16]));
          if (lineAy.size() > 17) currChunk.dSignRoll             = MathHelpers::ConstrainAngle(std::stod(lineAy[17]));
          //inc chunk index
          ++iChunkLine;
        } else if (iChunkLine == 2) {
          //process line 3
          if (lineAy.size() > 0) currChunk.iLUOuterWallHOffset = std::stoi(lineAy[0]);
          if (lineAy.size() > 1) currChunk.iLLOuterWallHOffset = std::stoi(lineAy[1]);
          if (lineAy.size() > 2) currChunk.iLOuterFloorHOffset = std::stoi(lineAy[2]);
          if (lineAy.size() > 3) currChunk.iROuterFloorHOffset = std::stoi(lineAy[3]);
          if (lineAy.size() > 4) currChunk.iRLOuterWallHOffset = std::stoi(lineAy[4]);
          if (lineAy.size() > 5) currChunk.iRUOuterWallHOffset = std::stoi(lineAy[5]);
          if (lineAy.size() > 6) currChunk.iLUOuterWallHeight  = std::stoi(lineAy[6]);
          if (lineAy.size() > 7) currChunk.iLLOuterWallHeight  = std::stoi(lineAy[7]);
          if (lineAy.size() > 8) currChunk.iLOuterFloorHeight  = std::stoi(lineAy[8]);
          if (lineAy.size() > 9) currChunk.iROuterFloorHeight  = std::stoi(lineAy[9]);
          if (lineAy.size() > 10) currChunk.iRLOuterWallHeight  = std::stoi(lineAy[10]);
          if (lineAy.size() > 11) currChunk.iRUOuterWallHeight  = std::stoi(lineAy[11]);
          if (lineAy.size() > 12) currChunk.iRoofHeight         = std::stoi(lineAy[12]);
          if (lineAy.size() > 13) currChunk.iNearForward        = std::stoi(lineAy[13]);
          if (lineAy.size() > 14) currChunk.iNearForwardExStart         = std::stoi(lineAy[14]);
          if (lineAy.size() > 15) currChunk.iNearForwardEx         = std::stoi(lineAy[15]);
          if (lineAy.size() > 16) currChunk.iLeftSubdivDist              = std::stoi(lineAy[16]);
          if (lineAy.size() > 17) currChunk.iCenterSubdivDist              = std::stoi(lineAy[17]);
          if (lineAy.size() > 18) currChunk.iRightSubdivDist              = std::stoi(lineAy[18]);
          if (lineAy.size() > 19) currChunk.iLWallSubdivDist              = std::stoi(lineAy[19]);
          if (lineAy.size() > 20) currChunk.iRWallSubdivDist              = std::stoi(lineAy[20]);
          if (lineAy.size() > 21) currChunk.iRoofSubdivDist              = std::stoi(lineAy[21]);
          if (lineAy.size() > 22) currChunk.iLUOuterWallSubdivDist              = std::stoi(lineAy[22]);
          if (lineAy.size() > 23) currChunk.iLLOuterWallSubdivDist              = std::stoi(lineAy[23]);
          if (lineAy.size() > 24) currChunk.iOuterFloorSubdivDist              = std::stoi(lineAy[24]);
          if (lineAy.size() > 25) currChunk.iRLOuterWallSubdivDist              = std::stoi(lineAy[25]);
          if (lineAy.size() > 26) currChunk.iRUOuterWallSubdivDist              = std::stoi(lineAy[26]);
          if (lineAy.size() > 27) currChunk.iNearBackward              = std::stoi(lineAy[27]);
          if (lineAy.size() > 28) currChunk.iNearBackwardExStart              = std::stoi(lineAy[28]);
          if (lineAy.size() > 29) currChunk.iNearBackwardEx              = std::stoi(lineAy[29]);
          //chunk is complete, add to array and reset index
          m_chunkAy.push_back(currChunk);
          iChunkLine = 0;
        }
        break;
      case eFileSection::SIGNS:
        if (lineAy.size() == SIGNS_COUNT) {
          ProcessSign(lineAy, section);
        } else {
          assert(0);
          Logging::LogMessage("Error loading file: signs section ended before anticipated");
          bSuccess = false;
        }
        break;
      case eFileSection::STUNTS:
        if (lineAy.size() == 0) {
            //do nothing
        } else if (lineAy.size() == 1) {
          if (lineAy[0].compare("-1") == 0) {
            //stunts section always ends with a single -1 value
            section = eFileSection::TEXTURE;
          }
        } else if (lineAy.size() == STUNTS_COUNT) {
          //process stunt
          int iGeometryIndex                    = std::stoi(lineAy[0]);
          tStunt *pStunt = &m_stuntMap[iGeometryIndex];
          memset(pStunt, 0, sizeof(*pStunt));

          pStunt->iChunkCount     = std::stoi(lineAy[1]);
          pStunt->iNumTicks       = std::stoi(lineAy[2]);
          pStunt->iTickStartIdx   = std::stoi(lineAy[3]);
          pStunt->iTimingGroup    = std::stoi(lineAy[4]);
          pStunt->iHeight         = std::stoi(lineAy[5]);
          pStunt->iTimeBulging    = std::stoi(lineAy[6]);
          pStunt->iTimeFlat       = std::stoi(lineAy[7]);
          pStunt->iRampSideLength = std::stoi(lineAy[8]);
          pStunt->iFlags          = std::stoi(lineAy[9]);
        } else {
          assert(0);
          Logging::LogMessage("Error loading file: stunts section ended before anticipated");
          bSuccess = false;
        }
        break;
      case eFileSection::TEXTURE:
        if (lineAy.size() == 0) {
            //do nothing
        } else if (lineAy.size() == 1) {
          if (lineAy[0].compare("-1") == 0) {
            //texture section always ends with a single -1 value
            section = eFileSection::TRACK_NUM;
          } else {
            //texture and building files
            size_t pos = lineAy[0].find(':');
            if (pos != std::string::npos) {
              std::string sTextureType = lineAy[0].substr(0, pos);
              std::string sTextureFile = lineAy[0].substr(pos + 1, lineAy[0].size() - 1);
              if (sTextureType.compare("TEX") == 0) {
                m_sTextureFile = sTextureFile;
              } else if (sTextureType.compare("BLD") == 0) {
                m_sBuildingFile = sTextureFile;
              }
            }
          }
        } else if (lineAy.size() == BACKS_COUNT) {
          //process backs
          int iTexIndex = std::stoi(lineAy[0]);
          m_backsMap[iTexIndex] = std::stoi(lineAy[1]);
        } else {
          assert(0);
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
          m_raceInfo.iImpossibleLaps  = std::stoi(lineAy[0]);
          m_raceInfo.iHardLaps        = std::stoi(lineAy[1]);
          m_raceInfo.iTrickyLaps      = std::stoi(lineAy[2]);
          m_raceInfo.iMediumLaps      = std::stoi(lineAy[3]);
          m_raceInfo.iEasyLaps        = std::stoi(lineAy[4]);
          m_raceInfo.iGirlieLaps      = std::stoi(lineAy[5]);
          section = eFileSection::MAP;
        }
        break;
      case eFileSection::MAP:
        if (lineAy.size() == MAP_COUNT) {
          m_raceInfo.dTrackMapSize      = std::stod(lineAy[0]);
          m_raceInfo.iTrackMapFidelity  = std::stoi(lineAy[1]);
          m_raceInfo.dPreviewSize       = std::stod(lineAy[2]);
        }
        section = eFileSection::END;
        break;
      case eFileSection::END:
        break;
    }
  }

  GenerateTrackMath();

  return bSuccess;
}

//-------------------------------------------------------------------------------------------------

unsigned int CTrack::GetSignedBitValueFromInt(int iValue)
{
  bool bNegative = iValue < 0;
  unsigned int uiRetVal = (unsigned int)abs(iValue);
  if (bNegative)
    uiRetVal |= 0x80000000;
  return uiRetVal;
}

//-------------------------------------------------------------------------------------------------

int CTrack::GetIntValueFromSignedBit(unsigned int uiValue)
{
  bool bNegative = uiValue & 0x80000000;
  uiValue &= ~0x80000000;
  int iRetVal = (int)uiValue;
  if (bNegative)
    iRetVal = iRetVal * -1;
  return iRetVal;
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

void CTrack::ProcessSign(const std::vector<std::string> &lineAy, eFileSection &section)
{
  //helper function because this process must be done in two places
  int iVal0 = std::stoi(lineAy[0]);
  int iVal1 = std::stoi(lineAy[1]);
  if (iVal0 == -1 || iVal1 == -1) {
    //sign section always ends in two -1 values
    section = eFileSection::STUNTS;
  } else {
    //process sign
    int iSignable = 0;
    int iChunk = 0;
    //extraneous signs will be lost
    while (iChunk < m_chunkAy.size()) {
      if (m_chunkAy[iChunk].iSignType >= 0 && m_chunkAy[iChunk].iSignType < 256) {
        if (iSignable == iVal0)
          break;
        iSignable++;
      }
      iChunk++;
    }
    if (iChunk < m_chunkAy.size()) {
      m_chunkAy[iChunk].iSignTexture = std::stoi(lineAy[1]);
    }
  }
}

//-------------------------------------------------------------------------------------------------

void CTrack::GetTrackData(std::vector<uint8> &data)
{
  //write header
  char szBuf[1024];
  snprintf(szBuf, sizeof(szBuf), " %4d %6d %6d %6d\r\n\r\n\r\n", (int)m_chunkAy.size(), m_header.iHeaderUnk1, m_header.iHeaderUnk2, m_header.iFloorDepth);
  WriteToVector(data, szBuf);

  //write chunks
  CSignMap signMap;
  int iSignIndex = 0;
  for (int i = 0; i < m_chunkAy.size(); ++i) {
    //fix angles
    m_chunkAy[i].dYaw = MathHelpers::ConstrainAngle(m_chunkAy[i].dYaw);
    m_chunkAy[i].dPitch = MathHelpers::ConstrainAngle(m_chunkAy[i].dPitch);
    m_chunkAy[i].dRoll = MathHelpers::ConstrainAngle(m_chunkAy[i].dRoll);
    m_chunkAy[i].dSignYaw = MathHelpers::ConstrainAngle(m_chunkAy[i].dSignYaw);
    m_chunkAy[i].dSignPitch = MathHelpers::ConstrainAngle(m_chunkAy[i].dSignPitch);
    m_chunkAy[i].dSignRoll = MathHelpers::ConstrainAngle(m_chunkAy[i].dSignRoll);

    //write chunk data
    char szGenerate[1024];
    GenerateChunkString(m_chunkAy[i], szGenerate, sizeof(szGenerate));
    WriteToVector(data, szGenerate);
    WriteToVector(data, "\r\n");
    if (m_chunkAy[i].iSignType >= 0 && m_chunkAy[i].iSignType < 256) { //signable chunks
      if (m_chunkAy[i].iSignType < g_signAyCount 
          && g_signAy[m_chunkAy[i].iSignType].bCanHaveTexture  //these signable chunks don't have textures
          && m_chunkAy[i].iSignTexture > 0) {
        signMap[iSignIndex] = m_chunkAy[i].iSignTexture;
      }
      iSignIndex++;
    }
  }

  //write signs
  for (CSignMap::iterator it = signMap.begin(); it != signMap.end(); ++it) {
    memset(szBuf, 0, sizeof(szBuf));
    snprintf(szBuf, sizeof(szBuf), " %4d %6d\r\n", it->first, (int)it->second);
    WriteToVector(data, szBuf);
  }
  memset(szBuf, 0, sizeof(szBuf));
  snprintf(szBuf, sizeof(szBuf), " %4d %6d\r\n", -1, -1);
  WriteToVector(data, szBuf);

  //write stunts
  for (CStuntMap::iterator it = m_stuntMap.begin(); it != m_stuntMap.end(); ++it) {
    memset(szBuf, 0, sizeof(szBuf));
    snprintf(szBuf, sizeof(szBuf), " %4d %6d %6d %6d %6d %6d %6d %6d %6d %6d\r\n",
             it->first, it->second.iChunkCount, it->second.iNumTicks, it->second.iTickStartIdx,
             it->second.iTimingGroup, it->second.iHeight, it->second.iTimeBulging,
             it->second.iTimeFlat, it->second.iRampSideLength, it->second.iFlags);
    WriteToVector(data, szBuf);
  }
  WriteToVector(data, "\r\n");
  memset(szBuf, 0, sizeof(szBuf));
  snprintf(szBuf, sizeof(szBuf), " %4d\r\n\r\n", -1);
  WriteToVector(data, szBuf);

  //write textures
  WriteToVector(data, "TEX:");
  WriteToVector(data, m_sTextureFile.c_str());
  WriteToVector(data, "\r\n");
  WriteToVector(data, "BLD:");
  WriteToVector(data, m_sBuildingFile.c_str());
  WriteToVector(data, "\r\n");
  WriteToVector(data, "BACKS:");
  WriteToVector(data, "\r\n");
  for (CSignMap::iterator it = m_backsMap.begin(); it != m_backsMap.end(); ++it) {
    memset(szBuf, 0, sizeof(szBuf));
    snprintf(szBuf, sizeof(szBuf), "%d %d\r\n", it->first, (int)it->second);
    WriteToVector(data, szBuf);
  }
  memset(szBuf, 0, sizeof(szBuf));
  snprintf(szBuf, sizeof(szBuf), " %4d\r\n\r\n", -1);
  WriteToVector(data, szBuf);

  //write info
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

    memset(szBuf, 0, sizeof(szBuf));
    snprintf(szBuf, sizeof(szBuf), "%d\r\n", m_raceInfo.iTrackNumber);
    WriteToVector(data, szBuf);
    memset(szBuf, 0, sizeof(szBuf));
    snprintf(szBuf, sizeof(szBuf), "%4d %4d %4d %4d %4d %4d\r\n",
             m_raceInfo.iImpossibleLaps, m_raceInfo.iHardLaps, m_raceInfo.iTrickyLaps,
             m_raceInfo.iMediumLaps, m_raceInfo.iEasyLaps, m_raceInfo.iGirlieLaps);
    WriteToVector(data, szBuf);
    memset(szBuf, 0, sizeof(szBuf));
    snprintf(szBuf, sizeof(szBuf), "%.2lf %4d %.2lf\r\n\r\n",
             m_raceInfo.dTrackMapSize, m_raceInfo.iTrackMapFidelity, m_raceInfo.dPreviewSize);
    WriteToVector(data, szBuf);
  }
}

//-------------------------------------------------------------------------------------------------

void CTrack::WriteToVector(std::vector<uint8> &data, const char *szText)
{
  int iLength = (int)strlen(szText);
  for (int i = 0; i < iLength; ++i) {
    uint8 val = (uint8)szText[i];
    data.push_back(val);
  }
}

//-------------------------------------------------------------------------------------------------

void CTrack::GenerateChunkString(tGeometryChunk &chunk, char *szBuf, int iSize)
{
  snprintf(szBuf, iSize,
           "%5d %6d %6d %6d %6d %6d %6d %11.5lf %11.5lf %11.5lf %5d %5d %5d %5d %3d %3d %3d %4d %5d %3d %3d %3d\r\n" //line 1
           "%6d %6d %6d %6d %6d %6d %6d %6d %6d %6d %6d %6d %4d %6d %6d %6.1lf %6.1lf %6.1lf\r\n"   //line 2
           "%5d %6d %6d %6d %6d %6d %6d %6d %6d %6d %6d %6d %6d"                             //line 3
           " %3d %3d %3d %d %d %d %d %d %d %d %d %d %d %d %3d %3d %3d\r\n"       //line 3 continued
           , chunk.iLeftShoulderWidth
           , chunk.iLeftLaneWidth
           , chunk.iRightLaneWidth
           , chunk.iRightShoulderWidth
           , chunk.iLeftShoulderHeight
           , chunk.iRightShoulderHeight
           , chunk.iLength
           , chunk.dYaw
           , chunk.dPitch
           , chunk.dRoll
           , chunk.iAILine1
           , chunk.iAILine2
           , chunk.iAILine3
           , chunk.iAILine4
           , chunk.iTrackGrip
           , chunk.iLeftShoulderGrip
           , chunk.iRightShoulderGrip
           , chunk.iAIMaxSpeed
           , chunk.iGroundHeight
           , chunk.iAudioAboveTrigger
           , chunk.iAudioTriggerSpeed
           , chunk.iAudioBelowTrigger
           , chunk.iLeftSurfaceType
           , chunk.iCenterSurfaceType
           , chunk.iRightSurfaceType
           , chunk.iLeftWallType
           , chunk.iRightWallType
           , chunk.iRoofType
           , chunk.iLUOuterWallType
           , chunk.iLLOuterWallType
           , chunk.iOuterFloorType
           , chunk.iRLOuterWallType
           , chunk.iRUOuterWallType
           , chunk.iEnvironmentFloorType
           , chunk.iSignType
           , chunk.iSignHorizOffset
           , chunk.iSignVertOffset
           , chunk.dSignYaw
           , chunk.dSignPitch
           , chunk.dSignRoll
           , chunk.iLUOuterWallHOffset
           , chunk.iLLOuterWallHOffset
           , chunk.iLOuterFloorHOffset
           , chunk.iROuterFloorHOffset
           , chunk.iRLOuterWallHOffset
           , chunk.iRUOuterWallHOffset
           , chunk.iLUOuterWallHeight
           , chunk.iLLOuterWallHeight
           , chunk.iLOuterFloorHeight
           , chunk.iROuterFloorHeight
           , chunk.iRLOuterWallHeight
           , chunk.iRUOuterWallHeight
           , chunk.iRoofHeight
           , chunk.iNearForward
           , chunk.iNearForwardExStart
           , chunk.iNearForwardEx
           , chunk.iLeftSubdivDist
           , chunk.iCenterSubdivDist
           , chunk.iRightSubdivDist
           , chunk.iLWallSubdivDist
           , chunk.iRWallSubdivDist
           , chunk.iRoofSubdivDist
           , chunk.iLUOuterWallSubdivDist
           , chunk.iLLOuterWallSubdivDist
           , chunk.iOuterFloorSubdivDist
           , chunk.iRLOuterWallSubdivDist
           , chunk.iRUOuterWallSubdivDist
           , chunk.iNearBackward
           , chunk.iNearBackwardExStart
           , chunk.iNearBackwardEx);
}

//-------------------------------------------------------------------------------------------------

void CTrack::GenerateTrackMath()
{
  if (m_chunkAy.empty()) {
    return;
  }

  ResetStunts();

  m_chunkAy[0].math.center = glm::vec3(0, 0, 0);
  for (uint32 i = 0; i < m_chunkAy.size(); ++i) {
    glm::vec3 nextCenter;
    GetCenter(i, m_chunkAy[i].math.center,
              nextCenter,
              m_chunkAy[i].math.pitchAxis,
              m_chunkAy[i].math.nextChunkPitched,
              m_chunkAy[i].math.normal,
              m_chunkAy[i].math.yawMat,
              m_chunkAy[i].math.pitchMat,
              m_chunkAy[i].math.rollMat);
    if (i + 1 < m_chunkAy.size())
      m_chunkAy[i + 1].math.center = nextCenter;
  }
  for (uint32 i = 0; i < m_chunkAy.size(); ++i) {
    int iPrevIndex = (int)m_chunkAy.size() - 1;
    if (i > 0)
      iPrevIndex = i - 1;

    glm::mat4 rollMatNoRoll = glm::mat4(1);

    //left lane
    GetLane(i,
            m_chunkAy[i].math.center,
            m_chunkAy[i].math.pitchAxis,
            m_chunkAy[i].math.rollMat,
            m_chunkAy[i].math.lLane, true);
    //right lane
    GetLane(i,
            m_chunkAy[i].math.center,
            m_chunkAy[i].math.pitchAxis,
            m_chunkAy[i].math.rollMat,
            m_chunkAy[i].math.rLane, false);
    //left shoulder
    GetShoulder(i,
                m_chunkAy[i].math.lLane,
                m_chunkAy[i].math.pitchAxis,
                m_chunkAy[i].math.rollMat,
                m_chunkAy[i].math.nextChunkPitched,
                m_chunkAy[i].math.lShoulder, true);
    //right shoulder
    GetShoulder(i,
                m_chunkAy[i].math.rLane,
                m_chunkAy[i].math.pitchAxis,
                m_chunkAy[i].math.rollMat,
                m_chunkAy[i].math.nextChunkPitched,
                m_chunkAy[i].math.rShoulder, false);
    //left wall
    m_chunkAy[i].math.bLWallAttachToLane = CTrack::GetSignedBitValueFromInt(m_chunkAy[i].iLeftWallType) & SURFACE_FLAG_WALL_31;
    if (m_chunkAy[i].iLeftWallType == -1)
      m_chunkAy[i].math.bLWallAttachToLane = m_chunkAy[iPrevIndex].math.bLWallAttachToLane;
    m_chunkAy[i].math.lWallBottomAttach = m_chunkAy[i].math.bLWallAttachToLane ? m_chunkAy[i].math.lLane : m_chunkAy[i].math.lShoulder;
    GetWall(i,
            m_chunkAy[i].math.lWallBottomAttach,
            m_chunkAy[i].math.pitchAxis,
            m_chunkAy[i].math.rollMat,
            m_chunkAy[i].math.nextChunkPitched,
            m_chunkAy[i].math.lWall, eShapeSection::LWALL);
    //right wall
    m_chunkAy[i].math.bRWallAttachToLane = CTrack::GetSignedBitValueFromInt(m_chunkAy[i].iRightWallType) & SURFACE_FLAG_WALL_31;
    if (m_chunkAy[i].iRightWallType == -1)
      m_chunkAy[i].math.bRWallAttachToLane = m_chunkAy[iPrevIndex].math.bRWallAttachToLane;
    m_chunkAy[i].math.rWallBottomAttach = m_chunkAy[i].math.bRWallAttachToLane ? m_chunkAy[i].math.rLane : m_chunkAy[i].math.rShoulder;
    GetWall(i,
            m_chunkAy[i].math.rWallBottomAttach,
            m_chunkAy[i].math.pitchAxis,
            m_chunkAy[i].math.rollMat,
            m_chunkAy[i].math.nextChunkPitched,
            m_chunkAy[i].math.rWall, eShapeSection::RWALL);
    //outer floor
    glm::vec3 lLaneNoRoll;
    GetLane(i,
            m_chunkAy[i].math.center,
            m_chunkAy[i].math.pitchAxis,
            rollMatNoRoll, lLaneNoRoll, true);
    glm::vec3 rLaneNoRoll;
    GetLane(i,
            m_chunkAy[i].math.center,
            m_chunkAy[i].math.pitchAxis,
            rollMatNoRoll, rLaneNoRoll, false);
    GetOWallFloor(i, lLaneNoRoll, rLaneNoRoll,
                  m_chunkAy[i].math.pitchAxis,
                  m_chunkAy[i].math.nextChunkPitched,
                  m_chunkAy[i].math.lFloor,
                  m_chunkAy[i].math.rFloor);
    //outer wall roll mat
    glm::mat4 oWallRollMat = m_chunkAy[i].iOuterFloorType < 0 ? m_chunkAy[i].math.rollMat : rollMatNoRoll;
    //llowall
    m_chunkAy[i].math.lloWallBottomAttach = m_chunkAy[i].math.lFloor;
    if (m_chunkAy[i].iOuterFloorType < 0) {
      m_chunkAy[i].math.bLloWallAttachToShoulder = true;
      m_chunkAy[i].math.lloWallBottomAttach = m_chunkAy[i].math.lShoulder;
    }
    GetWall(i,
            m_chunkAy[i].math.lloWallBottomAttach,
            m_chunkAy[i].math.pitchAxis, oWallRollMat,
            m_chunkAy[i].math.nextChunkPitched,
            m_chunkAy[i].math.lloWall, eShapeSection::LLOWALL);
    //rlowall
    m_chunkAy[i].math.rloWallBottomAttach = m_chunkAy[i].math.rFloor;
    if (m_chunkAy[i].iOuterFloorType < 0) {
      m_chunkAy[i].math.bRloWallAttachToShoulder = true;
      m_chunkAy[i].math.rloWallBottomAttach = m_chunkAy[i].math.rShoulder;
    }
    GetWall(i,
            m_chunkAy[i].math.rloWallBottomAttach,
            m_chunkAy[i].math.pitchAxis, oWallRollMat,
            m_chunkAy[i].math.nextChunkPitched,
            m_chunkAy[i].math.rloWall, eShapeSection::RLOWALL);
    //luowall
    GetWall(i,
            m_chunkAy[i].math.lloWall,
            m_chunkAy[i].math.pitchAxis, oWallRollMat,
            m_chunkAy[i].math.nextChunkPitched,
            m_chunkAy[i].math.luoWall, eShapeSection::LUOWALL);
    //ruowall
    GetWall(i,
            m_chunkAy[i].math.rloWall,
            m_chunkAy[i].math.pitchAxis, oWallRollMat,
            m_chunkAy[i].math.nextChunkPitched,
            m_chunkAy[i].math.ruoWall, eShapeSection::RUOWALL);
    //ailines
    GetAILine(i,
              m_chunkAy[i].math.center,
              m_chunkAy[i].math.pitchAxis,
              m_chunkAy[i].math.rollMat,
              m_chunkAy[i].math.nextChunkPitched,
              m_chunkAy[i].math.aiLine1,
              eShapeSection::AILINE1, m_iAILineHeight);
    GetAILine(i,
              m_chunkAy[i].math.center,
              m_chunkAy[i].math.pitchAxis,
              m_chunkAy[i].math.rollMat,
              m_chunkAy[i].math.nextChunkPitched,
              m_chunkAy[i].math.aiLine2,
              eShapeSection::AILINE2, m_iAILineHeight);
    GetAILine(i,
              m_chunkAy[i].math.center,
              m_chunkAy[i].math.pitchAxis,
              m_chunkAy[i].math.rollMat,
              m_chunkAy[i].math.nextChunkPitched,
              m_chunkAy[i].math.aiLine3, eShapeSection::AILINE3, m_iAILineHeight);
    GetAILine(i,
              m_chunkAy[i].math.center,
              m_chunkAy[i].math.pitchAxis,
              m_chunkAy[i].math.rollMat,
              m_chunkAy[i].math.nextChunkPitched,
              m_chunkAy[i].math.aiLine4,
              eShapeSection::AILINE4, m_iAILineHeight);
    //car positions are ai lines with 0 height
    GetAILine(i,
              m_chunkAy[i].math.center,
              m_chunkAy[i].math.pitchAxis,
              m_chunkAy[i].math.rollMat,
              m_chunkAy[i].math.nextChunkPitched,
              m_chunkAy[i].math.carLine1,
              eShapeSection::AILINE1, 0);
    GetAILine(i,
              m_chunkAy[i].math.center,
              m_chunkAy[i].math.pitchAxis,
              m_chunkAy[i].math.rollMat,
              m_chunkAy[i].math.nextChunkPitched,
              m_chunkAy[i].math.carLine2,
              eShapeSection::AILINE2, 0);
    GetAILine(i,
              m_chunkAy[i].math.center,
              m_chunkAy[i].math.pitchAxis,
              m_chunkAy[i].math.rollMat,
              m_chunkAy[i].math.nextChunkPitched,
              m_chunkAy[i].math.carLine3, 
              eShapeSection::AILINE3, 0);
    GetAILine(i,
              m_chunkAy[i].math.center,
              m_chunkAy[i].math.pitchAxis,
              m_chunkAy[i].math.rollMat,
              m_chunkAy[i].math.nextChunkPitched,
              m_chunkAy[i].math.carLine4,
              eShapeSection::AILINE4, 0);

    m_chunkAy[i].math.centerStunt = m_chunkAy[i].math.center;
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
      glm::vec3 prevCenter = (i == iStart) ? m_chunkAy[iPrevIndex].math.center : m_chunkAy[iPrevIndex].math.centerStunt;
      glm::vec3 nextChunkBase = glm::vec3(0, 0, 1);
      glm::mat4 yawMat = m_chunkAy[i].math.yawMat;
      glm::vec3 nextChunkYawed = glm::vec3(yawMat * glm::vec4(nextChunkBase, 1.0f));
      glm::vec3 pitchAxis = glm::normalize(glm::cross(nextChunkYawed, glm::vec3(0.0f, 1.0f, 0.0f)));
      glm::mat4 pitchMat = glm::rotate(fTheta, pitchAxis);
      glm::vec3 nextChunkPitched = glm::vec3(pitchMat * glm::vec4(nextChunkYawed, 1.0f));
      glm::mat4 translateMat = glm::mat4(1);
      if (i > 0)
        translateMat = glm::translate(prevCenter);
      float fLen = (float)m_chunkAy[i].iLength * ((float)iLengthPercent / (float)STUNT_LENGTH_100_PERCENT);
      glm::mat4 scaleMat = glm::scale(glm::vec3(fLen, fLen, fLen));
      m_chunkAy[i].math.centerStunt = glm::vec3(translateMat * scaleMat * glm::vec4(nextChunkPitched, 1.0f));
      glm::vec3 lLaneStunt;
      glm::vec3 rLaneStunt;
      GetLane(i, m_chunkAy[i].math.centerStunt, pitchAxis, m_chunkAy[i].math.rollMat, lLaneStunt, true);
      GetLane(i, m_chunkAy[i].math.centerStunt, pitchAxis, m_chunkAy[i].math.rollMat, rLaneStunt, false);
      if (it->second.iFlags & STUNT_FLAG_LLANE)
        m_chunkAy[i].math.lLane = lLaneStunt;
      if (it->second.iFlags & STUNT_FLAG_RLANE)
        m_chunkAy[i].math.rLane = rLaneStunt;
      if (it->second.iFlags & STUNT_FLAG_LSHOULDER)
        GetShoulder(i, lLaneStunt,
                    m_chunkAy[i].math.pitchAxis,
                    m_chunkAy[i].math.rollMat,
                    m_chunkAy[i].math.nextChunkPitched,
                    m_chunkAy[i].math.lShoulder, true);
      if (it->second.iFlags & STUNT_FLAG_RSHOULDER)
        GetShoulder(i, rLaneStunt,
                    m_chunkAy[i].math.pitchAxis,
                    m_chunkAy[i].math.rollMat,
                    m_chunkAy[i].math.nextChunkPitched,
                    m_chunkAy[i].math.rShoulder, false);
      m_chunkAy[i].math.lWallBottomAttach = m_chunkAy[i].math.bLWallAttachToLane ? m_chunkAy[i].math.lLane : m_chunkAy[i].math.lShoulder;
      if (it->second.iFlags & STUNT_FLAG_LWALL)
        GetWall(i,
                m_chunkAy[i].math.lWallBottomAttach,
                m_chunkAy[i].math.pitchAxis,
                m_chunkAy[i].math.rollMat,
                m_chunkAy[i].math.nextChunkPitched,
                m_chunkAy[i].math.lWall, eShapeSection::LWALL);
      m_chunkAy[i].math.rWallBottomAttach = m_chunkAy[i].math.bRWallAttachToLane ? m_chunkAy[i].math.rLane : m_chunkAy[i].math.rShoulder;
      if (it->second.iFlags & STUNT_FLAG_RWALL)
        GetWall(i,
                m_chunkAy[i].math.rWallBottomAttach,
                m_chunkAy[i].math.pitchAxis,
                m_chunkAy[i].math.rollMat,
                m_chunkAy[i].math.nextChunkPitched,
                m_chunkAy[i].math.rWall, eShapeSection::RWALL);
      //ailines
      GetAILine(i,
                m_chunkAy[i].math.centerStunt,
                m_chunkAy[i].math.pitchAxis,
                m_chunkAy[i].math.rollMat,
                m_chunkAy[i].math.nextChunkPitched,
                m_chunkAy[i].math.aiLine1,
                eShapeSection::AILINE1, m_iAILineHeight);
      GetAILine(i,
                m_chunkAy[i].math.centerStunt,
                m_chunkAy[i].math.pitchAxis,
                m_chunkAy[i].math.rollMat,
                m_chunkAy[i].math.nextChunkPitched,
                m_chunkAy[i].math.aiLine2,
                eShapeSection::AILINE2, m_iAILineHeight);
      GetAILine(i,
                m_chunkAy[i].math.centerStunt,
                m_chunkAy[i].math.pitchAxis,
                m_chunkAy[i].math.rollMat,
                m_chunkAy[i].math.nextChunkPitched,
                m_chunkAy[i].math.aiLine3, eShapeSection::AILINE3, m_iAILineHeight);
      GetAILine(i,
                m_chunkAy[i].math.centerStunt,
                m_chunkAy[i].math.pitchAxis,
                m_chunkAy[i].math.rollMat,
                m_chunkAy[i].math.nextChunkPitched,
                m_chunkAy[i].math.aiLine4,
                eShapeSection::AILINE4, m_iAILineHeight);
      //car positions are ai lines with 0 height
      GetAILine(i,
                m_chunkAy[i].math.centerStunt,
                m_chunkAy[i].math.pitchAxis,
                m_chunkAy[i].math.rollMat,
                m_chunkAy[i].math.nextChunkPitched,
                m_chunkAy[i].math.carLine1,
                eShapeSection::AILINE1, 0);
      GetAILine(i,
                m_chunkAy[i].math.centerStunt,
                m_chunkAy[i].math.pitchAxis,
                m_chunkAy[i].math.rollMat,
                m_chunkAy[i].math.nextChunkPitched,
                m_chunkAy[i].math.carLine2,
                eShapeSection::AILINE2, 0);
      GetAILine(i,
                m_chunkAy[i].math.centerStunt,
                m_chunkAy[i].math.pitchAxis,
                m_chunkAy[i].math.rollMat,
                m_chunkAy[i].math.nextChunkPitched,
                m_chunkAy[i].math.carLine3,
                eShapeSection::AILINE3, 0);
      GetAILine(i,
                m_chunkAy[i].math.centerStunt,
                m_chunkAy[i].math.pitchAxis,
                m_chunkAy[i].math.rollMat,
                m_chunkAy[i].math.nextChunkPitched,
                m_chunkAy[i].math.carLine4,
                eShapeSection::AILINE4, 0);
    }
    //ramp after stunt
    for (int i = iEnd; i > it->first; --i) {
      int iPrevIndex = 0;
      if (i < (int)m_chunkAy.size() - 1)
        iPrevIndex = i + 1;
      glm::vec3 prevCenter = (i == iEnd) ? m_chunkAy[iPrevIndex].math.center : m_chunkAy[iPrevIndex].math.centerStunt;
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
      m_chunkAy[i].math.centerStunt = glm::vec3(translateMat * scaleMat * glm::vec4(nextChunkPitched, 1.0f));
      glm::vec3 lLaneStunt;
      glm::vec3 rLaneStunt;
      GetLane(i, m_chunkAy[i].math.centerStunt, pitchAxis, m_chunkAy[i].math.rollMat, lLaneStunt, false);
      GetLane(i, m_chunkAy[i].math.centerStunt, pitchAxis, m_chunkAy[i].math.rollMat, rLaneStunt, true);
      if (it->second.iFlags & STUNT_FLAG_LLANE)
        m_chunkAy[i].math.lLane = lLaneStunt;
      if (it->second.iFlags & STUNT_FLAG_RLANE)
        m_chunkAy[i].math.rLane = rLaneStunt;
      if (it->second.iFlags & STUNT_FLAG_LSHOULDER)
        GetShoulder(i, lLaneStunt,
                    m_chunkAy[i].math.pitchAxis,
                    m_chunkAy[i].math.rollMat,
                    m_chunkAy[i].math.nextChunkPitched,
                    m_chunkAy[i].math.lShoulder, true);
      if (it->second.iFlags & STUNT_FLAG_RSHOULDER)
        GetShoulder(i,
                    rLaneStunt,
                    m_chunkAy[i].math.pitchAxis,
                    m_chunkAy[i].math.rollMat,
                    m_chunkAy[i].math.nextChunkPitched,
                    m_chunkAy[i].math.rShoulder, false);
      m_chunkAy[i].math.lWallBottomAttach = m_chunkAy[i].math.bLWallAttachToLane ? m_chunkAy[i].math.lLane : m_chunkAy[i].math.lShoulder;
      if (it->second.iFlags & STUNT_FLAG_LWALL)
        GetWall(i,
                m_chunkAy[i].math.lWallBottomAttach,
                m_chunkAy[i].math.pitchAxis,
                m_chunkAy[i].math.rollMat,
                m_chunkAy[i].math.nextChunkPitched,
                m_chunkAy[i].math.lWall, eShapeSection::LWALL);
      m_chunkAy[i].math.rWallBottomAttach = m_chunkAy[i].math.bRWallAttachToLane ? m_chunkAy[i].math.rLane : m_chunkAy[i].math.rShoulder;
      if (it->second.iFlags & STUNT_FLAG_RWALL)
        GetWall(i,
                m_chunkAy[i].math.rWallBottomAttach,
                m_chunkAy[i].math.pitchAxis,
                m_chunkAy[i].math.rollMat,
                m_chunkAy[i].math.nextChunkPitched,
                m_chunkAy[i].math.rWall, eShapeSection::RWALL);
      //ailines
      GetAILine(i,
                m_chunkAy[i].math.centerStunt,
                m_chunkAy[i].math.pitchAxis,
                m_chunkAy[i].math.rollMat,
                m_chunkAy[i].math.nextChunkPitched,
                m_chunkAy[i].math.aiLine1,
                eShapeSection::AILINE1, m_iAILineHeight);
      GetAILine(i,
                m_chunkAy[i].math.centerStunt,
                m_chunkAy[i].math.pitchAxis,
                m_chunkAy[i].math.rollMat,
                m_chunkAy[i].math.nextChunkPitched,
                m_chunkAy[i].math.aiLine2,
                eShapeSection::AILINE2, m_iAILineHeight);
      GetAILine(i,
                m_chunkAy[i].math.centerStunt,
                m_chunkAy[i].math.pitchAxis,
                m_chunkAy[i].math.rollMat,
                m_chunkAy[i].math.nextChunkPitched,
                m_chunkAy[i].math.aiLine3, eShapeSection::AILINE3, m_iAILineHeight);
      GetAILine(i,
                m_chunkAy[i].math.centerStunt,
                m_chunkAy[i].math.pitchAxis,
                m_chunkAy[i].math.rollMat,
                m_chunkAy[i].math.nextChunkPitched,
                m_chunkAy[i].math.aiLine4,
                eShapeSection::AILINE4, m_iAILineHeight);
      //car positions are ai lines with 0 height
      GetAILine(i,
                m_chunkAy[i].math.centerStunt,
                m_chunkAy[i].math.pitchAxis,
                m_chunkAy[i].math.rollMat,
                m_chunkAy[i].math.nextChunkPitched,
                m_chunkAy[i].math.carLine1,
                eShapeSection::AILINE1, 0);
      GetAILine(i,
                m_chunkAy[i].math.centerStunt,
                m_chunkAy[i].math.pitchAxis,
                m_chunkAy[i].math.rollMat,
                m_chunkAy[i].math.nextChunkPitched,
                m_chunkAy[i].math.carLine2,
                eShapeSection::AILINE2, 0);
      GetAILine(i,
                m_chunkAy[i].math.centerStunt,
                m_chunkAy[i].math.pitchAxis,
                m_chunkAy[i].math.rollMat,
                m_chunkAy[i].math.nextChunkPitched,
                m_chunkAy[i].math.carLine3,
                eShapeSection::AILINE3, 0);
      GetAILine(i,
                m_chunkAy[i].math.centerStunt,
                m_chunkAy[i].math.pitchAxis,
                m_chunkAy[i].math.rollMat,
                m_chunkAy[i].math.nextChunkPitched,
                m_chunkAy[i].math.carLine4,
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
    if (MathHelpers::RayCollisionTriangle(rayOrig, rayVec, m_chunkAy[iPrevIndex].math.lLane, m_chunkAy[i].math.rLane, m_chunkAy[i].math.lLane)) {
      iClosestChunk = i;
      iPrevChunk = iPrevIndex;
      p0 = m_chunkAy[iPrevIndex].math.lLane;
      p1 = m_chunkAy[i].math.rLane;
      p2 = m_chunkAy[i].math.lLane;
      p3 = m_chunkAy[iPrevIndex].math.rLane;
    }
    if (MathHelpers::RayCollisionTriangle(rayOrig, rayVec, m_chunkAy[iPrevIndex].math.rLane, m_chunkAy[iPrevIndex].math.lLane, m_chunkAy[i].math.rLane)) {
      iClosestChunk = i;
      iPrevChunk = iPrevIndex;
      p0 = m_chunkAy[iPrevIndex].math.lLane;
      p1 = m_chunkAy[iPrevIndex].math.rLane;
      p2 = m_chunkAy[i].math.rLane;
      p3 = m_chunkAy[i].math.lLane;
    }
    
    //lshoulder
    if (MathHelpers::RayCollisionTriangle(rayOrig, rayVec, m_chunkAy[iPrevIndex].math.lShoulder, m_chunkAy[i].math.lLane, m_chunkAy[i].math.lShoulder)) {
      iClosestChunk = i;
      iPrevChunk = iPrevIndex;
      p0 = m_chunkAy[iPrevIndex].math.lShoulder;
      p1 = m_chunkAy[i].math.lLane;
      p2 = m_chunkAy[i].math.lShoulder;
      p3 = m_chunkAy[iPrevIndex].math.lLane;
    }
    if (MathHelpers::RayCollisionTriangle(rayOrig, rayVec, m_chunkAy[iPrevIndex].math.lShoulder, m_chunkAy[iPrevIndex].math.lLane, m_chunkAy[i].math.lLane)) {
      iClosestChunk = i;
      iPrevChunk = iPrevIndex;
      p0 = m_chunkAy[iPrevIndex].math.lShoulder;
      p1 = m_chunkAy[iPrevIndex].math.lLane;
      p2 = m_chunkAy[i].math.lLane;
      p3 = m_chunkAy[i].math.lShoulder;
    }

    //rShoulder
    if (MathHelpers::RayCollisionTriangle(rayOrig, rayVec, m_chunkAy[iPrevIndex].math.rLane, m_chunkAy[i].math.rShoulder, m_chunkAy[i].math.rLane)) {
      iClosestChunk = i;
      iPrevChunk = iPrevIndex;
      p0 = m_chunkAy[iPrevIndex].math.rLane;
      p1 = m_chunkAy[i].math.rShoulder;
      p2 = m_chunkAy[i].math.rLane;
      p3 = m_chunkAy[iPrevIndex].math.rShoulder;
    }
    if (MathHelpers::RayCollisionTriangle(rayOrig, rayVec, m_chunkAy[iPrevIndex].math.rLane, m_chunkAy[iPrevIndex].math.rShoulder, m_chunkAy[i].math.rShoulder)) {
      iClosestChunk = i;
      iPrevChunk = iPrevIndex;
      p0 = m_chunkAy[iPrevIndex].math.rLane;
      p1 = m_chunkAy[iPrevIndex].math.rShoulder;
      p2 = m_chunkAy[i].math.rShoulder;
      p3 = m_chunkAy[i].math.rLane;
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
