#include "EditorRenderService.h"

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QMetaObject>
#include <QMutex>
#include <QMutexLocker>
#include <QPointer>
#include <QThread>
#include <QTemporaryFile>
#include <QWaitCondition>

#include <algorithm>
#include <deque>
#include <limits>
#include <string>
#include <unordered_set>
#include <utility>

namespace
{
std::string EncodePath(const QString &sPath)
{
  const QByteArray Encoded = QFile::encodeName(sPath);
  return std::string(Encoded.constData(), static_cast<size_t>(Encoded.size()));
}

QSize NormalizeDevicePixelSize(const QSize &Size)
{
  return QSize(std::max(1, Size.width()), std::max(1, Size.height()));
}
}

class CEditorRenderThread : public QThread
{
public:
  CEditorRenderThread(CEditorRenderService *pOwner, std::string sAssetRoot)
    : m_pOwner(pOwner)
    , m_sAssetRoot(std::move(sAssetRoot))
    , m_bStopping(false)
    , m_bInitAttempted(false)
    , m_eInitResult(ROLLER_ED_RESULT_NOT_INITIALIZED)
    , m_ullActiveDocumentId(0)
  {
  }

  void Enqueue(tEdRenderRequest Request)
  {
    QMutexLocker Locker(&m_Mutex);
    if (m_bStopping || m_InvalidDocuments.count(Request.Tag.ullDocumentId) != 0)
      return;
    m_Requests.push_back(std::move(Request));
    m_WorkAvailable.wakeOne();
  }

  void InvalidateDocument(uint64_t ullDocumentId)
  {
    QMutexLocker Locker(&m_Mutex);
    m_InvalidDocuments.insert(ullDocumentId);
    m_Requests.erase(
        std::remove_if(m_Requests.begin(), m_Requests.end(),
                       [ullDocumentId](const tEdRenderRequest &Request) {
                         return Request.Tag.ullDocumentId == ullDocumentId;
                       }),
        m_Requests.end());
  }

  void StopAndWait()
  {
    {
      QMutexLocker Locker(&m_Mutex);
      if (m_bStopping)
        return;
      m_bStopping = true;
      m_Requests.clear();
      m_WorkAvailable.wakeOne();
    }
    wait();
  }

protected:
  void run() override
  {
    AssertWorkerThread("RollerEd_Init");
    tRollerEdInitInfo InitInfo = {};
    InitInfo.uiStructSize = sizeof(InitInfo);
    InitInfo.uiVersion = ROLLER_ED_INIT_INFO_VERSION;
    InitInfo.szAssetRoot = m_sAssetRoot.c_str();
    InitInfo.ePreferredRenderer = ROLLER_ED_RENDERER_GPU;
    InitInfo.uiAllowSoftwareFallback = 1;
    m_bInitAttempted = true;
    m_eInitResult = RollerEd_Init(&InitInfo);
    if (m_eInitResult != ROLLER_ED_RESULT_OK)
      m_sInitError = CopyFacadeError();

    for (;;) {
      tEdRenderRequest Request;
      {
        QMutexLocker Locker(&m_Mutex);
        while (!m_bStopping && m_Requests.empty())
          m_WorkAvailable.wait(&m_Mutex);
        if (m_bStopping)
          break;
        Request = std::move(m_Requests.front());
        m_Requests.pop_front();
        if (m_InvalidDocuments.count(Request.Tag.ullDocumentId) != 0)
          continue;
      }

      tEdRenderResult Result = ProcessRequest(Request);
      {
        QMutexLocker Locker(&m_Mutex);
        if (m_InvalidDocuments.count(Request.Tag.ullDocumentId) != 0)
          continue;
      }
      PostResult(std::move(Result));
    }

    if (m_bInitAttempted) {
      AssertWorkerThread("RollerEd_Shutdown");
      RollerEd_Shutdown();
    }
  }

private:
  void AssertWorkerThread(const char *szCall) const
  {
    Q_ASSERT_X(QThread::currentThread() == this,
               "CEditorRenderThread", szCall);
  }

  std::string CopyFacadeError()
  {
    AssertWorkerThread("RollerEd_GetLastError");
    const char *szError = RollerEd_GetLastError();
    return szError ? std::string(szError) : std::string();
  }

