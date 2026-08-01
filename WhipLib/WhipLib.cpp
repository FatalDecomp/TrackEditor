#include "WhipLib.h"
#include "Palette.h"
#include "Texture.h"
#include "ShapeData.h"
#include "ShapeFactory.h"
#include "Track.h"
#include <vector>
#include <map>
//-------------------------------------------------------------------------------------------------
#if defined(_DEBUG) && defined(IS_WINDOWS)
#define new new(_CLIENT_BLOCK, __FILE__, __LINE__)
#endif
//-------------------------------------------------------------------------------------------------

struct tTrackData
{
  tTrackData()
    : pTrackShape(NULL)
  {};
  ~tTrackData()
  {
    track.ClearData();
    if (pTrackShape) {
      delete pTrackShape;
      pTrackShape = NULL;
    }
    for (std::vector<CShapeData *>::iterator it = signAy.begin(); it != signAy.end(); ++it) {
      delete *it;
    }
    signAy.clear();
  }

  CTrack track;
  CShapeData *pTrackShape;
  std::vector<CShapeData *> signAy;
};

//-------------------------------------------------------------------------------------------------

typedef std::map<int, tTrackData *> CTrackMap;
static CTrackMap s_trackMap;
static int s_iNextTrackId = 0;

//-------------------------------------------------------------------------------------------------

bool StuffModel(CShapeData *pShape,
                tWhiplashVertex *pVertexBuf,
                int iVertexBufSize,
                uint32 *pIndexBuf,
                int iIndexBufSize,
                int &iNumVertices,
                int &iNumIndices)
{
  bool bSuccess = false;
  if (pShape->m_uiNumVerts <= (uint32)(iVertexBufSize / sizeof(tWhiplashVertex))
      && pShape->m_uiNumIndices <= (uint32)iIndexBufSize / sizeof(uint32)) {
    bSuccess = true;
    iNumVertices = (int)pShape->m_uiNumVerts;
    iNumIndices = (int)pShape->m_uiNumIndices;
    for (int i = 0; i < (int)pShape->m_uiNumVerts; ++i) {
      pVertexBuf[i].fX = pShape->m_vertices[i].position.x;
      pVertexBuf[i].fY = pShape->m_vertices[i].position.y;
      pVertexBuf[i].fZ = pShape->m_vertices[i].position.z;
      pVertexBuf[i].fNormalX = pShape->m_vertices[i].normal.x;
      pVertexBuf[i].fNormalY = pShape->m_vertices[i].normal.y;
      pVertexBuf[i].fNormalZ = pShape->m_vertices[i].normal.z;
      pVertexBuf[i].fTexX = pShape->m_vertices[i].texCoords.x;
      pVertexBuf[i].fTexY = pShape->m_vertices[i].texCoords.y;
    }
    for (int i = 0; i < (int)pShape->m_uiNumIndices; ++i) {
      pIndexBuf[i] = pShape->m_indices[i];
    }
  }
  return bSuccess;
}

//-------------------------------------------------------------------------------------------------

WLFUNC void wlSetLoggingCallback(Logging::WhipLibLoggingCallback pfnLogMsg)
{
  Logging::SetWhipLibLoggingCallback(pfnLogMsg);
}

//-------------------------------------------------------------------------------------------------

WLFUNC int wlLoadTexture(uint8 *pDataBuf, int iBufSize, const char *szTex, const char *szPal)
{
  int iRet = -1;
  if (!pDataBuf || !szTex || !szPal)
    return iRet;

  //load palette
  CPalette pal;
  if (!pal.LoadPalette(szPal))
    return iRet;

  //load texture
  CTexture tex;
  if (!tex.LoadTexture(szTex, &pal))
    return iRet;

  //generate bmp
  int iBmpSize;
  uint8 *pBmpData = tex.GenerateBitmapData(iBmpSize);

  //fill buffer
  if (iBmpSize <= iBufSize) {
    iRet = iBmpSize;
    for (int i = 0; i < iBmpSize; ++i) {
      pDataBuf[i] = pBmpData[i];
    }
  }

  //cleanup
  delete[] pBmpData;

  return iRet;
}

//-------------------------------------------------------------------------------------------------

WLFUNC bool wlGetModel(uint8 *pBmpBuf,
                       int iBmpBufSize,
                       tWhiplashVertex *pVertexBuf,
                       int iVertexBufSize,
                       uint32 *pIndexBuf,
                       int iIndexBufSize,
                       eWhipModel model,
                       const char *szTex,
                       const char *szPal,
                       int &iBmpSize,
                       int &iNumVertices,
                       int &iNumIndices)
{
  bool bSuccess = false;
  if (!pBmpBuf || !pVertexBuf || !pIndexBuf || !szTex || !szPal)
    return bSuccess;

  //load palette
  CPalette pal;
  if (!pal.LoadPalette(szPal))
    return bSuccess;

  //load texture
  CTexture tex;
  if (!tex.LoadTexture(szTex, &pal))
    return bSuccess;

  //generate bmp
  uint8 *pBmpData = tex.GenerateBitmapData(iBmpSize);

  //generate model
  CShapeData *pShape = NULL;
  CShapeFactory::GetShapeFactory().MakeModel(&pShape, &tex, model);
  if (!pShape)
    return bSuccess;

  //fill buffers
  bSuccess = StuffModel(pShape, pVertexBuf, iVertexBufSize, pIndexBuf, iIndexBufSize, iNumVertices, iNumIndices);
  if (iBmpSize <= iBmpBufSize) {
    bSuccess = true;
    for (int i = 0; i < iBmpSize; ++i) {
      pBmpBuf[i] = pBmpData[i];
    }
  }

  //cleanup
  delete pShape;
  delete[] pBmpData;

  return bSuccess;
}

