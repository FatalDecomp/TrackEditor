#include "ShapeFactory.h"
#include "FBXExporter.h"
#include "ObjExporter.h"
#include "ShapeData.h"
#include "Texture.h"
#include "Palette.h"
#include "Logging.h"
#include "Track.h"
#include "CarHelpers.h"
#include <string>
#include <filesystem>
//-------------------------------------------------------------------------------------------------

static void LogMessageCbStatic(const char *szMsg, int iLen)
{
  (void)(iLen);
  printf(szMsg);
  printf("\n");
}

//-------------------------------------------------------------------------------------------------

bool ExportCar(eWhipModel carModel, std::string sWhipDir, std::string sOutDir, bool bObj)
{
  std::string sCarName = CarHelpers::GetCarNameFromModel(carModel);
  std::string sTex = CarHelpers::GetCarTextureFromModel(carModel);

  //load palette
  CPalette palette;
  if (!palette.LoadPalette(sWhipDir + "\\" + "PALETTE.PAL"))
    return false;

  //load texture
  CTexture carTex;
  if (!carTex.LoadTexture(sWhipDir + "\\" + sTex, &palette))
    return false;

  //make texture file
  std::string sTexFile = sOutDir + "\\" + sCarName + ".png";
  carTex.ExportToPngFile(sTexFile);
  printf("Exporting ");
  printf(sTexFile.c_str());
  printf("...\n");

  //create shape data
  CShapeData *pCar = NULL;
  CShapeFactory::GetShapeFactory().MakeModel(&pCar, NULL, &carTex, carModel);
  if (!pCar)
    return false;
  pCar->FlipTexCoordsForExport();

  std::string sExtension = ".fbx";
  if (bObj)
    sExtension = ".obj";
  std::string sFilename = sOutDir + std::string("\\") + sCarName + sExtension;
  printf("Exporting ");
  printf(sFilename.c_str());
  printf("...\n");
  bool bSuccess = false;
  if (bObj) {
    //export material
    std::vector<std::string> texAy;
    texAy.push_back(sCarName);
    std::string sMtlFile = sOutDir + std::string("\\") + sCarName + std::string(".mtl");
    CObjExporter::GetObjExporter().ExportMaterial(sMtlFile, texAy);

    bSuccess = CObjExporter::GetObjExporter().ExportShape(pCar, sFilename, sCarName, sMtlFile, sCarName);
  } else {
    bSuccess = CFBXExporter::GetFBXExporter().ExportShape(pCar, sCarName.c_str(), sFilename.c_str(), sTexFile.c_str());
  }

  delete pCar;
  return bSuccess;
}

//-------------------------------------------------------------------------------------------------