  void SetFacadeFailure(tEdRenderResult &Result, eRollerEdResult eResult)
  {
    Result.Tag.eResult = eResult;
    Result.sErrorText = CopyFacadeError();
  }

  void QueryFailedLoadEpoch(tEdRenderResult &Result)
  {
    tEdGeometrySizes FailedSizes = {};
    FailedSizes.uiStructSize = sizeof(FailedSizes);
    FailedSizes.uiVersion = ROLLER_ED_GEOMETRY_SIZES_VERSION;
    AssertWorkerThread("RollerEd_QueryGeometrySizes after failed load");
    if (RollerEd_QueryGeometrySizes(&FailedSizes) == ROLLER_ED_RESULT_OK)
      Result.Tag.uiActualGeometryEpoch = FailedSizes.uiGeometryEpoch;
  }

  void SetSerializedTrackFailure(tEdRenderResult &Result,
                                 const tEdRenderRequest &Request,
                                 const std::string &sErrorText)
  {
    Result.Tag.eResult = ROLLER_ED_RESULT_LOAD_FAILED;
    Result.bLoadFailed = true;
    Result.sErrorText = sErrorText;

    // A host-side temporary-file failure has the same scene semantics as a
    // failed facade load: the worker scene must not keep an older document.
    AssertWorkerThread("RollerEd_UnloadTrack after serialized load failure");
    RollerEd_UnloadTrack();
    QueryFailedLoadEpoch(Result);
    m_ullActiveDocumentId = Request.Tag.ullDocumentId;
  }

  bool QueryGeometry(tEdRenderResult &Result, tEdGeometrySizes &Sizes)
  {
    Sizes = {};
    Sizes.uiStructSize = sizeof(Sizes);
    Sizes.uiVersion = ROLLER_ED_GEOMETRY_SIZES_VERSION;
    AssertWorkerThread("RollerEd_QueryGeometrySizes");
    const eRollerEdResult eResult = RollerEd_QueryGeometrySizes(&Sizes);
    if (eResult != ROLLER_ED_RESULT_OK) {
      SetFacadeFailure(Result, eResult);
      return false;
    }
    Result.Tag.uiActualGeometryEpoch = Sizes.uiGeometryEpoch;
    return true;
  }

