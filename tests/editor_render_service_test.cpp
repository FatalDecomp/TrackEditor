#include "EditorRenderQueue.h"
#include "EditorRenderService.h"

#include <QCoreApplication>
#include <QEventLoop>
#include <QThread>
#include <QTimer>

#include <atomic>
#include <cassert>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>

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
float g_fCameraX = 0.0f;
uint32_t g_uiGeometryEpoch = 0;
uint32_t g_uiSceneState = ROLLER_ED_SCENE_EMPTY;
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
  ++g_uiGeometryEpoch;
  if (g_sLoadedTrackPath == "fail.trk") {
    g_uiSceneState = ROLLER_ED_SCENE_FAILED;
    g_sError = "copied load error";
    return ROLLER_ED_RESULT_LOAD_FAILED;
  }
  g_uiSceneState = ROLLER_ED_SCENE_READY;
  g_sError.clear();
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
  const uint64_t ullGoodRequest = Service.EnqueueLoadAndRender(
      Document.GetDocumentId(), Document.GetDocumentRevision(), sTrackPath,
      sAssetRoot, QSize(4, 3), 1.0, Camera);
  Document.BeginRequest(ullGoodRequest);

  // The queued command must not retain any caller-owned pointer payload.
  sTrackPath = "fail.trk";
  sAssetRoot = "mutated-assets";
  Camera.fPosition[0] = 999.0f;

  Service.Start();
  const tEdRenderResult GoodResult = WaitForResult(Service, ullGoodRequest);
  assert(g_sLoadedTrackPath == "good.trk");
  assert(g_sLoadedAssetRoot == "document-assets");
  assert(g_fCameraX == 123.0f);
  assert(GoodResult.Tag.eResult == ROLLER_ED_RESULT_OK);
  assert(GoodResult.Tag.uiActualGeometryEpoch == 1);
  assert(GoodResult.uiRenderedGeometryEpoch == 1);
  assert(GoodResult.Image.size() == QSize(4, 3));
  assert(Document.ApplyResult(GoodResult));
  assert(Document.CanExport());

  const uint64_t ullFailedRequest = Service.EnqueueLoadAndRender(
      Document.GetDocumentId(), Document.GetDocumentRevision(), "fail.trk",
      "document-assets", QSize(4, 3), 1.0, Camera);
  Document.BeginRequest(ullFailedRequest);
  const tEdRenderResult FailedResult = WaitForResult(Service, ullFailedRequest);
  assert(FailedResult.Tag.eResult == ROLLER_ED_RESULT_LOAD_FAILED);
  assert(FailedResult.sErrorText == "copied load error");
  assert(g_sError == "a later facade call replaced the error buffer");
  assert(Document.ApplyResult(FailedResult));
  assert(Document.GetDisplayState()
      == eEdFrameDisplayState::STALE_AFTER_LOAD_FAILURE);
  assert(!Document.CanExport());

  const uint32_t uiRenderCountBeforeStaleRequest = g_uiRenderCount.load();
  const uint64_t ullStaleRequest = Service.EnqueueRender(
      Document.GetDocumentId(), Document.GetDocumentRevision(), 1,
      QSize(4, 3), 1.0, Camera);
  Document.BeginRequest(ullStaleRequest);
  const tEdRenderResult StaleResult = WaitForResult(Service, ullStaleRequest);
  assert(StaleResult.Tag.eResult == ROLLER_ED_RESULT_STALE);
  assert(g_uiRenderCount.load() == uiRenderCountBeforeStaleRequest);

  Service.InvalidateDocument(Document.GetDocumentId());
  Document.Invalidate();
  Service.Stop();
  assert(g_bAllFacadeCallsOnWorker.load());

  std::cout << "E3-S1 editor render service tests passed\n";
  return 0;
}