//-------------------------------------------------------------------------------------------------

WLFUNC int wlLoadTrack(const char *szTrack)
{
  tTrackData *pNewTrack = new tTrackData();

  bool bSuccess = pNewTrack->track.LoadTrack(szTrack);
  bSuccess |= pNewTrack->track.LoadTextures();

  if (bSuccess) {
    pNewTrack->pTrackShape = NULL;
    CShapeFactory::GetShapeFactory().MakeTrackSurface(&pNewTrack->pTrackShape, &pNewTrack->track, eShapeSection::EXPORT, true, false);
    CShapeFactory::GetShapeFactory().MakeSigns(&pNewTrack->track, pNewTrack->signAy);
    for (int i = 0; i < (int)pNewTrack->signAy.size(); ++i) {
      pNewTrack->signAy[i]->TransformVertsForExport();
    }

    s_trackMap[++s_iNextTrackId] = pNewTrack;
    return s_iNextTrackId;
  } else {
    delete pNewTrack;
    return -1;
  }
}

//-------------------------------------------------------------------------------------------------

WLFUNC bool wlUnloadTrack(int iTrackId)
{
  CTrackMap::iterator it = s_trackMap.find(iTrackId);
  if (it == s_trackMap.end())
    return false;

  delete it->second;
  s_trackMap.erase(it);
  return true;
}

//-------------------------------------------------------------------------------------------------

WLFUNC void wlShutdownModule()
{
  CTrackMap::iterator it = s_trackMap.begin();
  for (; it != s_trackMap.end(); ++it) {
    delete it->second;
  }
  s_trackMap.clear();
}

//-------------------------------------------------------------------------------------------------

WLFUNC int wlGetTrackTex(int iTrackId, uint8 *pDataBuf, int iBufSize)
{
  int iBmpSize = -1;
  CTrackMap::iterator it = s_trackMap.find(iTrackId);
  if (it == s_trackMap.end())
    return iBmpSize;

  if (!it->second->track.m_pTex)
    return iBmpSize;

  //generate bmp
  uint8 *pBmpData = it->second->track.m_pTex->GenerateBitmapData(iBmpSize);

  //fill buffer
  if (iBmpSize <= iBufSize) {
    for (int i = 0; i < iBmpSize; ++i) {
      pDataBuf[i] = pBmpData[i];
    }
  }

  //cleanup
  delete[] pBmpData;
  return iBmpSize;
}

//-------------------------------------------------------------------------------------------------

WLFUNC int wlGetTrackBld(int iTrackId, uint8 *pDataBuf, int iBufSize)
{
  int iBmpSize = -1;
  CTrackMap::iterator it = s_trackMap.find(iTrackId);
  if (it == s_trackMap.end())
    return iBmpSize;

  if (!it->second->track.m_pBld)
    return iBmpSize;

  //generate bmp
  uint8 *pBmpData = it->second->track.m_pBld->GenerateBitmapData(iBmpSize);

  //fill buffer
  if (iBmpSize <= iBufSize) {
    for (int i = 0; i < iBmpSize; ++i) {
      pDataBuf[i] = pBmpData[i];
    }
  }

  //cleanup
  delete[] pBmpData;
  return iBmpSize;
}

//-------------------------------------------------------------------------------------------------

WLFUNC int wlGetNumSigns(int iTrackId)
{
  CTrackMap::iterator it = s_trackMap.find(iTrackId);
  if (it == s_trackMap.end())
    return -1;

  return (int)it->second->signAy.size();
}

//-------------------------------------------------------------------------------------------------

WLFUNC bool wlGetTrackModel(int iTrackId,
                            tWhiplashVertex *pVertexBuf,
                            int iVertexBufSize,
                            uint32 *pIndexBuf,
                            int iIndexBufSize,
                            int &iNumVertices,
                            int &iNumIndices)
{
  bool bSuccess = false;
  CTrackMap::iterator it = s_trackMap.find(iTrackId);
  if (it == s_trackMap.end())
    return bSuccess;
  if (!it->second->pTrackShape)
    return bSuccess;

  //fill buffers
  bSuccess = StuffModel(it->second->pTrackShape, pVertexBuf, iVertexBufSize, pIndexBuf, iIndexBufSize, iNumVertices, iNumIndices);

  return bSuccess;
}

//-------------------------------------------------------------------------------------------------

WLFUNC bool wlGetSignModel(int iTrackId,
                           int iSignIndex,
                           tWhiplashVertex *pVertexBuf,
                           int iVertexBufSize,
                           uint32 *pIndexBuf,
                           int iIndexBufSize,
                           int &iNumVertices,
                           int &iNumIndices)
{
  bool bSuccess = false;
  CTrackMap::iterator it = s_trackMap.find(iTrackId);
  if (it == s_trackMap.end())
    return bSuccess;

  if (iSignIndex < 0 || iSignIndex > it->second->signAy.size() - 1)
    return bSuccess;

  //fill buffers
  bSuccess = StuffModel(it->second->signAy[iSignIndex], pVertexBuf, iVertexBufSize, pIndexBuf, iIndexBufSize, iNumVertices, iNumIndices);

  return bSuccess;
}

//-------------------------------------------------------------------------------------------------
