#include "EditorRenderQueue.h"
#include "EditorRenderService.h"

#include <QCoreApplication>
#include <QEventLoop>
#include <QFile>
#include <QThread>
#include <QTimer>

#include <atomic>
#include <cassert>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#ifdef assert
#undef assert
#endif
#define assert(condition)                                                      \
  do {                                                                         \
    if (!(condition)) {                                                        \
      std::cerr << "assertion failed: " #condition << " (" << __FILE__       \
                << ':' << __LINE__ << ")\n";                                  \
      std::abort();                                                            \
    }                                                                          \
  } while (false)

namespace
{
std::string g_sError;
std::string g_sLoadedTrackPath;
std::string g_sLoadedAssetRoot;
std::string g_sLoadedFallbackAssetRoot;
std::string g_sLoadedTrackData;
float g_fCameraX = 0.0f;
uint32_t g_uiOverlaySurfaceClassMask = 0;
uint32_t g_uiOverlayWireframeClassMask = 0;
uint32_t g_uiOverlayFlags = 0;
uint32_t g_uiOverlayCount = 0;
uint32_t g_uiReferenceMeshCount = 0;
uint32_t g_uiReferenceVertexCount = 0;
uint32_t g_uiReferenceIndexCount = 0;
uint32_t g_uiReferenceFlags = 0;
uint32_t g_uiGraphicsCount = 0;
tEdGraphicsSettings g_LastGraphicsSettings = {};
float g_fReferenceFirstX = 0.0f;
float g_fReferenceScaleX = 0.0f;
uint32_t g_uiGeometryEpoch = 0;
uint32_t g_uiTrackGeneration = 0;
uint32_t g_uiSceneState = ROLLER_ED_SCENE_EMPTY;
std::atomic<uint32_t> g_uiInitCount(0);
std::atomic<uint32_t> g_uiRenderCount(0);
std::atomic<uint32_t> g_uiStuntTickCount(0);
std::atomic<uint32_t> g_uiStuntTicksAtLastRender(0);
std::atomic<uint32_t> g_uiFillCount(0);
std::atomic<uint32_t> g_uiTowerCountQueryCount(0);
std::atomic<uint32_t> g_uiTowerQueryCount(0);
uint32_t g_uiRefusedFillEpoch = 0;

// E4-S1. The stubbed extraction is one quad, which is enough to prove the
// worker copies the arrays out and hands the caller storage it owns.
const uint32_t g_uiStubVertexCount = 4;
const uint32_t g_uiStubIndexCount = 6;
const uint32_t g_uiStubPrimitiveCount = 1;
const uint32_t g_uiStubMaterialCount = 1;
Qt::HANDLE g_pWorkerThreadId = nullptr;
Qt::HANDLE g_pUiThreadId = nullptr;
std::atomic<bool> g_bAllFacadeCallsOnWorker(true);

void RecordFacadeThread()
{
  const Qt::HANDLE pCurrentThreadId = QThread::currentThreadId();
  if (pCurrentThreadId == g_pUiThreadId)
    g_bAllFacadeCallsOnWorker.store(false);
  if (!g_pWorkerThreadId)
    g_pWorkerThreadId = pCurrentThreadId;
  else if (g_pWorkerThreadId != pCurrentThreadId)
    g_bAllFacadeCallsOnWorker.store(false);
}

tEdRenderResult WaitForResult(CEditorRenderService &Service,
                              uint64_t ullExpectedRequestId)
{
  QEventLoop Loop;
  QTimer Timeout;
  Timeout.setSingleShot(true);
  tEdRenderResult Captured;
  bool bReceived = false;

  const QMetaObject::Connection Connection = QObject::connect(
      &Service, &CEditorRenderService::FrameCompleted,
      &Loop, [&](const tEdRenderResult &Result) {
        if (Result.Tag.ullRequestId != ullExpectedRequestId)
          return;
        Captured = Result;
        bReceived = true;
        Loop.quit();
      });
  QObject::connect(&Timeout, &QTimer::timeout, &Loop, &QEventLoop::quit);
  Timeout.start(5000);
  Loop.exec();
  QObject::disconnect(Connection);
  assert(bReceived);
  return Captured;
}
}

extern "C" eRollerEdResult ROLLER_ED_CALL RollerEd_Init(
    const tRollerEdInitInfo *pInfo)
{
  RecordFacadeThread();
  assert(pInfo);
  assert(std::string(pInfo->szAssetRoot) == "test-assets");
  ++g_uiInitCount;
  return ROLLER_ED_RESULT_OK;
}

extern "C" eRollerEdResult ROLLER_ED_CALL RollerEd_Shutdown(void)
{
  RecordFacadeThread();
  return ROLLER_ED_RESULT_OK;
}

