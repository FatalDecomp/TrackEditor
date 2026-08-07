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
float g_fReferenceFirstX = 0.0f;
float g_fReferenceScaleX = 0.0f;
uint32_t g_uiGeometryEpoch = 0;
uint32_t g_uiSceneState = ROLLER_ED_SCENE_EMPTY;
std::atomic<uint32_t> g_uiInitCount(0);
std::atomic<uint32_t> g_uiRenderCount(0);
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
  RecordFacadeThread();
  g_sLoadedTrackPath = szTrackPath ? szTrackPath : "";
  g_sLoadedAssetRoot = szDocumentAssetRoot ? szDocumentAssetRoot : "";
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
  pSizesOut->uiSceneState = g_uiSceneState;
  g_sError = "a later facade call replaced the error buffer";
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
      sAssetRoot, QSize(4, 3), 1.0, Camera, Overlay);
  Document.BeginRequest(ullGoodRequest);

  // The queued command must not retain any caller-owned pointer payload.
  sTrackPath = "fail.trk";
  sAssetRoot = "mutated-assets";
  Camera.fPosition[0] = 999.0f;
  Overlay.uiSurfaceClassMask = 0xffffu;
  Overlay.uiWireframeClassMask = 0xffffu;

  Service.Start();
  const tEdRenderResult GoodResult = WaitForResult(Service, ullGoodRequest);
  assert(g_sLoadedTrackPath == "good.trk");
  assert(g_sLoadedAssetRoot == "document-assets");
  assert(g_fCameraX == 123.0f);
  // AD-16: the overlay was deep-copied into the command, so mutating the
  // caller's copy after enqueueing cannot reach the worker.
  assert(g_uiOverlaySurfaceClassMask
         == ROLLER_ED_OVERLAY_CLASS_BIT(ROLLER_ED_SURFACE_CLASS_CENTER));
  assert(g_uiOverlayWireframeClassMask
         == ROLLER_ED_OVERLAY_CLASS_BIT(ROLLER_ED_SURFACE_CLASS_ROOF));
  assert((g_uiOverlayFlags & ROLLER_ED_OVERLAY_SHOW_SURFACES) != 0);
  assert(g_uiOverlayCount == 1);
  // E3A-S7: no mesh was supplied, so the worker never touched the facade's.
  assert(g_uiReferenceMeshCount == 0);
  assert(GoodResult.Tag.eResult == ROLLER_ED_RESULT_OK);
  assert(GoodResult.Tag.uiActualGeometryEpoch == 1);
  assert(GoodResult.uiRenderedGeometryEpoch == 1);
  assert(GoodResult.Image.size() == QSize(4, 3));
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
    assert(g_uiReferenceMeshCount == 1);
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

  Service.InvalidateDocument(TabB.GetDocumentId());
  TabB.Invalidate();
  Service.InvalidateDocument(Document.GetDocumentId());
  Document.Invalidate();
  Service.Stop();
  assert(g_bAllFacadeCallsOnWorker.load());
  assert(g_uiInitCount.load() == 1);

  std::cout << "E3-S1/S2 editor render service tests passed\n";
  return 0;
}