bool ExportTrack(CTrack *pTrack, std::string sOutDir, bool bObj)
{
  if (!pTrack || !pTrack->m_pTex || !pTrack->m_pBld)
    return false;

  //get track name
  std::string sTrackName = pTrack->m_sTrackFile;
  size_t pos = pTrack->m_sTrackFile.find_last_of('\\');
  if (pos == std::string::npos) {
    pos = pTrack->m_sTrackFile.find_last_of('/');
  }
  sTrackName = pTrack->m_sTrackFile.substr(pos + 1, pTrack->m_sTrackFile.size() - pos);
  pos = sTrackName.find_last_of('.');
  sTrackName = sTrackName.substr(0, pos);

  //make texture files
  std::string sTexFile = sOutDir + "\\" + sTrackName + ".png";
  printf("Exporting ");
  printf(sTexFile.c_str());
  printf("...\n");
  pTrack->m_pTex->ExportToPngFile(sTexFile);
  std::string sSignTexFile = sOutDir + "\\" + sTrackName + "_BLD.png";
  printf("Exporting ");
  printf(sSignTexFile.c_str());
  printf("...\n");
  pTrack->m_pBld->ExportToPngFile(sSignTexFile);

  //setup
  pTrack->GenerateTrackMath();

  //generate models
  std::vector<std::pair<std::string, CShapeData *>> trackSectionAy;
  std::vector<CShapeData *> signAy;

  CShapeData *pCenterLine = NULL;
  CShapeData *pAILine1 = NULL;
  CShapeData *pAILine2 = NULL;
  CShapeData *pAILine3 = NULL;
  CShapeData *pAILine4 = NULL;
  CShapeData *pCenterSurf = NULL;
  CShapeData *pLShoulderSurf = NULL;
  CShapeData *pRShoulderSurf = NULL;
  CShapeData *pLWallSurf = NULL;
  CShapeData *pRWallSurf = NULL;
  CShapeData *pRoofSurf = NULL;
  CShapeData *pOWallFloorSurf = NULL;
  CShapeData *pLLOWallSurf = NULL;
  CShapeData *pRLOWallSurf = NULL;
  CShapeData *pLUOWallSurf = NULL;
  CShapeData *pRUOWallSurf = NULL;

  CShapeFactory::GetShapeFactory().MakeAILine(&pCenterLine, NULL, pTrack, eShapeSection::CENTERLINE, true);
  CShapeFactory::GetShapeFactory().MakeAILine(&pAILine1, NULL, pTrack, eShapeSection::CARLINE1, true);
  CShapeFactory::GetShapeFactory().MakeAILine(&pAILine2, NULL, pTrack, eShapeSection::CARLINE2, true);
  CShapeFactory::GetShapeFactory().MakeAILine(&pAILine3, NULL, pTrack, eShapeSection::CARLINE3, true);
  CShapeFactory::GetShapeFactory().MakeAILine(&pAILine4, NULL, pTrack, eShapeSection::CARLINE4, true);
  CShapeFactory::GetShapeFactory().MakeTrackSurface(&pCenterSurf, NULL, pTrack, eShapeSection::CENTER, true);
  CShapeFactory::GetShapeFactory().MakeTrackSurface(&pLShoulderSurf, NULL, pTrack, eShapeSection::LSHOULDER, true);
  CShapeFactory::GetShapeFactory().MakeTrackSurface(&pRShoulderSurf, NULL, pTrack, eShapeSection::RSHOULDER, true);
  CShapeFactory::GetShapeFactory().MakeTrackSurface(&pLWallSurf, NULL, pTrack, eShapeSection::LWALL, true);
  CShapeFactory::GetShapeFactory().MakeTrackSurface(&pRWallSurf, NULL, pTrack, eShapeSection::RWALL, true);
  CShapeFactory::GetShapeFactory().MakeTrackSurface(&pRoofSurf, NULL, pTrack, eShapeSection::ROOF, true);
  CShapeFactory::GetShapeFactory().MakeTrackSurface(&pOWallFloorSurf, NULL, pTrack, eShapeSection::OWALLFLOOR, true);
  CShapeFactory::GetShapeFactory().MakeTrackSurface(&pLLOWallSurf, NULL, pTrack, eShapeSection::LLOWALL, true);
  CShapeFactory::GetShapeFactory().MakeTrackSurface(&pRLOWallSurf, NULL, pTrack, eShapeSection::RLOWALL, true);
  CShapeFactory::GetShapeFactory().MakeTrackSurface(&pLUOWallSurf, NULL, pTrack, eShapeSection::LUOWALL, true);
  CShapeFactory::GetShapeFactory().MakeTrackSurface(&pRUOWallSurf, NULL, pTrack, eShapeSection::RUOWALL, true);

  trackSectionAy.push_back(std::make_pair("Centerline", pCenterLine));
  trackSectionAy.push_back(std::make_pair("AI Line 1", pAILine1));
  trackSectionAy.push_back(std::make_pair("AI Line 2", pAILine2));
  trackSectionAy.push_back(std::make_pair("AI Line 3", pAILine3));
  trackSectionAy.push_back(std::make_pair("AI Line 4", pAILine4));
  trackSectionAy.push_back(std::make_pair("Center", pCenterSurf));
  trackSectionAy.push_back(std::make_pair("Left Shoulder", pLShoulderSurf));
  trackSectionAy.push_back(std::make_pair("Right Shoulder", pRShoulderSurf));
  trackSectionAy.push_back(std::make_pair("Left Wall", pLWallSurf));
  trackSectionAy.push_back(std::make_pair("Right Wall", pRWallSurf));
  trackSectionAy.push_back(std::make_pair("Roof", pRoofSurf));
  trackSectionAy.push_back(std::make_pair("Outer Wall Floor", pOWallFloorSurf));
  trackSectionAy.push_back(std::make_pair("Left Lower Outer Wall", pLLOWallSurf));
  trackSectionAy.push_back(std::make_pair("Right Lower Outer Wall", pRLOWallSurf));
  trackSectionAy.push_back(std::make_pair("Left Upper Outer Wall", pLUOWallSurf));
  trackSectionAy.push_back(std::make_pair("Right Upper Outer Wall", pRUOWallSurf));

  CShapeFactory::GetShapeFactory().MakeSigns(NULL, pTrack, signAy);

  for (std::vector<std::pair<std::string, CShapeData *>>::iterator it = trackSectionAy.begin(); it != trackSectionAy.end(); ++it)
    it->second->FlipTexCoordsForExport();
  //signs need to be moved to the right position on track, this is normally done in the shader
  for (std::vector<CShapeData *>::iterator it = signAy.begin(); it != signAy.end(); ++it) {
    (*it)->TransformVertsForExport();
    (*it)->FlipTexCoordsForExport();
  }

  //export
  std::string sExtension = ".fbx";
  if (bObj)
    sExtension = ".obj";
  std::string sFilename = sOutDir + std::string("\\") + sTrackName + sExtension;
  printf("Exporting ");
  printf(sFilename.c_str());
  printf("...\n");
  bool bExported = false;
  if (bObj) {
    bExported = CObjExporter::GetObjExporter().ExportTrack(trackSectionAy,
                                                           signAy,
                                                           sOutDir,
                                                           sTrackName,
                                                           sFilename);
  } else {
    bExported = CFBXExporter::GetFBXExporter().ExportTrack(trackSectionAy,
                                                           signAy,
                                                           sTrackName.c_str(),
                                                           sFilename.c_str(),
                                                           sTexFile.c_str(),
                                                           sSignTexFile.c_str());
  }

  //cleanup
  for (std::vector<std::pair<std::string, CShapeData *>>::iterator it = trackSectionAy.begin(); it != trackSectionAy.end(); ++it)
    delete it->second;
  for (std::vector<CShapeData *>::iterator it = signAy.begin(); it != signAy.end(); ++it)
    delete *it;
  return bExported;
}