extern "C" eRollerEdResult ROLLER_ED_CALL RollerEd_LoadTrackFile(
    const char *szTrackPath, const char *szDocumentAssetRoot)
{
  return RollerEd_LoadTrackFileEx(
      szTrackPath, szDocumentAssetRoot, nullptr);
}

extern "C" eRollerEdResult ROLLER_ED_CALL RollerEd_LoadTrackFileEx(
    const char *szTrackPath, const char *szDocumentAssetRoot,
    const char *szFallbackAssetRoot)
{
  RecordFacadeThread();
  g_sLoadedTrackPath = szTrackPath ? szTrackPath : "";
  g_sLoadedAssetRoot = szDocumentAssetRoot ? szDocumentAssetRoot : "";
  g_sLoadedFallbackAssetRoot = szFallbackAssetRoot ? szFallbackAssetRoot : "";
  g_sLoadedTrackData.clear();
  QFile TrackFile(QString::fromLocal8Bit(g_sLoadedTrackPath.c_str()));
  if (TrackFile.open(QIODevice::ReadOnly)) {
    const QByteArray Data = TrackFile.readAll();
    g_sLoadedTrackData.assign(Data.constData(),
                              static_cast<size_t>(Data.size()));
  }
  ++g_uiGeometryEpoch;
  if (g_sLoadedTrackPath == "fail.trk" || g_sLoadedTrackData == "FAIL") {
    g_uiSceneState = ROLLER_ED_SCENE_FAILED;
    g_sError = "copied load error";
    return ROLLER_ED_RESULT_LOAD_FAILED;
  }
  g_uiSceneState = ROLLER_ED_SCENE_READY;
  ++g_uiTrackGeneration;
  g_sError.clear();
  return ROLLER_ED_RESULT_OK;
}

extern "C" eRollerEdResult ROLLER_ED_CALL RollerEd_UnloadTrack(void)
{
  RecordFacadeThread();
  ++g_uiGeometryEpoch;
  g_uiSceneState = ROLLER_ED_SCENE_EMPTY;
  return ROLLER_ED_RESULT_OK;
}

extern "C" eRollerEdResult ROLLER_ED_CALL RollerEd_QueryGeometrySizes(
    tEdGeometrySizes *pSizesOut)
{
  RecordFacadeThread();
  assert(pSizesOut);
  pSizesOut->uiGeometryEpoch = g_uiGeometryEpoch;
  pSizesOut->uiTrackGeneration = g_uiTrackGeneration;
  pSizesOut->uiSceneState = g_uiSceneState;
  // EMPTY and FAILED publish zero counts, exactly as the facade does.
  const bool bReady = g_uiSceneState == ROLLER_ED_SCENE_READY;
  pSizesOut->uiVertexCount = bReady ? g_uiStubVertexCount : 0;
  pSizesOut->uiIndexCount = bReady ? g_uiStubIndexCount : 0;
  pSizesOut->uiPrimitiveCount = bReady ? g_uiStubPrimitiveCount : 0;
  pSizesOut->uiMaterialCount = bReady ? g_uiStubMaterialCount : 0;
  pSizesOut->uiVertexStride = sizeof(tEdVertex);
  pSizesOut->uiPrimitiveStride = sizeof(tEdPrimitive);
  pSizesOut->uiMaterialStride = sizeof(tEdMaterial);
  g_sError = "a later facade call replaced the error buffer";
  return ROLLER_ED_RESULT_OK;
}

extern "C" eRollerEdResult ROLLER_ED_CALL RollerEd_QueryTowerCount(
    uint32_t *puiCountOut)
{
  RecordFacadeThread();
  assert(puiCountOut);
  ++g_uiTowerCountQueryCount;
  if (g_uiSceneState != ROLLER_ED_SCENE_READY) {
    g_sError = "no scene";
    return ROLLER_ED_RESULT_NO_SCENE;
  }
  *puiCountOut = 2u;
  return ROLLER_ED_RESULT_OK;
}

extern "C" eRollerEdResult ROLLER_ED_CALL RollerEd_QueryTower(
    uint32_t uiTowerIndex, tEdTowerInfo *pInfoOut)
{
  RecordFacadeThread();
  assert(pInfoOut);
  assert(pInfoOut->uiStructSize == sizeof(*pInfoOut));
  assert(pInfoOut->uiVersion == ROLLER_ED_TOWER_INFO_VERSION);
  ++g_uiTowerQueryCount;
  if (g_uiSceneState != ROLLER_ED_SCENE_READY) {
    g_sError = "no scene";
    return ROLLER_ED_RESULT_NO_SCENE;
  }
  if (uiTowerIndex >= 2u) {
    g_sError = "tower index out of range";
    return ROLLER_ED_RESULT_INVALID_ARGUMENT;
  }

  const float fTowerIndex = static_cast<float>(uiTowerIndex);
  pInfoOut->uiChunkId = uiTowerIndex == 0u ? 7u : 11u;
  pInfoOut->fWorldPosition[0] = 1000.0f
      + static_cast<float>(g_uiGeometryEpoch) + 10.0f * fTowerIndex;
  pInfoOut->fWorldPosition[1] = 20.0f + fTowerIndex;
  pInfoOut->fWorldPosition[2] = 30.0f + fTowerIndex;
  pInfoOut->fAnchorPosition[0] = 40.0f + fTowerIndex;
  pInfoOut->fAnchorPosition[1] = 50.0f + fTowerIndex;
  pInfoOut->fAnchorPosition[2] = 60.0f + fTowerIndex;
  return ROLLER_ED_RESULT_OK;
}

