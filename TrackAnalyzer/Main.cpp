#include <fstream>
#include <string>
#include <iostream>
#include <filesystem>
#include "Track.h"
#include "Texture.h"
//-------------------------------------------------------------------------------------------------

static std::string g_surfaceAy[13] = {
  "center",
  "lshoulder",
  "rshoulder",
  "lwall",
  "rwall",
  "roof",
  "luowall",
  "llowall",
  "ofloor",
  "rlowall",
  "ruowall",
  "efloor",
  "sign"
};
static int g_surfaceAyCount = 13;

//-------------------------------------------------------------------------------------------------

struct tTrackWithValues
{
  std::string sTrack;
  std::vector<int> valuesFound;
};

//-------------------------------------------------------------------------------------------------

int GetSurface(CTrack *pTrack, int i, const std::string &sSurface)
{
  if (sSurface.compare(g_surfaceAy[0]) == 0) {
    return pTrack->m_chunkAy[i].iCenterSurfaceType;
  } else if (sSurface.compare(g_surfaceAy[1]) == 0) {
    return pTrack->m_chunkAy[i].iLeftSurfaceType;
  } else if (sSurface.compare(g_surfaceAy[2]) == 0) {
    return pTrack->m_chunkAy[i].iRightSurfaceType;
  } else if (sSurface.compare(g_surfaceAy[3]) == 0) {
    return pTrack->m_chunkAy[i].iLeftWallType;
  } else if (sSurface.compare(g_surfaceAy[4]) == 0) {
    return pTrack->m_chunkAy[i].iRightWallType;
  } else if (sSurface.compare(g_surfaceAy[5]) == 0) {
    return pTrack->m_chunkAy[i].iRoofType;
  } else if (sSurface.compare(g_surfaceAy[6]) == 0) {
    return pTrack->m_chunkAy[i].iLUOuterWallType;
  } else if (sSurface.compare(g_surfaceAy[7]) == 0) {
    return pTrack->m_chunkAy[i].iLLOuterWallType;
  } else if (sSurface.compare(g_surfaceAy[8]) == 0) {
    return pTrack->m_chunkAy[i].iOuterFloorType;
  } else if (sSurface.compare(g_surfaceAy[9]) == 0) {
    return pTrack->m_chunkAy[i].iRLOuterWallType;
  } else if (sSurface.compare(g_surfaceAy[10]) == 0) {
    return pTrack->m_chunkAy[i].iRUOuterWallType;
  } else if (sSurface.compare(g_surfaceAy[11]) == 0) {
    return pTrack->m_chunkAy[i].iEnvironmentFloorType;
  } else if (sSurface.compare(g_surfaceAy[12]) == 0) {
    return pTrack->m_chunkAy[i].iSignTexture;
  }
  return -1;
}

//-------------------------------------------------------------------------------------------------

bool SurfaceHasFlag(const int &iSurface, const std::string &sFlag)
{
  uint32 uiSurface = CTrack::GetSignedBitValueFromInt(iSurface);
  uint32 uiFlag = 0;
  int iFlagNum = std::stoi(sFlag);
  switch (iFlagNum) {
    case 8:  uiFlag = SURFACE_FLAG_APPLY_TEXTURE; break;
    case 9:  uiFlag = SURFACE_FLAG_NO_EXTRAS; break;
    case 10: uiFlag = SURFACE_FLAG_PARTIAL_TRANS; break;
    case 11: uiFlag = SURFACE_FLAG_BACK; break;
    case 12: uiFlag = SURFACE_FLAG_FLIP_HORIZ; break;
    case 13: uiFlag = SURFACE_FLAG_FLIP_BACKFACE; break;
    case 14: uiFlag = SURFACE_FLAG_CONCAVE; break;
    case 15: uiFlag = SURFACE_FLAG_PREVENT_JUMP; break;
    case 16: uiFlag = SURFACE_FLAG_TEXTURE_PAIR; break;
    case 17: uiFlag = SURFACE_FLAG_SKIP_RENDER; break;
    case 18: uiFlag = SURFACE_FLAG_FLIP_VERT; break;
    case 19: uiFlag = SURFACE_FLAG_NON_MAGNETIC; break;
    case 20: uiFlag = SURFACE_FLAG_FALL_OFF; break;
    case 21: uiFlag = SURFACE_FLAG_TRANSPARENT; break;
    case 22: uiFlag = SURFACE_FLAG_WALL_22; break;
    case 23: uiFlag = SURFACE_FLAG_AI_FAST_STRAT; break;
    case 24: uiFlag = SURFACE_FLAG_PIT_ZONE; break;
    case 25: uiFlag = SURFACE_FLAG_PIT; break;
    case 26: uiFlag = SURFACE_FLAG_PIT_BOX; break;
    case 27: uiFlag = SURFACE_FLAG_NO_SPAWN; break;
    case 28: uiFlag = SURFACE_FLAG_AI_MAX_SPEED; break;
    case 29: uiFlag = SURFACE_FLAG_ECHO; break;
    case 30: uiFlag = SURFACE_FLAG_BOUNCE; break;
    case 31: uiFlag = SURFACE_FLAG_WALL_31; break;
  }
  return uiSurface & uiFlag;
}