  tEdRenderResult ProcessRequest(const tEdRenderRequest &Request)
  {
    AssertWorkerThread("render request");
    tEdRenderResult Result;
    Result.Tag.ullRequestId = Request.Tag.ullRequestId;
    Result.Tag.ullDocumentId = Request.Tag.ullDocumentId;
    Result.Tag.ullDocumentRevision = Request.Tag.ullDocumentRevision;
    Result.Tag.eResult = ROLLER_ED_RESULT_OK;

    const bool bLoadCommand = Request.eKind
        != eEdRenderCommandKind::RENDER_ONLY;
    if (m_eInitResult != ROLLER_ED_RESULT_OK) {
      Result.Tag.eResult = m_eInitResult;
      Result.bLoadFailed = bLoadCommand;
      Result.sErrorText = m_sInitError;
      return Result;
    }

    tEdGeometrySizes Sizes = {};
    if (Request.eKind == eEdRenderCommandKind::UNLOAD) {
      AssertWorkerThread("RollerEd_UnloadTrack for empty document");
      const eRollerEdResult eUnloadResult = RollerEd_UnloadTrack();
      if (eUnloadResult != ROLLER_ED_RESULT_OK) {
        Result.bLoadFailed = true;
        SetFacadeFailure(Result, eUnloadResult);
        return Result;
      }
      m_ullActiveDocumentId = Request.Tag.ullDocumentId;
      if (!QueryGeometry(Result, Sizes))
        return Result;
      Result.bSceneEmpty = true;
      Result.sErrorText = "Track has no geometry chunks";
      return Result;
    }

    QTemporaryFile TemporaryTrack;
    std::string sTrackPath = Request.sTrackPath;
    if (Request.eKind
        == eEdRenderCommandKind::LOAD_SERIALIZED_AND_RENDER) {
      TemporaryTrack.setFileTemplate(
          QDir::temp().filePath("TrackEditor-render-XXXXXX.TRK"));
      if (!TemporaryTrack.open()) {
        SetSerializedTrackFailure(
            Result, Request,
            std::string("could not create temporary track: ")
                + TemporaryTrack.errorString().toStdString());
        return Result;
      }

      const size_t uiDataSize = Request.SerializedTrackData.size();
      if (uiDataSize
              > static_cast<size_t>(std::numeric_limits<qint64>::max())
          || TemporaryTrack.write(
                 reinterpret_cast<const char *>(
                     Request.SerializedTrackData.data()),
                 static_cast<qint64>(uiDataSize))
              != static_cast<qint64>(uiDataSize)
          || !TemporaryTrack.flush()) {
        SetSerializedTrackFailure(
            Result, Request,
            std::string("could not write temporary track: ")
                + TemporaryTrack.errorString().toStdString());
        return Result;
      }

      sTrackPath = EncodePath(TemporaryTrack.fileName());
      TemporaryTrack.close();
    }

    if (bLoadCommand) {
      AssertWorkerThread("RollerEd_LoadTrackFile");
      const eRollerEdResult eLoadResult = RollerEd_LoadTrackFile(
          sTrackPath.c_str(), Request.sDocumentAssetRoot.c_str());
      if (eLoadResult != ROLLER_ED_RESULT_OK) {
        Result.Tag.eResult = eLoadResult;
        Result.bLoadFailed = true;
        Result.sErrorText = CopyFacadeError();

        // The failed load has already advanced the actual geometry epoch. Preserve
        // the copied load error before this next facade call invalidates its pointer.
        QueryFailedLoadEpoch(Result);
        m_ullActiveDocumentId = Request.Tag.ullDocumentId;
        return Result;
      }
      m_ullActiveDocumentId = Request.Tag.ullDocumentId;
      if (!QueryGeometry(Result, Sizes))
        return Result;
    } else {
      if (m_ullActiveDocumentId != Request.Tag.ullDocumentId) {
        Result.Tag.eResult = ROLLER_ED_RESULT_STALE;
        Result.sErrorText = "render request no longer owns the worker scene";
        return Result;
      }
      if (!QueryGeometry(Result, Sizes))
        return Result;
      if ((Request.Tag.uiFlags & ROLLER_ED_REQUEST_HAS_EXPECTED_EPOCH) != 0
          && Request.Tag.uiExpectedGeometryEpoch != Sizes.uiGeometryEpoch) {
        Result.Tag.eResult = ROLLER_ED_RESULT_STALE;
        Result.sErrorText = "render request geometry epoch is stale";
        return Result;
      }
    }

    if (Sizes.uiSceneState != ROLLER_ED_SCENE_READY) {
      Result.Tag.eResult = ROLLER_ED_RESULT_NO_SCENE;
      Result.sErrorText = "render request has no ready scene";
      return Result;
    }

    if (Request.bHasCamera) {
      AssertWorkerThread("RollerEd_SetCamera");
      const eRollerEdResult eCameraResult = RollerEd_SetCamera(&Request.Camera);
      if (eCameraResult != ROLLER_ED_RESULT_OK) {
        SetFacadeFailure(Result, eCameraResult);
        return Result;
      }
    }

    if (Request.uiWidth == 0 || Request.uiHeight == 0
        || Request.uiWidth > static_cast<uint32_t>(std::numeric_limits<int>::max())
        || Request.uiHeight > static_cast<uint32_t>(std::numeric_limits<int>::max())) {
      Result.Tag.eResult = ROLLER_ED_RESULT_INVALID_ARGUMENT;
      Result.sErrorText = "invalid render target dimensions";
      return Result;
    }

    QImage Image(static_cast<int>(Request.uiWidth),
                 static_cast<int>(Request.uiHeight), QImage::Format_RGBA8888);
    const uint64_t ullBufferSize = static_cast<uint64_t>(Image.bytesPerLine())
        * static_cast<uint64_t>(Image.height());
    if (Image.isNull() || ullBufferSize > std::numeric_limits<uint32_t>::max()) {
      Result.Tag.eResult = ROLLER_ED_RESULT_OUT_OF_MEMORY;
      Result.sErrorText = "could not allocate the worker-owned frame buffer";
      return Result;
    }

    AssertWorkerThread("RollerEd_RenderFrame");
    const eRollerEdResult eRenderResult = RollerEd_RenderFrame(
        Image.bits(), static_cast<uint32_t>(ullBufferSize),
        static_cast<uint32_t>(Image.bytesPerLine()), Request.uiWidth,
        Request.uiHeight, ROLLER_ED_PIXEL_RGBA8);
    if (eRenderResult != ROLLER_ED_RESULT_OK) {
      SetFacadeFailure(Result, eRenderResult);
      return Result;
    }

    Image.setDevicePixelRatio(Request.dDevicePixelRatio);
    Result.uiRenderedGeometryEpoch = Sizes.uiGeometryEpoch;
    Result.Image = std::move(Image);
    return Result;
  }