extern "C" eRollerEdResult ROLLER_ED_CALL RollerEd_FillGeometry(
    uint32_t uiExpectedGeometryEpoch,
    tEdVertex *pVerts, uint32_t uiVertexCapacity,
    uint32_t *puiIndices, uint32_t uiIndexCapacity,
    tEdPrimitive *pPrims, uint32_t uiPrimitiveCapacity,
    tEdMaterial *pMats, uint32_t uiMaterialCapacity)
{
  RecordFacadeThread();
  if (g_uiSceneState != ROLLER_ED_SCENE_READY) {
    g_sError = "no scene";
    return ROLLER_ED_RESULT_NO_SCENE;
  }
  // Validated against the geometry epoch, never the generation, and nothing
  // is written on refusal.
  if (uiExpectedGeometryEpoch != g_uiGeometryEpoch) {
    g_uiRefusedFillEpoch = uiExpectedGeometryEpoch;
    g_sError = "stale geometry epoch";
    return ROLLER_ED_RESULT_STALE;
  }
  if (uiVertexCapacity < g_uiStubVertexCount
      || uiIndexCapacity < g_uiStubIndexCount
      || uiPrimitiveCapacity < g_uiStubPrimitiveCount
      || uiMaterialCapacity < g_uiStubMaterialCount) {
    g_sError = "buffer too small";
    return ROLLER_ED_RESULT_BUFFER_TOO_SMALL;
  }

  std::memset(pVerts, 0, g_uiStubVertexCount * sizeof(*pVerts));
  for (uint32_t i = 0; i < g_uiStubVertexCount; ++i)
    pVerts[i].fPosition[0] = static_cast<float>(100 + i);
  const uint32_t auiOrder[6] = { 0, 1, 2, 0, 2, 3 };
  for (uint32_t i = 0; i < g_uiStubIndexCount; ++i)
    puiIndices[i] = auiOrder[i];
  std::memset(pPrims, 0, g_uiStubPrimitiveCount * sizeof(*pPrims));
  pPrims[0].uiIndexCount = g_uiStubIndexCount;
  pPrims[0].uiBackMaterialId = ROLLER_ED_INVALID_MATERIAL_ID;
  pPrims[0].unSurfaceClass = ROLLER_ED_SURFACE_CLASS_CENTER;
  pPrims[0].unContentClass = ROLLER_ED_CONTENT_AUTHORED_TRACK;
  pPrims[0].byTopology = ROLLER_ED_TOPOLOGY_TRIANGLE_LIST;
  std::memset(pMats, 0, g_uiStubMaterialCount * sizeof(*pMats));
  pMats[0].uiKind = ROLLER_ED_MATERIAL_TEXTURED_TILE;
  ++g_uiFillCount;
  return ROLLER_ED_RESULT_OK;
}

extern "C" eRollerEdResult ROLLER_ED_CALL RollerEd_SetCamera(
    const tEdCameraState *pCamera)
{
  RecordFacadeThread();
  assert(pCamera);
  g_fCameraX = pCamera->fPosition[0];
  return ROLLER_ED_RESULT_OK;
}

extern "C" eRollerEdResult ROLLER_ED_CALL RollerEd_SetGraphicsSettings(
    const tEdGraphicsSettings *pSettings)
{
  RecordFacadeThread();
  assert(pSettings);
  assert(pSettings->uiStructSize == sizeof(*pSettings));
  assert(pSettings->uiVersion == ROLLER_ED_GRAPHICS_SETTINGS_VERSION);
  g_LastGraphicsSettings = *pSettings;
  ++g_uiGraphicsCount;
  return ROLLER_ED_RESULT_OK;
}

