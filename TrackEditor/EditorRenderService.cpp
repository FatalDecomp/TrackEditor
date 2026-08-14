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
#include <cstring>
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

// E4-S1. Wakes the blocked caller exactly once, whatever became of its
// request. Every path that drops a queued command routes through here.
void CompleteExtraction(const std::shared_ptr<tEdGeometryExtraction> &Slot,
                        eRollerEdResult eResult, std::string sErrorText,
                        tEdGeometrySnapshot *pSnapshot)
{
  if (!Slot)
    return;
  QMutexLocker Locker(&Slot->Mutex);
  if (Slot->bComplete)
    return;
  Slot->eResult = eResult;
  Slot->sErrorText = std::move(sErrorText);
  if (pSnapshot)
    Slot->Snapshot = std::move(*pSnapshot);
  Slot->bComplete = true;
  Slot->Completed.wakeAll();
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
    , m_ullAppliedGraphicsSettingsRevision(0)
  {
  }

  bool Enqueue(tEdRenderRequest Request)
  {
    QMutexLocker Locker(&m_Mutex);
    if (m_bStopping || m_InvalidDocuments.count(Request.Tag.ullDocumentId) != 0)
      return false;
    m_Requests.push_back(std::move(Request));
    m_WorkAvailable.wakeOne();
    return true;
  }

  void InvalidateDocument(uint64_t ullDocumentId)
  {
    std::vector<tEdRenderRequest> Dropped;
    {
      QMutexLocker Locker(&m_Mutex);
      m_InvalidDocuments.insert(ullDocumentId);
      TakeMatchingRequests(
          Dropped, [ullDocumentId](const tEdRenderRequest &Request) {
            return Request.Tag.ullDocumentId == ullDocumentId;
          });
    }
    FailDroppedRequests(Dropped, "the document was closed before its "
                                 "geometry could be extracted");
  }

  void StopAndWait()
  {
    std::vector<tEdRenderRequest> Dropped;
    {
      QMutexLocker Locker(&m_Mutex);
      if (m_bStopping)
        return;
      m_bStopping = true;
      TakeMatchingRequests(Dropped,
                           [](const tEdRenderRequest &) { return true; });
      m_WorkAvailable.wakeOne();
    }
    FailDroppedRequests(Dropped, "the render worker stopped before its "
                                 "geometry could be extracted");
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
        if (m_InvalidDocuments.count(Request.Tag.ullDocumentId) != 0) {
          Locker.unlock();
          CompleteExtraction(Request.Extraction, ROLLER_ED_RESULT_STALE,
                             "the document was closed before its geometry "
                             "could be extracted",
                             nullptr);
          continue;
        }
      }

      // Extraction produces no frame, so it never reaches the frame-delivery
      // path: it publishes into its own slot and wakes the blocked caller.
      if (Request.eKind == eEdRenderCommandKind::EXTRACT_GEOMETRY) {
        ProcessExtractGeometry(Request);
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
  // Callers hold m_Mutex. Moves out every request the predicate selects so the
  // caller can complete their extraction slots without holding the lock.
  template <typename TPredicate>
  void TakeMatchingRequests(std::vector<tEdRenderRequest> &Taken,
                            TPredicate Matches)
  {
    std::deque<tEdRenderRequest> Kept;
    for (std::deque<tEdRenderRequest>::iterator it = m_Requests.begin();
         it != m_Requests.end(); ++it) {
      if (Matches(*it))
        Taken.push_back(std::move(*it));
      else
        Kept.push_back(std::move(*it));
    }
    m_Requests = std::move(Kept);
  }

  static void FailDroppedRequests(std::vector<tEdRenderRequest> &Dropped,
                                  const char *szReason)
  {
    for (size_t i = 0; i < Dropped.size(); ++i) {
      CompleteExtraction(Dropped[i].Extraction, ROLLER_ED_RESULT_STALE,
                         szReason, nullptr);
    }
  }

  // szCall feeds Q_ASSERT_X, which compiles out of release builds.
  void AssertWorkerThread([[maybe_unused]] const char *szCall) const
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

  // E7-S6. Tower placement belongs to the committed ROLLER scene, so copy it
  // out beside the load on this worker. The UI receives only owned structs
  // and never calls the facade itself (AD-4e).
  bool QueryTowers(tEdRenderResult &Result)
  {
    uint32_t uiTowerCount = 0;
    AssertWorkerThread("RollerEd_QueryTowerCount");
    eRollerEdResult eResult = RollerEd_QueryTowerCount(&uiTowerCount);
    if (eResult != ROLLER_ED_RESULT_OK) {
      Result.bLoadFailed = true;
      SetFacadeFailure(Result, eResult);
      return false;
    }

    constexpr uint32_t MAX_TOWER_COUNT = 32u;
    if (uiTowerCount > MAX_TOWER_COUNT) {
      Result.Tag.eResult = ROLLER_ED_RESULT_INVALID_ARGUMENT;
      Result.bLoadFailed = true;
      Result.sErrorText = "the core published more than 32 towers";
      return false;
    }

    Result.Towers.resize(uiTowerCount);
    for (uint32_t uiIndex = 0; uiIndex < uiTowerCount; ++uiIndex) {
      tEdTowerInfo &Info = Result.Towers[uiIndex];
      Info = {};
      Info.uiStructSize = sizeof(Info);
      Info.uiVersion = ROLLER_ED_TOWER_INFO_VERSION;
      AssertWorkerThread("RollerEd_QueryTower");
      eResult = RollerEd_QueryTower(uiIndex, &Info);
      if (eResult != ROLLER_ED_RESULT_OK) {
        Result.Towers.clear();
        Result.bLoadFailed = true;
        SetFacadeFailure(Result, eResult);
        return false;
      }
    }
    return true;
  }

  // E4-S1. Query then fill, both on the worker, both against the epoch the
  // query reported. RollerEd_FillGeometry refuses in a fixed order and writes
  // nothing on refusal, so a failed extraction leaves the snapshot untouched.
  void ProcessExtractGeometry(const tEdRenderRequest &Request)
  {
    AssertWorkerThread("geometry extraction");

    if (m_eInitResult != ROLLER_ED_RESULT_OK) {
      CompleteExtraction(Request.Extraction, m_eInitResult, m_sInitError,
                         nullptr);
      return;
    }
    if (m_ullActiveDocumentId != Request.Tag.ullDocumentId) {
      CompleteExtraction(Request.Extraction, ROLLER_ED_RESULT_STALE,
                         "the exporting document no longer owns the worker "
                         "scene",
                         nullptr);
      return;
    }

    tEdGeometrySizes Sizes = {};
    Sizes.uiStructSize = sizeof(Sizes);
    Sizes.uiVersion = ROLLER_ED_GEOMETRY_SIZES_VERSION;
    AssertWorkerThread("RollerEd_QueryGeometrySizes for export");
    eRollerEdResult eResult = RollerEd_QueryGeometrySizes(&Sizes);
    if (eResult != ROLLER_ED_RESULT_OK) {
      CompleteExtraction(Request.Extraction, eResult, CopyFacadeError(),
                         nullptr);
      return;
    }
    if (Sizes.uiSceneState != ROLLER_ED_SCENE_READY) {
      CompleteExtraction(Request.Extraction, ROLLER_ED_RESULT_NO_SCENE,
                         "the exporting document has no ready scene", nullptr);
      return;
    }
    if (Sizes.uiPrimitiveCount == 0 || Sizes.uiVertexCount == 0
        || Sizes.uiIndexCount == 0 || Sizes.uiMaterialCount == 0) {
      CompleteExtraction(Request.Extraction, ROLLER_ED_RESULT_NO_SCENE,
                         "the loaded track produced no exportable geometry",
                         nullptr);
      return;
    }
    // The ABI is frozen per struct (AD-12); a core built against a different
    // layout would silently misread every array.
    if (Sizes.uiVertexStride != sizeof(tEdVertex)
        || Sizes.uiPrimitiveStride != sizeof(tEdPrimitive)
        || Sizes.uiMaterialStride != sizeof(tEdMaterial)) {
      CompleteExtraction(Request.Extraction, ROLLER_ED_RESULT_INVALID_VERSION,
                         "the core publishes a geometry layout this build "
                         "does not share",
                         nullptr);
      return;
    }

    tEdGeometrySnapshot Snapshot;
    Snapshot.uiGeometryEpoch = Sizes.uiGeometryEpoch;
    Snapshot.uiTrackGeneration = Sizes.uiTrackGeneration;
    Snapshot.Vertices.resize(Sizes.uiVertexCount);
    Snapshot.Indices.resize(Sizes.uiIndexCount);
    Snapshot.Primitives.resize(Sizes.uiPrimitiveCount);
    Snapshot.Materials.resize(Sizes.uiMaterialCount);

    AssertWorkerThread("RollerEd_FillGeometry");
    eResult = RollerEd_FillGeometry(
        Sizes.uiGeometryEpoch, Snapshot.Vertices.data(), Sizes.uiVertexCount,
        Snapshot.Indices.data(), Sizes.uiIndexCount,
        Snapshot.Primitives.data(), Sizes.uiPrimitiveCount,
        Snapshot.Materials.data(), Sizes.uiMaterialCount);
    if (eResult != ROLLER_ED_RESULT_OK) {
      CompleteExtraction(Request.Extraction, eResult, CopyFacadeError(),
                         nullptr);
      return;
    }

    CompleteExtraction(Request.Extraction, ROLLER_ED_RESULT_OK, std::string(),
                       &Snapshot);
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
    Result.bHasTowerSnapshot = bLoadCommand;
    if (m_eInitResult != ROLLER_ED_RESULT_OK) {
      Result.Tag.eResult = m_eInitResult;
      Result.bLoadFailed = bLoadCommand;
      Result.sErrorText = m_sInitError;
      return Result;
    }

    if (Request.ullGraphicsSettingsRevision != 0
        && Request.ullGraphicsSettingsRevision
            != m_ullAppliedGraphicsSettingsRevision) {
      AssertWorkerThread("RollerEd_SetGraphicsSettings");
      const eRollerEdResult eGraphicsResult =
          RollerEd_SetGraphicsSettings(&Request.GraphicsSettings);
      if (eGraphicsResult != ROLLER_ED_RESULT_OK) {
        Result.bLoadFailed = bLoadCommand;
        SetFacadeFailure(Result, eGraphicsResult);
        return Result;
      }
      m_ullAppliedGraphicsSettingsRevision =
          Request.ullGraphicsSettingsRevision;
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

    if (bLoadCommand && !QueryTowers(Result))
      return Result;

    if (Request.uiStuntTicks != 0) {
      AssertWorkerThread("RollerEd_AdvanceStunts");
      const eRollerEdResult eStuntResult =
          RollerEd_AdvanceStunts(Request.uiStuntTicks);
      if (eStuntResult != ROLLER_ED_RESULT_OK) {
        SetFacadeFailure(Result, eStuntResult);
        return Result;
      }
    }

    if (Request.bHasCamera) {
      AssertWorkerThread("RollerEd_SetCamera");
      const eRollerEdResult eCameraResult = RollerEd_SetCamera(&Request.Camera);
      if (eCameraResult != ROLLER_ED_RESULT_OK) {
        SetFacadeFailure(Result, eCameraResult);
        return Result;
      }
    }

    // Overlay state is applied on the worker like the camera, and like the
    // camera it moves neither the geometry epoch nor the track generation, so
    // the epoch checked above stays the one this frame renders at.
    if (Request.bHasOverlay) {
      AssertWorkerThread("RollerEd_SetOverlayState");
      const eRollerEdResult eOverlayResult =
          RollerEd_SetOverlayState(&Request.Overlay);
      if (eOverlayResult != ROLLER_ED_RESULT_OK) {
        SetFacadeFailure(Result, eOverlayResult);
        return Result;
      }
    }

    // E3A-S7. Same reasoning as the overlay: the mesh is a view setting, so
    // it moves neither the geometry epoch nor the track generation and the
    // epoch checked above is still the one this frame renders at. The facade
    // copies the arrays during the call, so the payload's storage only has to
    // outlive this statement.
    if (Request.bHasReferenceMesh) {
      AssertWorkerThread("RollerEd_SetReferenceMesh");
      tEdReferenceMesh Mesh;
      std::memset(&Mesh, 0, sizeof(Mesh));
      Mesh.uiStructSize = sizeof(Mesh);
      Mesh.uiVersion = ROLLER_ED_REFERENCE_MESH_VERSION;
      if (!Request.ReferenceMesh.Vertices.empty()) {
        Mesh.pVertices = Request.ReferenceMesh.Vertices.data();
        Mesh.uiVertexCount =
            static_cast<uint32_t>(Request.ReferenceMesh.Vertices.size());
      }
      if (!Request.ReferenceMesh.Indices.empty()) {
        Mesh.puiIndices = Request.ReferenceMesh.Indices.data();
        Mesh.uiIndexCount =
            static_cast<uint32_t>(Request.ReferenceMesh.Indices.size());
      }
      std::memcpy(Mesh.fPosition, Request.ReferenceMesh.fPosition,
                  sizeof(Mesh.fPosition));
      std::memcpy(Mesh.fRotation, Request.ReferenceMesh.fRotation,
                  sizeof(Mesh.fRotation));
      std::memcpy(Mesh.fScale, Request.ReferenceMesh.fScale,
                  sizeof(Mesh.fScale));
      Mesh.uiFlags = Request.ReferenceMesh.uiFlags;

      const eRollerEdResult eMeshResult = RollerEd_SetReferenceMesh(&Mesh);
      if (eMeshResult != ROLLER_ED_RESULT_OK) {
        SetFacadeFailure(Result, eMeshResult);
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
  uint64_t m_ullAppliedGraphicsSettingsRevision;
};

CEditorRenderService::CEditorRenderService(const QString &sAssetRoot, QObject *pParent)
  : QObject(pParent)
  , m_pThread(new CEditorRenderThread(this, EncodePath(sAssetRoot)))
  , m_ullGraphicsSettingsRevision(1)
{
  Q_ASSERT(!sAssetRoot.isEmpty());
  m_GraphicsSettings = {};
  m_GraphicsSettings.uiStructSize = sizeof(m_GraphicsSettings);
  m_GraphicsSettings.uiVersion = ROLLER_ED_GRAPHICS_SETTINGS_VERSION;
  m_GraphicsSettings.eRenderer = ROLLER_ED_RENDERER_GPU;
  m_GraphicsSettings.eSoftwareDisplay = ROLLER_ED_SOFTWARE_DISPLAY_SVGA;
  m_GraphicsSettings.eAntiAliasing = ROLLER_ED_ANTI_ALIASING_OFF;
  m_GraphicsSettings.eAnisotropy = ROLLER_ED_ANISOTROPY_16X;
  m_GraphicsSettings.eTextureFilter = ROLLER_ED_TEXTURE_FILTER_NEAREST;
  m_GraphicsSettings.fDrawDistanceFraction = 1.0f;
  m_GraphicsSettings.uiEmulateTransparentBorders = 1u;
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

void CEditorRenderService::SetGraphicsSettings(
    const tEdGraphicsSettings &Settings)
{
  Q_ASSERT(QThread::currentThread() == thread());
  m_GraphicsSettings = Settings;
  if (m_ullGraphicsSettingsRevision == std::numeric_limits<uint64_t>::max())
    std::terminate();
  ++m_ullGraphicsSettingsRevision;
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
    const tEdCameraState &Camera, const tEdOverlayState &Overlay)
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
  Request.Overlay = Overlay;
  Request.bHasOverlay = true;
  Request.GraphicsSettings = m_GraphicsSettings;
  Request.ullGraphicsSettingsRevision = m_ullGraphicsSettingsRevision;
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
    double dDevicePixelRatio, const tEdCameraState &Camera,
    const tEdOverlayState &Overlay)
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
  Request.Overlay = Overlay;
  Request.bHasOverlay = true;
  Request.GraphicsSettings = m_GraphicsSettings;
  Request.ullGraphicsSettingsRevision = m_ullGraphicsSettingsRevision;
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
    double dDevicePixelRatio, const tEdCameraState &Camera,
    const tEdOverlayState &Overlay,
    const tEdReferenceMeshPayload *pReferenceMesh,
    uint32_t uiStuntTicks)
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
  Request.GraphicsSettings = m_GraphicsSettings;
  Request.ullGraphicsSettingsRevision = m_ullGraphicsSettingsRevision;
  Request.Camera = Camera;
  Request.bHasCamera = true;
  Request.Overlay = Overlay;
  Request.bHasOverlay = true;
  Request.uiStuntTicks = uiStuntTicks;
  // AD-16: copied here, so mutating the caller's arrays after enqueueing
  // cannot reach the worker.
  if (pReferenceMesh) {
    Request.ReferenceMesh = *pReferenceMesh;
    Request.bHasReferenceMesh = true;
  }
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
  Request.GraphicsSettings = m_GraphicsSettings;
  Request.ullGraphicsSettingsRevision = m_ullGraphicsSettingsRevision;
  const uint64_t ullRequestId = Request.Tag.ullRequestId;
  m_pThread->Enqueue(std::move(Request));
  return ullRequestId;
}

eRollerEdResult CEditorRenderService::ExtractGeometry(
    uint64_t ullDocumentId, uint64_t ullDocumentRevision,
    tEdGeometrySnapshot &SnapshotOut, std::string &sErrorOut)
{
  Q_ASSERT(QThread::currentThread() == thread());
  sErrorOut.clear();
  if (!IsDocumentRegistered(ullDocumentId)) {
    sErrorOut = "the exporting document is not registered with the worker";
    return ROLLER_ED_RESULT_INVALID_ARGUMENT;
  }

  const std::shared_ptr<tEdGeometryExtraction> Slot =
      std::make_shared<tEdGeometryExtraction>();

  tEdRenderRequest Request;
  Request.Tag.ullRequestId = CEditorRenderIds::NextRequestId();
  Request.Tag.ullDocumentId = ullDocumentId;
  Request.Tag.ullDocumentRevision = ullDocumentRevision;
  Request.eKind = eEdRenderCommandKind::EXTRACT_GEOMETRY;
  Request.Extraction = Slot;
  if (!m_pThread->Enqueue(std::move(Request))) {
    sErrorOut = "the render worker is not accepting work";
    return ROLLER_ED_RESULT_INVALID_STATE;
  }

  // Deliberately not a nested event loop: the worker publishes into the slot
  // rather than through a queued invocation, so blocking here cannot deadlock
  // against this thread's own result delivery. Every drop path completes the
  // slot, so this wait always terminates.
  {
    QMutexLocker Locker(&Slot->Mutex);
    while (!Slot->bComplete)
      Slot->Completed.wait(&Slot->Mutex);
    sErrorOut = Slot->sErrorText;
    if (Slot->eResult == ROLLER_ED_RESULT_OK)
      SnapshotOut = std::move(Slot->Snapshot);
    return Slot->eResult;
  }
}

void CEditorRenderService::PublishResult(tEdRenderResult Result)
{
  Q_ASSERT(QThread::currentThread() == thread());
  if (!IsDocumentRegistered(Result.Tag.ullDocumentId))
    return;
  emit FrameCompleted(Result);
}