  void PostResult(tEdRenderResult Result)
  {
    QPointer<CEditorRenderService> Owner(m_pOwner);
    QMetaObject::invokeMethod(
        m_pOwner,
        [Owner, Result = std::move(Result)]() mutable {
          if (Owner)
            Owner->PublishResult(std::move(Result));
        },
        Qt::QueuedConnection);
  }

  CEditorRenderService *m_pOwner;
  std::string m_sAssetRoot;
  QMutex m_Mutex;
  QWaitCondition m_WorkAvailable;
  std::deque<tEdRenderRequest> m_Requests;
  std::unordered_set<uint64_t> m_InvalidDocuments;
  bool m_bStopping;
  bool m_bInitAttempted;
  eRollerEdResult m_eInitResult;
  std::string m_sInitError;
  uint64_t m_ullActiveDocumentId;
};

CEditorRenderService::CEditorRenderService(const QString &sAssetRoot, QObject *pParent)
  : QObject(pParent)
  , m_pThread(new CEditorRenderThread(this, EncodePath(sAssetRoot)))
{
  Q_ASSERT(!sAssetRoot.isEmpty());
}

CEditorRenderService::~CEditorRenderService()
{
  Stop();
  delete m_pThread;
  m_pThread = nullptr;
}

void CEditorRenderService::Start()
{
  Q_ASSERT(QThread::currentThread() == thread());
  if (!m_pThread->isRunning())
    m_pThread->start();
}

void CEditorRenderService::Stop()
{
  Q_ASSERT(QThread::currentThread() == thread());
  if (m_pThread->isRunning())
    m_pThread->StopAndWait();
}

void CEditorRenderService::RegisterDocument(uint64_t ullDocumentId)
{
  Q_ASSERT(QThread::currentThread() == thread());
  if (ullDocumentId != 0)
    m_RegisteredDocuments.insert(ullDocumentId);
}

void CEditorRenderService::InvalidateDocument(uint64_t ullDocumentId)
{
  Q_ASSERT(QThread::currentThread() == thread());
  m_RegisteredDocuments.erase(ullDocumentId);
  m_pThread->InvalidateDocument(ullDocumentId);
}

bool CEditorRenderService::IsDocumentRegistered(uint64_t ullDocumentId) const
{
  return m_RegisteredDocuments.count(ullDocumentId) != 0;
}

uint64_t CEditorRenderService::EnqueueLoadAndRender(
    uint64_t ullDocumentId, uint64_t ullDocumentRevision,
    const QString &sTrackPath, const QString &sDocumentAssetRoot,
    const QSize &DevicePixelSize, double dDevicePixelRatio,
    const tEdCameraState &Camera)
{
  Q_ASSERT(QThread::currentThread() == thread());
  if (!IsDocumentRegistered(ullDocumentId) || sTrackPath.isEmpty())
    return 0;

  const QSize NormalizedSize = NormalizeDevicePixelSize(DevicePixelSize);
  tEdRenderRequest Request;
  Request.Tag.ullRequestId = CEditorRenderIds::NextRequestId();
  Request.Tag.ullDocumentId = ullDocumentId;
  Request.Tag.ullDocumentRevision = ullDocumentRevision;
  Request.eKind = eEdRenderCommandKind::LOAD_AND_RENDER;
  Request.sTrackPath = EncodePath(sTrackPath);
  Request.sDocumentAssetRoot = EncodePath(sDocumentAssetRoot);
  Request.Camera = Camera;
  Request.bHasCamera = true;
  Request.uiWidth = static_cast<uint32_t>(NormalizedSize.width());
  Request.uiHeight = static_cast<uint32_t>(NormalizedSize.height());
  Request.dDevicePixelRatio = dDevicePixelRatio;
  const uint64_t ullRequestId = Request.Tag.ullRequestId;
  m_pThread->Enqueue(std::move(Request));
  return ullRequestId;
}