extern "C" eRollerEdResult ROLLER_ED_CALL RollerEd_SetReferenceMesh(
    const tEdReferenceMesh *pMesh)
{
  RecordFacadeThread();
  assert(pMesh);
  assert(pMesh->uiStructSize == sizeof(*pMesh));
  assert(pMesh->uiVersion == ROLLER_ED_REFERENCE_MESH_VERSION);
  g_uiReferenceVertexCount = pMesh->uiVertexCount;
  g_uiReferenceIndexCount = pMesh->uiIndexCount;
  g_uiReferenceFlags = pMesh->uiFlags;
  g_fReferenceScaleX = pMesh->fScale[0];
  g_fReferenceFirstX = pMesh->uiVertexCount > 0
      ? pMesh->pVertices[0].fPosition[0] : 0.0f;
  ++g_uiReferenceMeshCount;
  return ROLLER_ED_RESULT_OK;
}

extern "C" eRollerEdResult ROLLER_ED_CALL RollerEd_SetOverlayState(
    const tEdOverlayState *pState)
{
  RecordFacadeThread();
  assert(pState);
  assert(pState->uiStructSize == sizeof(*pState));
  assert(pState->uiVersion == ROLLER_ED_OVERLAY_STATE_VERSION);
  g_uiOverlaySurfaceClassMask = pState->uiSurfaceClassMask;
  g_uiOverlayWireframeClassMask = pState->uiWireframeClassMask;
  g_uiOverlayFlags = pState->uiFlags;
  ++g_uiOverlayCount;
  return ROLLER_ED_RESULT_OK;
}

extern "C" eRollerEdResult ROLLER_ED_CALL RollerEd_AdvanceStunts(
    uint32_t uiTicks)
{
  RecordFacadeThread();
  g_uiStuntTickCount += uiTicks;
  return ROLLER_ED_RESULT_OK;
}

extern "C" eRollerEdResult ROLLER_ED_CALL RollerEd_RenderFrame(
    uint8_t *pbyPixels, uint32_t uiBufferSize, uint32_t uiRowPitch,
    uint32_t uiWidth, uint32_t uiHeight, eRollerEdPixelFormat eFormat)
{
  RecordFacadeThread();
  assert(pbyPixels);
  assert(eFormat == ROLLER_ED_PIXEL_RGBA8);
  assert(uiBufferSize >= uiRowPitch * uiHeight);
  for (uint32_t y = 0; y < uiHeight; ++y) {
    for (uint32_t x = 0; x < uiWidth; ++x) {
      uint8_t *pPixel = pbyPixels + y * uiRowPitch + x * 4u;
      pPixel[0] = 17;
      pPixel[1] = 34;
      pPixel[2] = 51;
      pPixel[3] = 255;
    }
  }
  g_uiStuntTicksAtLastRender = g_uiStuntTickCount.load();
  ++g_uiRenderCount;
  return ROLLER_ED_RESULT_OK;
}

extern "C" const char *ROLLER_ED_CALL RollerEd_GetLastError(void)
{
  RecordFacadeThread();
  return g_sError.c_str();
}