//-------------------------------------------------------------------------------------------------

int main(int argc, char *argv[])
{
  //not enough arguments
  if (argc < 4) {
    printf("Usage: TrackAnalyzer whiplash_dir surface_type surface_flag\n");
    printf("       ex: TrackAnalyzer C:\\WHIP\\WHIPLASH\\FATDATA lshoulder 9\n");
    printf("\n");
    printf("surfaces: ");
    for (int i = 0; i < g_surfaceAyCount; ++i) {
      printf("%s ", g_surfaceAy[i].c_str());
    }
    printf("\n");
    return -1;
  }

  std::string sPath = argv[1];
  std::string sSurface = argv[2];
  std::string sFlag = argv[3];

  //load all tracks in dir
  std::vector<CTrack *> trackAy;
  for (const auto &entry : std::filesystem::directory_iterator(sPath)) {
    std::string sEntry = entry.path().string();
    size_t pos = sEntry.find_last_of('.');
    if (pos != std::string::npos) {
      std::string sExtension = sEntry.substr(pos, sEntry.size() - pos);
      if (sExtension.compare(".TRK") == 0) {
        CTrack *pNewTrack = new CTrack();
        printf("Loading %s ", sEntry.c_str());
        if (pNewTrack->LoadTrack(sEntry)) {
          printf("success\n");
          trackAy.push_back(pNewTrack);
        } else {
          printf("failure\n");
          delete pNewTrack;
        }
      }
    }
  }

  //analyze
  std::vector<tTrackWithValues> resultAy;
  for (std::vector<CTrack *>::iterator it = trackAy.begin(); it != trackAy.end(); ++it) {
    CTrack *pTrack = *it;
    tTrackWithValues tResult;
    tResult.sTrack = pTrack->m_sTrackFile;
    for (int i = 0; i < pTrack->m_chunkAy.size(); ++i) {
      int iSurface = GetSurface(pTrack, i, sSurface);
      if (SurfaceHasFlag(iSurface, sFlag))
        tResult.valuesFound.push_back(i);
    }
    resultAy.push_back(tResult);

    //cleanup
    delete pTrack;
  }

  //print results
  for (int i = 0; i < (int)resultAy.size(); ++i) {
    if (!resultAy[i].valuesFound.empty()) {
      printf("----------------------------------------------------------------------------------\n");
      printf("Surface %s uses flag %s %d times in track %s\n",
             sSurface.c_str(), sFlag.c_str(), (int)resultAy[i].valuesFound.size(), resultAy[i].sTrack.c_str());
      printf("   chunks: ");
      for (int j = 0; j < (int)resultAy[i].valuesFound.size(); ++j) {
        printf("%d, ", resultAy[i].valuesFound[j]);
      }
      printf("\n");
    }
  }

  //exit
  return 0;
}

//-------------------------------------------------------------------------------------------------