uint64_t CEditorRenderService::EnqueueSerializedLoadAndRender(
    uint64_t ullDocumentId, uint64_t ullDocumentRevision,
    const std::vector<uint8_t> &SerializedTrackData,
    const QString &sDocumentAssetRoot, const QSize &DevicePixelSize,
    double dDevicePixelRatio, const tEdCameraState &Camera)
{
  Q_ASSERT(QThread::currentThread() == thread());
  if (!IsDocumentRegistered(ullDocumentId))
    return 0;

  const QSize NormalizedSize = NormalizeDevicePixelSize(DevicePixelSize);
  tEdRenderRequest Request;
  Request.Tag.ullRequestId = CEditorRenderIds::NextRequestId();
  Request.Tag.ullDocumentId = ullDocumentId;
  Request.Tag.ullDocumentRevision = ullDocumentRevision;
  Request.eKind = eEdRenderCommandKind::LOAD_SERIALIZED_AND_RENDER;
  Request.sDocumentAssetRoot = EncodePath(sDocumentAssetRoot);
  Request.SerializedTrackData = SerializedTrackData;
  Request.Camera = Camera;
  Request.bHasCamera = true;
  Request.uiWidth = static_cast<uint32_t>(NormalizedSize.width());
  Request.uiHeight = static_cast<uint32_t>(NormalizedSize.height());
  Request.dDevicePixelRatio = dDevicePixelRatio;
  const uint64_t ullRequestId = Request.Tag.ullRequestId;
  m_pThread->Enqueue(std::move(Request));
  return ullRequestId;
}

uint64_t CEditorRenderService::EnqueueRender(
    uint64_t ullDocumentId, uint64_t ullDocumentRevision,
    uint32_t uiExpectedGeometryEpoch, const QSize &DevicePixelSize,
    double dDevicePixelRatio, const tEdCameraState &Camera)
{
  Q_ASSERT(QThread::currentThread() == thread());
  if (!IsDocumentRegistered(ullDocumentId))
    return 0;

  const QSize NormalizedSize = NormalizeDevicePixelSize(DevicePixelSize);
  tEdRenderRequest Request;
  Request.Tag.ullRequestId = CEditorRenderIds::NextRequestId();
  Request.Tag.ullDocumentId = ullDocumentId;
  Request.Tag.ullDocumentRevision = ullDocumentRevision;
  Request.Tag.uiExpectedGeometryEpoch = uiExpectedGeometryEpoch;
  Request.Tag.uiFlags = ROLLER_ED_REQUEST_HAS_EXPECTED_EPOCH;
  Request.eKind = eEdRenderCommandKind::RENDER_ONLY;
  Request.Camera = Camera;
  Request.bHasCamera = true;
  Request.uiWidth = static_cast<uint32_t>(NormalizedSize.width());
  Request.uiHeight = static_cast<uint32_t>(NormalizedSize.height());
  Request.dDevicePixelRatio = dDevicePixelRatio;
  const uint64_t ullRequestId = Request.Tag.ullRequestId;
  m_pThread->Enqueue(std::move(Request));
  return ullRequestId;
}

uint64_t CEditorRenderService::EnqueueUnload(
    uint64_t ullDocumentId, uint64_t ullDocumentRevision)
{
  Q_ASSERT(QThread::currentThread() == thread());
  if (!IsDocumentRegistered(ullDocumentId))
    return 0;

  tEdRenderRequest Request;
  Request.Tag.ullRequestId = CEditorRenderIds::NextRequestId();
  Request.Tag.ullDocumentId = ullDocumentId;
  Request.Tag.ullDocumentRevision = ullDocumentRevision;
  Request.eKind = eEdRenderCommandKind::UNLOAD;
  const uint64_t ullRequestId = Request.Tag.ullRequestId;
  m_pThread->Enqueue(std::move(Request));
  return ullRequestId;
}

void CEditorRenderService::PublishResult(tEdRenderResult Result)
{
  Q_ASSERT(QThread::currentThread() == thread());
  if (!IsDocumentRegistered(Result.Tag.ullDocumentId))
    return;
  emit FrameCompleted(Result);
}