int main(int argc, char **argv)
{
  QCoreApplication Application(argc, argv);
  g_pUiThreadId = QThread::currentThreadId();

  CEditorRenderService Service("test-assets");
  tEdGraphicsSettings Graphics = {};
  Graphics.uiStructSize = sizeof(Graphics);
  Graphics.uiVersion = ROLLER_ED_GRAPHICS_SETTINGS_VERSION;
  Graphics.eRenderer = ROLLER_ED_RENDERER_SOFTWARE;
  Graphics.eSoftwareDisplay = ROLLER_ED_SOFTWARE_DISPLAY_VGA;
  Graphics.eAntiAliasing = ROLLER_ED_ANTI_ALIASING_2X;
  Graphics.eAnisotropy = ROLLER_ED_ANISOTROPY_8X;
  Graphics.eTextureFilter = ROLLER_ED_TEXTURE_FILTER_BILINEAR;
  Graphics.uiTrilinear = 1u;
  Graphics.fDrawDistanceFraction = 0.75f;
  Graphics.fLodBias = -0.5f;
  Graphics.uiEmulateTransparentBorders = 0u;
  Service.SetGraphicsSettings(Graphics);
  CDocumentFrameState Document(CEditorRenderIds::NextDocumentId());
  Service.RegisterDocument(Document.GetDocumentId());

  QString sTrackPath("good.trk");
  QString sAssetRoot("document-assets");
  tEdCameraState Camera = {};
  Camera.uiStructSize = sizeof(Camera);
  Camera.uiVersion = ROLLER_ED_CAMERA_STATE_VERSION;
  Camera.fPosition[0] = 123.0f;
  tEdOverlayState Overlay = {};
  Overlay.uiStructSize = sizeof(Overlay);
  Overlay.uiVersion = ROLLER_ED_OVERLAY_STATE_VERSION;
  Overlay.uiFlags = ROLLER_ED_OVERLAY_SHOW_SURFACES
      | ROLLER_ED_OVERLAY_SHOW_WIREFRAME;
  Overlay.uiSurfaceClassMask =
      ROLLER_ED_OVERLAY_CLASS_BIT(ROLLER_ED_SURFACE_CLASS_CENTER);
  Overlay.uiWireframeClassMask =
      ROLLER_ED_OVERLAY_CLASS_BIT(ROLLER_ED_SURFACE_CLASS_ROOF);
  const uint64_t ullGoodRequest = Service.EnqueueLoadAndRender(
      Document.GetDocumentId(), Document.GetDocumentRevision(), sTrackPath,
      sAssetRoot, QSize(4, 3), 1.0, Camera, Overlay, "fatdata-assets");
  Document.BeginRequest(ullGoodRequest);

  // The queued command must not retain any caller-owned pointer payload.
  sTrackPath = "fail.trk";
  sAssetRoot = "mutated-assets";
  Camera.fPosition[0] = 999.0f;
  Overlay.uiSurfaceClassMask = 0xffffu;
  Overlay.uiWireframeClassMask = 0xffffu;
  Graphics.fDrawDistanceFraction = 0.1f;

  Service.Start();
  const tEdRenderResult GoodResult = WaitForResult(Service, ullGoodRequest);
  assert(g_sLoadedTrackPath == "good.trk");
  assert(g_sLoadedAssetRoot == "document-assets");
  assert(g_sLoadedFallbackAssetRoot == "fatdata-assets");
  assert(g_fCameraX == 123.0f);
  // AD-16: the overlay was deep-copied into the command, so mutating the
  // caller's copy after enqueueing cannot reach the worker.
  assert(g_uiOverlaySurfaceClassMask
         == ROLLER_ED_OVERLAY_CLASS_BIT(ROLLER_ED_SURFACE_CLASS_CENTER));
  assert(g_uiOverlayWireframeClassMask
         == ROLLER_ED_OVERLAY_CLASS_BIT(ROLLER_ED_SURFACE_CLASS_ROOF));
  assert((g_uiOverlayFlags & ROLLER_ED_OVERLAY_SHOW_SURFACES) != 0);
  assert(g_uiOverlayCount == 1);
  assert(g_uiGraphicsCount == 1);
  assert(g_LastGraphicsSettings.eRenderer == ROLLER_ED_RENDERER_SOFTWARE);
  assert(g_LastGraphicsSettings.eSoftwareDisplay
         == ROLLER_ED_SOFTWARE_DISPLAY_VGA);
  assert(g_LastGraphicsSettings.eAntiAliasing
         == ROLLER_ED_ANTI_ALIASING_2X);
  assert(g_LastGraphicsSettings.eAnisotropy == ROLLER_ED_ANISOTROPY_8X);
  assert(g_LastGraphicsSettings.eTextureFilter
         == ROLLER_ED_TEXTURE_FILTER_BILINEAR);
  assert(g_LastGraphicsSettings.uiTrilinear == 1u);
  assert(g_LastGraphicsSettings.fDrawDistanceFraction == 0.75f);
  assert(g_LastGraphicsSettings.fLodBias == -0.5f);
  assert(g_LastGraphicsSettings.uiEmulateTransparentBorders == 0u);
  // E3A-S7: no mesh was supplied, so the worker never touched the facade's.
  assert(g_uiReferenceMeshCount == 0);
  assert(GoodResult.Tag.eResult == ROLLER_ED_RESULT_OK);
  assert(GoodResult.Tag.uiActualGeometryEpoch == 1);
  assert(GoodResult.uiRenderedGeometryEpoch == 1);
  assert(GoodResult.Image.size() == QSize(4, 3));
  assert(GoodResult.bHasTowerSnapshot);
  assert(GoodResult.Towers.size() == 2u);
  assert(GoodResult.Towers[0].uiStructSize == sizeof(tEdTowerInfo));
  assert(GoodResult.Towers[0].uiVersion == ROLLER_ED_TOWER_INFO_VERSION);
  assert(GoodResult.Towers[0].uiChunkId == 7u);
  assert(GoodResult.Towers[0].fWorldPosition[0] == 1001.0f);
  assert(GoodResult.Towers[0].fAnchorPosition[2] == 60.0f);
  assert(g_uiTowerCountQueryCount.load() == 1u);
  assert(g_uiTowerQueryCount.load() == 2u);
  assert(Document.ApplyResult(GoodResult));
  assert(Document.CanExport());

  // E3A-S7. A reference mesh rides its own deep-copied payload: the caller's
  // arrays are mutated straight after enqueueing and must not reach the
  // worker, exactly as for the overlay above.
  {
    tEdReferenceMeshPayload Payload;
    Payload.Vertices.resize(3);
    Payload.Vertices[0].fPosition[0] = 7.0f;
    Payload.Indices = { 0u, 1u, 2u };
    Payload.fScale[0] = 2.0f;
    Payload.fScale[1] = 2.0f;
    Payload.fScale[2] = 2.0f;
    Payload.uiFlags = ROLLER_ED_REFERENCE_HAS_NORMALS
        | ROLLER_ED_REFERENCE_WIREFRAME;

    const uint64_t ullMeshRequest = Service.EnqueueRender(
        Document.GetDocumentId(), Document.GetDocumentRevision(),
        Document.GetInstalledGeometryEpoch(), QSize(4, 3), 1.0, Camera,
        Overlay, &Payload);
    Document.BeginRequest(ullMeshRequest);
    Payload.Vertices[0].fPosition[0] = 999.0f;
    Payload.Vertices.clear();
    Payload.Indices.clear();
    Payload.fScale[0] = 99.0f;
    const tEdRenderResult MeshResult =
        WaitForResult(Service, ullMeshRequest);
    assert(MeshResult.Tag.eResult == ROLLER_ED_RESULT_OK);
    assert(!MeshResult.bHasTowerSnapshot);
    assert(MeshResult.Towers.empty());
    assert(g_uiReferenceMeshCount == 1);
    assert(g_uiReferenceVertexCount == 3);
    assert(g_uiReferenceIndexCount == 3);
    assert(g_fReferenceFirstX == 7.0f);
    assert(g_fReferenceScaleX == 2.0f);
    assert((g_uiReferenceFlags & ROLLER_ED_REFERENCE_WIREFRAME) != 0);

    // A render with no mesh does not re-upload the previous one.
    const uint64_t ullPlainRequest = Service.EnqueueRender(
        Document.GetDocumentId(), Document.GetDocumentRevision(),
        Document.GetInstalledGeometryEpoch(), QSize(4, 3), 1.0, Camera,
        Overlay);
    Document.BeginRequest(ullPlainRequest);
    const tEdRenderResult PlainResult =
        WaitForResult(Service, ullPlainRequest);
    assert(PlainResult.Tag.eResult == ROLLER_ED_RESULT_OK);
    assert(!PlainResult.bHasTowerSnapshot);
    assert(PlainResult.Towers.empty());
    assert(g_uiReferenceMeshCount == 1);

    // Stunt ticks are copied into the command and applied on the same worker
    // immediately before the frame that presents them.
    const uint64_t ullStuntRequest = Service.EnqueueRender(
        Document.GetDocumentId(), Document.GetDocumentRevision(),
        Document.GetInstalledGeometryEpoch(), QSize(4, 3), 1.0, Camera,
        Overlay, nullptr, 3u);
    Document.BeginRequest(ullStuntRequest);
    const tEdRenderResult StuntResult =
        WaitForResult(Service, ullStuntRequest);
    assert(StuntResult.Tag.eResult == ROLLER_ED_RESULT_OK);
    assert(!StuntResult.bHasTowerSnapshot);
    assert(g_uiTowerCountQueryCount.load() == 1u);
    assert(g_uiTowerQueryCount.load() == 2u);
    assert(g_uiStuntTickCount.load() == 3u);
    assert(g_uiStuntTicksAtLastRender.load() == 3u);
  }


  std::vector<uint8_t> EditedTrackData = {'E', 'D', 'I', 'T'};
  const uint64_t ullEditedRequest = Service.EnqueueSerializedLoadAndRender(
      Document.GetDocumentId(), Document.GetDocumentRevision(), EditedTrackData,
      "original-document-assets", QSize(4, 3), 1.0, Camera, Overlay);
  Document.BeginRequest(ullEditedRequest);
  EditedTrackData.assign({'M', 'U', 'T', 'A', 'T', 'E', 'D'});
  const tEdRenderResult EditedResult = WaitForResult(Service, ullEditedRequest);
  const QString sEditedTemporaryTrack =
      QString::fromLocal8Bit(g_sLoadedTrackPath.c_str());
  assert(sEditedTemporaryTrack.endsWith(".TRK"));
  assert(g_sLoadedTrackData == "EDIT");
  assert(g_sLoadedAssetRoot == "original-document-assets");
  assert(!QFile::exists(sEditedTemporaryTrack));
  assert(EditedResult.Tag.eResult == ROLLER_ED_RESULT_OK);
  assert(EditedResult.bHasTowerSnapshot);
  assert(EditedResult.Towers.size() == 2u);
  assert(EditedResult.Towers[0].fWorldPosition[0]
         != GoodResult.Towers[0].fWorldPosition[0]);
  assert(Document.ApplyResult(EditedResult));
  assert(Document.CanExport());

  const std::vector<uint8_t> InvalidTrackData = {'F', 'A', 'I', 'L'};
  const uint64_t ullFailedRequest = Service.EnqueueSerializedLoadAndRender(
      Document.GetDocumentId(), Document.GetDocumentRevision(),
      InvalidTrackData, "original-document-assets", QSize(4, 3), 1.0,
      Camera, Overlay);
  Document.BeginRequest(ullFailedRequest);
  const tEdRenderResult FailedResult = WaitForResult(Service, ullFailedRequest);
  const QString sFailedTemporaryTrack =
      QString::fromLocal8Bit(g_sLoadedTrackPath.c_str());
  assert(FailedResult.Tag.eResult == ROLLER_ED_RESULT_LOAD_FAILED);
  assert(FailedResult.bHasTowerSnapshot);
  assert(FailedResult.Towers.empty());
  assert(FailedResult.sErrorText == "copied load error");
  assert(g_sError == "a later facade call replaced the error buffer");
  assert(!QFile::exists(sFailedTemporaryTrack));
  assert(Document.ApplyResult(FailedResult));
  assert(Document.GetDisplayState()
      == eEdFrameDisplayState::STALE_AFTER_LOAD_FAILURE);
  assert(!Document.CanExport());

  const std::vector<uint8_t> RecoveredTrackData = {'R', 'E', 'C', 'O', 'V', 'E', 'R'};
  const uint64_t ullRecoveredRequest = Service.EnqueueSerializedLoadAndRender(
      Document.GetDocumentId(), Document.GetDocumentRevision(),
      RecoveredTrackData, "original-document-assets", QSize(4, 3), 1.0,
      Camera, Overlay);
  Document.BeginRequest(ullRecoveredRequest);
  const tEdRenderResult RecoveredResult =
      WaitForResult(Service, ullRecoveredRequest);
  assert(RecoveredResult.Tag.eResult == ROLLER_ED_RESULT_OK);
  assert(Document.ApplyResult(RecoveredResult));
  assert(Document.GetDisplayState() == eEdFrameDisplayState::CURRENT);
  assert(Document.CanExport());

  const uint32_t uiRenderCountBeforeStaleRequest = g_uiRenderCount.load();
  const uint64_t ullStaleRequest = Service.EnqueueRender(
      Document.GetDocumentId(), Document.GetDocumentRevision(),
      Document.GetInstalledGeometryEpoch() - 1,
      QSize(4, 3), 1.0, Camera, Overlay);
  Document.BeginRequest(ullStaleRequest);
  const tEdRenderResult StaleResult = WaitForResult(Service, ullStaleRequest);
  assert(StaleResult.Tag.eResult == ROLLER_ED_RESULT_STALE);
  assert(g_uiRenderCount.load() == uiRenderCountBeforeStaleRequest);

  CDocumentFrameState TabB(CEditorRenderIds::NextDocumentId());
  Service.RegisterDocument(TabB.GetDocumentId());
  const std::vector<uint8_t> TabBTrackData = {'T', 'A', 'B', 'B'};
  const uint64_t ullTabBRequest = Service.EnqueueSerializedLoadAndRender(
      TabB.GetDocumentId(), TabB.GetDocumentRevision(), TabBTrackData,
      "tab-b-document-assets", QSize(4, 3), 1.0, Camera, Overlay);
  TabB.BeginRequest(ullTabBRequest);
  const tEdRenderResult TabBResult = WaitForResult(Service, ullTabBRequest);
  assert(g_sLoadedTrackData == "TABB");
  assert(g_sLoadedAssetRoot == "tab-b-document-assets");
  assert(TabB.ApplyResult(TabBResult));
  assert(TabB.CanExport());
  assert(g_uiInitCount.load() == 1);

  TabB.MarkDocumentEdited();
  const uint64_t ullEmptyTabBRequest = Service.EnqueueUnload(
      TabB.GetDocumentId(), TabB.GetDocumentRevision());
  TabB.BeginRequest(ullEmptyTabBRequest);
  const tEdRenderResult EmptyTabBResult =
      WaitForResult(Service, ullEmptyTabBRequest);
  assert(EmptyTabBResult.Tag.eResult == ROLLER_ED_RESULT_OK);
  assert(EmptyTabBResult.bSceneEmpty);
  assert(EmptyTabBResult.bHasTowerSnapshot);
  assert(EmptyTabBResult.Towers.empty());
  assert(g_uiSceneState == ROLLER_ED_SCENE_EMPTY);
  assert(TabB.ApplyResult(EmptyTabBResult));
  assert(TabB.GetDisplayState() == eEdFrameDisplayState::PLACEHOLDER);
  assert(TabB.GetImage().isNull());
  assert(!TabB.CanExport());

  TabB.MarkDocumentEdited();
  const std::vector<uint8_t> RedoneTabBTrackData = {'T', 'A', 'B', 'B', '-', 'R', 'E', 'D', 'O'};
  const uint64_t ullRedoneTabBRequest = Service.EnqueueSerializedLoadAndRender(
      TabB.GetDocumentId(), TabB.GetDocumentRevision(), RedoneTabBTrackData,
      "tab-b-document-assets", QSize(4, 3), 1.0, Camera, Overlay);
  TabB.BeginRequest(ullRedoneTabBRequest);
  const tEdRenderResult RedoneTabBResult =
      WaitForResult(Service, ullRedoneTabBRequest);
  assert(g_sLoadedTrackData == "TABB-REDO");
  assert(RedoneTabBResult.Tag.eResult == ROLLER_ED_RESULT_OK);
  assert(TabB.ApplyResult(RedoneTabBResult));
  assert(TabB.GetDisplayState() == eEdFrameDisplayState::CURRENT);
  assert(TabB.CanExport());

  // E4-S1. Export reads the canonical geometry through the same worker, on a
  // blocking call rather than through the frame-delivery signal.
  {
    const uint32_t uiRenderCountBeforeExtraction = g_uiRenderCount.load();
    tEdGeometrySnapshot Snapshot;
    std::string sExtractError;
    const eRollerEdResult eExtractResult = Service.ExtractGeometry(
        TabB.GetDocumentId(), TabB.GetDocumentRevision(), Snapshot,
        sExtractError);
    assert(eExtractResult == ROLLER_ED_RESULT_OK);
    assert(g_uiFillCount.load() == 1);
    // Extraction renders nothing and moves no epoch.
    assert(g_uiRenderCount.load() == uiRenderCountBeforeExtraction);
    assert(Snapshot.uiGeometryEpoch == g_uiGeometryEpoch);
    assert(Snapshot.uiTrackGeneration == g_uiTrackGeneration);
    // The caller owns the arrays outright; no core-owned pointer escaped.
    assert(Snapshot.Vertices.size() == g_uiStubVertexCount);
    assert(Snapshot.Indices.size() == g_uiStubIndexCount);
    assert(Snapshot.Primitives.size() == g_uiStubPrimitiveCount);
    assert(Snapshot.Materials.size() == g_uiStubMaterialCount);
    assert(Snapshot.Vertices[0].fPosition[0] == 100.0f);
    assert(Snapshot.Primitives[0].unSurfaceClass
           == ROLLER_ED_SURFACE_CLASS_CENTER);

    // A document that no longer owns the worker scene cannot export the one
    // that does.
    tEdGeometrySnapshot StaleSnapshot;
    std::string sStaleError;
    const eRollerEdResult eStaleResult = Service.ExtractGeometry(
        Document.GetDocumentId(), Document.GetDocumentRevision(),
        StaleSnapshot, sStaleError);
    assert(eStaleResult == ROLLER_ED_RESULT_STALE);
    assert(!sStaleError.empty());
    assert(StaleSnapshot.Primitives.empty());
    assert(g_uiFillCount.load() == 1);

    // An unloaded scene publishes zero counts, so there is nothing to export
    // and the fill is never reached.
    const uint64_t ullUnloadRequest = Service.EnqueueUnload(
        TabB.GetDocumentId(), TabB.GetDocumentRevision());
    TabB.BeginRequest(ullUnloadRequest);
    WaitForResult(Service, ullUnloadRequest);
    tEdGeometrySnapshot EmptySnapshot;
    std::string sEmptyError;
    const eRollerEdResult eEmptyResult = Service.ExtractGeometry(
        TabB.GetDocumentId(), TabB.GetDocumentRevision(), EmptySnapshot,
        sEmptyError);
    assert(eEmptyResult == ROLLER_ED_RESULT_NO_SCENE);
    assert(EmptySnapshot.Primitives.empty());
    assert(g_uiFillCount.load() == 1);
  }

  Service.InvalidateDocument(TabB.GetDocumentId());
  TabB.Invalidate();
  Service.InvalidateDocument(Document.GetDocumentId());
  Document.Invalidate();
  Service.Stop();
  assert(g_bAllFacadeCallsOnWorker.load());
  assert(g_uiInitCount.load() == 1);

  // E4-S1. A blocking extraction against a stopped worker must fail rather
  // than wait forever for a request nothing will ever run.
  {
    CDocumentFrameState LateDocument(CEditorRenderIds::NextDocumentId());
    Service.RegisterDocument(LateDocument.GetDocumentId());
    tEdGeometrySnapshot LateSnapshot;
    std::string sLateError;
    const eRollerEdResult eLateResult = Service.ExtractGeometry(
        LateDocument.GetDocumentId(), LateDocument.GetDocumentRevision(),
        LateSnapshot, sLateError);
    assert(eLateResult != ROLLER_ED_RESULT_OK);
    assert(!sLateError.empty());
    assert(LateSnapshot.Primitives.empty());
  }

  std::cout << "E3-S1/S2 editor render service tests passed\n";
  return 0;
}