//-------------------------------------------------------------------------------------------------

int main(int argc, char *argv[])
{
  //not enough arguments
  if (argc < 4) {
    printf("Converts all whiplash car models and tracks to OBJ or FBX format\n");
    printf("Usage: ModelExporter whiplash_dir output_dir type\n");
    printf("       ex: ModelExporter C:\\WHIP\\WHIPLASH\\FATDATA C:\\WHIP\\output FBX\n");
    return -1;
  }

  //setup
  Logging::SetWhipLibLoggingCallback(LogMessageCbStatic);
  std::string sWhipDir = argv[1];
  std::string sOutDir = argv[2];
  std::string sType = argv[3];
  bool bObj = false;
  if (sType.compare("fbx") == 0 || sType.compare("FBX") == 0) {
    bObj = false;
  } else if (sType.compare("obj") == 0 || sType.compare("OBJ") == 0) {
    bObj = true;
  } else {
    printf("type must be OBJ or FBX\n");
    return -1;
  }
  CShapeFactory::GetShapeFactory().m_bOglRunning = false;

  //export cars
  ExportCar(eWhipModel::CAR_F1WACK, sWhipDir, sOutDir, bObj);
  ExportCar(eWhipModel::CAR_XAUTO, sWhipDir, sOutDir, bObj);
  ExportCar(eWhipModel::CAR_XDESILVA, sWhipDir, sOutDir, bObj);
  ExportCar(eWhipModel::CAR_XPULSE, sWhipDir, sOutDir, bObj);
  ExportCar(eWhipModel::CAR_XGLOBAL, sWhipDir, sOutDir, bObj);
  ExportCar(eWhipModel::CAR_XMILLION, sWhipDir, sOutDir, bObj);
  ExportCar(eWhipModel::CAR_XMISSION, sWhipDir, sOutDir, bObj);
  ExportCar(eWhipModel::CAR_XZIZIN, sWhipDir, sOutDir, bObj);
  ExportCar(eWhipModel::CAR_XREISE, sWhipDir, sOutDir, bObj);
  ExportCar(eWhipModel::CAR_YAUTO, sWhipDir, sOutDir, bObj);
  ExportCar(eWhipModel::CAR_YDESILVA, sWhipDir, sOutDir, bObj);
  ExportCar(eWhipModel::CAR_YPULSE, sWhipDir, sOutDir, bObj);
  ExportCar(eWhipModel::CAR_YGLOBAL, sWhipDir, sOutDir, bObj);
  ExportCar(eWhipModel::CAR_YMILLION, sWhipDir, sOutDir, bObj);
  ExportCar(eWhipModel::CAR_YMISSION, sWhipDir, sOutDir, bObj);
  ExportCar(eWhipModel::CAR_YZIZIN, sWhipDir, sOutDir, bObj);
  ExportCar(eWhipModel::CAR_YREISE, sWhipDir, sOutDir, bObj);
  ExportCar(eWhipModel::CAR_DEATH, sWhipDir, sOutDir, bObj);

  //load all tracks in dir
  std::vector<CTrack *> trackAy;
  for (const auto &entry : std::filesystem::directory_iterator(sWhipDir)) {
    std::string sEntry = entry.path().string();
    size_t pos = sEntry.find_last_of('.');
    if (pos != std::string::npos) {
      std::string sExtension = sEntry.substr(pos, sEntry.size() - pos);
      if (sExtension.compare(".TRK") == 0) {
        CTrack *pNewTrack = new CTrack();
        printf("Loading %s ", sEntry.c_str());
        if (pNewTrack->LoadTrack(sEntry) && pNewTrack->LoadTextures()) {
          printf("success\n");
          trackAy.push_back(pNewTrack);
        } else {
          printf("failure\n");
          delete pNewTrack;
        }
      }
    }
  }
  //export tracks
  for (std::vector<CTrack *>::iterator it = trackAy.begin(); it != trackAy.end(); ++it) {
    CTrack *pTrack = *it;
    ExportTrack(pTrack, sOutDir, bObj);
    delete pTrack;
  }

  //exit
  return 0;
};

//-------------------------------------------------------------------------------------------------