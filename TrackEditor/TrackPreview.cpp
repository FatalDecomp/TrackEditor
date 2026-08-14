#include "TrackPreview.h"
#include "EditorRenderService.h"
#include "MainWindow.h"
#include "Track.h"
#include "DisplaySettings.h"
#include "Texture.h"
#include "EditorObjImporter.h"
#include "EditorObjExporter.h"
#include "EditorGltfExporter.h"
#include "ExportWizard.h"
#include "Palette.h"
#include "qtemporarydir.h"
#include "qevent.h"
#include "qdir.h"
#include "qmessagebox.h"
#include "qfiledialog.h"
#include "qfile.h"
#include "qfileinfo.h"
#include "qpainter.h"
#include "qtimer.h"
#include "qtextstream.h"
#include <cstring>
#include <fstream>
//-------------------------------------------------------------------------------------------------
#if defined(_DEBUG) && defined(IS_WINDOWS)
#define new new(_CLIENT_BLOCK, __FILE__, __LINE__)
#endif
//-------------------------------------------------------------------------------------------------

class CTrackPreviewPrivate
{
public:
  CTrackPreviewPrivate() = default;

  CTrack m_track;
  CTrackHistory m_history;
};

//-------------------------------------------------------------------------------------------------

CTrackPreview::CTrackPreview(QWidget *pParent,
                             CEditorRenderService *pRenderService,
                             const QString &sTrackFile)
  // Members initialize in declaration order regardless of what this list says,
  // so the list follows the header rather than the other way round.
  : QWidget(pParent)
  , m_bUnsavedChanges(true)
  , m_iSelFrom(0)
  , m_iSelTo(0)
  , m_bToChecked(false)
  , m_sReferenceModelFile("")
  , m_dRefYaw(0.0)
  , m_dRefPitch(0.0)
  , m_dRefRoll(0.0)
  , m_iRefX(0)
  , m_iRefY(0)
  , m_iRefZ(0)
  , m_dRefScale(1.0)
  , m_uiShowModels(0)
  , m_uiShowFeatures(0)
  , m_carModel(eWhipModel::CAR_XZIZIN)
  , m_carAILine(eShapeSection::AILINE1)
  , m_bMillionPlus(false)
  , m_bAttachLast(false)
  , m_bAnimateStunts(false)
  , m_iScale(1)
  , m_bAlreadySaved(false)
  , m_sTrackFile(sTrackFile)
  , m_sLastCarTex("")
  , m_pRenderService(pRenderService)
  , m_ullDocumentId(CEditorRenderIds::NextDocumentId())
  , m_FrameState(m_ullDocumentId)
  , m_pResizeTimer(new QTimer(this))
  , m_pEditTimer(new QTimer(this))
  , m_pCameraRenderTimer(new QTimer(this))
  , m_pStuntTimer(new QTimer(this))
  , m_uiPendingStuntTicks(0)
  , m_ullCameraRequestId(0)
  , m_bCameraRenderPending(false)
  , m_bReloadPending(false)
{
  p = new CTrackPreviewPrivate;

  Q_ASSERT(m_pRenderService);
  m_pRenderService->RegisterDocument(m_ullDocumentId);
  connect(m_pRenderService, &CEditorRenderService::FrameCompleted,
          this, &CTrackPreview::OnRenderCompleted);

  m_pResizeTimer->setSingleShot(true);
  m_pResizeTimer->setInterval(100);
  connect(m_pResizeTimer, &QTimer::timeout,
          this, &CTrackPreview::QueueResizeRender);

  m_pEditTimer->setSingleShot(true);
  m_pEditTimer->setInterval(100);
  connect(m_pEditTimer, &QTimer::timeout,
          this, &CTrackPreview::QueueEditedTrackReload);

  m_pCameraRenderTimer->setSingleShot(true);
  m_pCameraRenderTimer->setInterval(16);
  connect(m_pCameraRenderTimer, &QTimer::timeout,
          this, &CTrackPreview::QueueCameraRender);

  // The legacy editor advanced moving stunts every 28 ms. Keep that cadence,
  // but send ticks through the render queue so only ROLLER's worker-owned
  // legacy scene is ever mutated.
  m_pStuntTimer->setTimerType(Qt::PreciseTimer);
  m_pStuntTimer->setInterval(28);
  connect(m_pStuntTimer, &QTimer::timeout,
          this, &CTrackPreview::QueueStuntTick);

  if (!sTrackFile.isEmpty()) {
    m_sDocumentAssetRoot = QFileInfo(sTrackFile).absolutePath();
    p->m_track.m_sTrackFileFolder = sTrackFile.left(sTrackFile.lastIndexOf(QDir::separator()) + 1).toLatin1().constData();
  }

  setFocusPolicy(Qt::StrongFocus);
  setAutoFillBackground(false);
}

//-------------------------------------------------------------------------------------------------

CTrackPreview::~CTrackPreview()
{
  m_FrameState.Invalidate();
  if (m_pRenderService)
    m_pRenderService->InvalidateDocument(m_ullDocumentId);
  if (p) {
    delete p;
    p = NULL;
  }
}

//-------------------------------------------------------------------------------------------------

void CTrackPreview::UpdateCameraPos(float fDeltaSeconds)
{
  if (!hasFocus()) {
    m_CameraController.ResetMouseTracking();
    return;
  }

  const tEditorCameraInput Input =
      g_pMainWindow->m_keyMapper.GetCameraInput();
  if (m_CameraController.Update(Input, fDeltaSeconds)) {
    ScheduleCameraRender();
  }
}

//-------------------------------------------------------------------------------------------------

bool CTrackPreview::LoadTrack(const QString &sFilename)
{
  m_sTrackFile = sFilename;
  m_sDocumentAssetRoot = QFileInfo(sFilename).absolutePath();
  const QByteArray EncodedFilename = QFile::encodeName(sFilename);
  bool bSuccess = p->m_track.LoadTrack(EncodedFilename.constData());
  if (bSuccess) {
    p->m_track.m_assets.LoadFromDocument(
        p->m_track.m_sTrackFileFolder,
        p->m_track.m_sTextureFile,
        p->m_track.m_sBuildingFile,
        g_pMainWindow->GetFatdataFolder().toStdString());
    if (!p->m_track.m_chunkAy.empty()) {
      m_CameraController.SetPosition(
          static_cast<float>(p->m_track.m_header.iHeaderUnk1) - 4000.0f,
          static_cast<float>(p->m_track.m_header.iHeaderUnk2),
          static_cast<float>(p->m_track.m_header.iFloorDepth) + 1600.0f);
    }
    p->m_history.Clear();
    SaveHistory(sFilename + " loaded", false);
    m_bUnsavedChanges = false;
    m_bAlreadySaved = true;
    QueueLoadAndRender();
  }
  return bSuccess;
}

//-------------------------------------------------------------------------------------------------

void CTrackPreview::DeleteEnvirFloor()
{
  update();
}

//-------------------------------------------------------------------------------------------------

void CTrackPreview::UpdateTrack()
{
  update();
}

//-------------------------------------------------------------------------------------------------

void CTrackPreview::ShowModels(uint32 uiShowModels)
{
  if (m_uiShowModels == uiShowModels)
    return;
  m_uiShowModels = uiShowModels;
  m_OverlaySettings.SetShowModels(uiShowModels);
  // An overlay change is a view change, not a document edit: it advances no
  // revision and needs no reload, so it rides the same coalesced render-only
  // path the camera uses.
  ScheduleCameraRender();
  update();
}

//-------------------------------------------------------------------------------------------------

void CTrackPreview::ShowFeatures(uint32 uiShowFeatures)
{
  if (m_uiShowFeatures == uiShowFeatures)
    return;
  m_uiShowFeatures = uiShowFeatures;
  m_OverlaySettings.SetShowFeatures(uiShowFeatures);
  // Feature visibility is overlay state only. The coalesced camera path
  // renders it immediately without advancing the geometry/document epoch.
  ScheduleCameraRender();
  update();
}

//-------------------------------------------------------------------------------------------------

void CTrackPreview::SetAnimateStunts(bool bAnimate)
{
  if (m_bAnimateStunts == bAnimate)
    return;

  m_bAnimateStunts = bAnimate;
  m_uiPendingStuntTicks = 0;
  if (m_bAnimateStunts)
    m_pStuntTimer->start();
  else
    m_pStuntTimer->stop();
}

//-------------------------------------------------------------------------------------------------

void CTrackPreview::UpdateGeometrySelection()
{
  // E3A-S3. The window keeps From <= To and collapses To onto From while the
  // "to" box is unchecked, so these are already an ordered range and a single
  // chunk selection has From == To. Like a display toggle this is a view
  // change, not a document edit: no revision bump, no reload.
  m_OverlaySettings.SetSelectionRange(m_iSelFrom, m_iSelTo);
  ScheduleCameraRender();
  update();
}

//-------------------------------------------------------------------------------------------------

void CTrackPreview::SaveHistory(const QString &sDescription, bool bDocumentEdit)
{
  if (bDocumentEdit)
    MarkDocumentEdited();
  p->m_history.Save(p->m_track, sDescription.toStdString(),
                    static_cast<size_t>(g_pMainWindow->m_preferences.iHistoryMaxSize));
}

//-------------------------------------------------------------------------------------------------

void CTrackPreview::Undo()
{
  if (p->m_history.Undo(p->m_track)) {
    p->m_track.m_assets.LoadFromDocument(
        p->m_track.m_sTrackFileFolder,
        p->m_track.m_sTextureFile,
        p->m_track.m_sBuildingFile,
        g_pMainWindow->GetFatdataFolder().toStdString());
    MarkDocumentEdited();
  }
}

//-------------------------------------------------------------------------------------------------

void CTrackPreview::Redo()
{
  if (p->m_history.Redo(p->m_track)) {
    p->m_track.m_assets.LoadFromDocument(
        p->m_track.m_sTrackFileFolder,
        p->m_track.m_sTextureFile,
        p->m_track.m_sBuildingFile,
        g_pMainWindow->GetFatdataFolder().toStdString());
    MarkDocumentEdited();
  }
}

//-------------------------------------------------------------------------------------------------

void CTrackPreview::UpdateReferenceModelPos(double dYaw, double dPitch, double dRoll,
                                            int iX, int iY, int iZ,
                                            double dScale)
{
  m_dRefYaw = dYaw;
  m_dRefPitch = dPitch;
  m_dRefRoll = dRoll;
  m_iRefX = iX;
  m_iRefY = iY;
  m_iRefZ = iZ;
  m_dRefScale = dScale;
  m_ReferenceMesh.SetTransform(dYaw, dPitch, dRoll, iX, iY, iZ, dScale);
  UpdateReferenceModelPos_Internal();
}

//-------------------------------------------------------------------------------------------------

void CTrackPreview::UpdateReferenceModelTexture()
{
  update();
}

//-------------------------------------------------------------------------------------------------

void CTrackPreview::UpdateReferenceModelWireframe(bool bWireframe)
{
  m_ReferenceMesh.SetWireframe(bWireframe);
  ScheduleReferenceMeshUpload();
}

//-------------------------------------------------------------------------------------------------

void CTrackPreview::ScheduleReferenceMeshUpload()
{
  // E3A-S7. Rebuild the queue payload from the mesh's current state and mark
  // it for the next render. Like every other display change this is a view
  // change, not a document edit: no revision bump, no serialize, no reload.
  const tEdReferenceMesh Mesh = m_ReferenceMesh.GetMesh();

  m_PendingReferenceMesh.Vertices = m_ReferenceMesh.Vertices();
  m_PendingReferenceMesh.Indices = m_ReferenceMesh.Indices();
  std::memcpy(m_PendingReferenceMesh.fPosition, Mesh.fPosition,
              sizeof(m_PendingReferenceMesh.fPosition));
  std::memcpy(m_PendingReferenceMesh.fRotation, Mesh.fRotation,
              sizeof(m_PendingReferenceMesh.fRotation));
  std::memcpy(m_PendingReferenceMesh.fScale, Mesh.fScale,
              sizeof(m_PendingReferenceMesh.fScale));
  m_PendingReferenceMesh.uiFlags = Mesh.uiFlags;
  m_bReferenceMeshDirty = true;
  ScheduleCameraRender();
  update();
}

//-------------------------------------------------------------------------------------------------

const tEdReferenceMeshPayload *CTrackPreview::TakePendingReferenceMesh()
{
  if (!m_bReferenceMeshDirty)
    return nullptr;
  m_bReferenceMeshDirty = false;
  return &m_PendingReferenceMesh;
}

//-------------------------------------------------------------------------------------------------

void CTrackPreview::UpdateCar(eWhipModel carModel, eShapeSection aiLine, bool bMillionPlus)
{
  if (m_carModel == carModel && m_carAILine == aiLine
      && m_bMillionPlus == bMillionPlus)
    return;
  m_carModel = carModel;
  m_carAILine = aiLine;
  m_bMillionPlus = bMillionPlus;
  // E3A-S6. The signature is unchanged; what it drives is now the facade's
  // overlay rather than a WhipLib shape. The car stands on m_iSelFrom, which
  // the overlay already carries, so nothing here needs the selection.
  //
  // Like every other display toggle this is a view change, not a document
  // edit: no revision bump, no serialize, no reload.
  m_OverlaySettings.SetTestCar(carModel, aiLine, bMillionPlus);
  ScheduleCameraRender();
  update();
}

//-------------------------------------------------------------------------------------------------

void CTrackPreview::AttachLast(bool bAttachLast)
{
  m_bAttachLast = bAttachLast;
  UpdateTrack();
}

//-------------------------------------------------------------------------------------------------

void CTrackPreview::OpenReferenceModel()
{
  const QString sFile = QFileDialog::getOpenFileName(
      this, "Open Reference Model", g_pMainWindow->m_sLastTrackFilesFolder,
      "Wavefront OBJ (*.obj);;All Files (*.*)");
  if (sFile.isEmpty())
    return;

  // Since E4-S6 the importer returns the file's raw units and axes, and the
  // interchange conversion happens below.
  tEditorImportedMesh Imported;
  if (!EditorObjImporter::ImportObj(sFile.toStdString(), Imported)) {
    QMessageBox::warning(this, "Reference Model",
                         "Could not read " + sFile + " as a Wavefront OBJ.");
    return;
  }

  // Convert to the facade's AD-13 vertex. Only position and normal survive:
  // the legacy editor overwrote every reference-model UV with a flat colour,
  // and roller-core draws it that way too, so UVs would be discarded anyway.
  //
  // E4-S6. The reference mesh is ROLLER world space (AD-13 inherits ADR 0003:
  // +Z up, legacy track units) and an OBJ is +Y up in whatever unit it was
  // authored in. Running the file through the exact inverse of the export
  // conversion is what makes a track this editor exported re-import lined up
  // with itself.
  std::vector<tEdReferenceVertex> Vertices(Imported.VertexCount());
  for (size_t i = 0; i < Imported.VertexCount(); ++i) {
    tEdReferenceVertex &Target = Vertices[i];
    CEditorExportConventions::ImportPosition(&Imported.Positions[i * 3],
                                             Target.fPosition);
    CEditorExportConventions::ImportDirection(&Imported.Normals[i * 3],
                                              Target.fNormal);
    Target.fUV[0] = 0.0f;
    Target.fUV[1] = 0.0f;
  }
  const std::vector<uint32_t> &Indices = Imported.Indices;

  if (Vertices.empty() || Indices.size() % 3 != 0) {
    QMessageBox::warning(this, "Reference Model",
                         sFile + " is not a triangle list.");
    return;
  }

  m_sReferenceModelFile = sFile;
  m_ReferenceMesh.SetGeometry(Vertices.data(), Vertices.size(),
                              Indices.data(), Indices.size());
  ScheduleReferenceMeshUpload();
  emit ReferenceModelChanged();
}

//-------------------------------------------------------------------------------------------------

void CTrackPreview::paintEvent(QPaintEvent *pEvent)
{
  (void)pEvent;
  QPainter Painter(this);
  Painter.fillRect(rect(), QColor(8, 12, 18));

  if (!m_FrameState.GetImage().isNull()) {
    Painter.drawImage(rect(), m_FrameState.GetImage());
  } else {
    Painter.setPen(QColor(190, 198, 210));
    Painter.drawText(rect(), Qt::AlignCenter,
                     m_FrameState.GetErrorText().empty()
                         ? QString("No rendered frame")
                         : QString::fromStdString(m_FrameState.GetErrorText()));
  }

  if (m_FrameState.GetDisplayState()
      == eEdFrameDisplayState::STALE_AFTER_LOAD_FAILURE) {
    const QRect Banner(0, 0, width(), 44);
    Painter.fillRect(Banner, QColor(150, 28, 28, 225));
    Painter.setPen(Qt::white);
    const QString sMessage = m_FrameState.GetErrorText().empty()
        ? QString("Stale frame: the latest track load failed")
        : QString("Stale frame: %1")
              .arg(QString::fromStdString(m_FrameState.GetErrorText()));
    Painter.drawText(Banner.adjusted(12, 0, -12, 0),
                     Qt::AlignVCenter | Qt::AlignLeft, sMessage);
  }
}

//-------------------------------------------------------------------------------------------------

void CTrackPreview::resizeEvent(QResizeEvent *pEvent)
{
  QWidget::resizeEvent(pEvent);
  m_pResizeTimer->start();
}

//-------------------------------------------------------------------------------------------------

void CTrackPreview::showEvent(QShowEvent *pEvent)
{
  QWidget::showEvent(pEvent);
  if (!m_sTrackFile.isEmpty() && m_FrameState.GetLatestRequestId() == 0)
    QueueLoadAndRender();
}

//-------------------------------------------------------------------------------------------------

QSize CTrackPreview::DevicePixelSize() const
{
  const qreal dDevicePixelRatio = devicePixelRatioF();
  return QSize(qMax(1, qRound(width() * dDevicePixelRatio)),
               qMax(1, qRound(height() * dDevicePixelRatio)));
}

//-------------------------------------------------------------------------------------------------

void CTrackPreview::QueueLoadAndRender()
{
  if (!m_pRenderService || m_sTrackFile.isEmpty())
    return;

  // A load reconstructs ROLLER's stunt state from the document, so ticks
  // accumulated for the previous installed scene must not cross the reload.
  m_uiPendingStuntTicks = 0;

  if (p->m_track.m_chunkAy.empty()) {
    const uint64_t ullRequestId = m_pRenderService->EnqueueUnload(
        m_ullDocumentId, m_FrameState.GetDocumentRevision());
    if (ullRequestId != 0) {
      m_pCameraRenderTimer->stop();
      m_bCameraRenderPending = false;
      m_bReloadPending = true;
      m_FrameState.BeginRequest(ullRequestId);
    }
    return;
  }

  // GetTrackData serializes the current model through CTrack::WriteToVector.
  // The worker owns this snapshot before materializing its temporary .TRK.
  std::vector<uint8> TrackData;
  p->m_track.GetTrackData(TrackData);
  const std::vector<uint8_t> SerializedTrackData(
      TrackData.begin(), TrackData.end());
  const uint64_t ullRequestId =
      m_pRenderService->EnqueueSerializedLoadAndRender(
      m_ullDocumentId, m_FrameState.GetDocumentRevision(),
          SerializedTrackData, m_sDocumentAssetRoot,
          DevicePixelSize(),
          devicePixelRatioF(), m_CameraController.GetCameraState(),
          m_OverlaySettings.GetOverlayState(),
          g_pMainWindow->GetFatdataFolder());
  if (ullRequestId != 0) {
    m_pCameraRenderTimer->stop();
    m_bCameraRenderPending = false;
    m_bReloadPending = true;
    m_FrameState.BeginRequest(ullRequestId);
  }
}

//-------------------------------------------------------------------------------------------------

void CTrackPreview::QueueEditedTrackReload()
{
  // The tab may have become hidden while the debounce timer was active. Only
  // the visible document may replace the process-wide worker scene.
  if (isVisible())
    QueueLoadAndRender();
}

//-------------------------------------------------------------------------------------------------

void CTrackPreview::QueueResizeRender()
{
  if (!m_pRenderService || m_bReloadPending)
    return;

  if (m_FrameState.GetDisplayState() == eEdFrameDisplayState::CURRENT) {
    const uint32_t uiStuntTicks = m_uiPendingStuntTicks;
    const uint64_t ullRequestId = m_pRenderService->EnqueueRender(
        m_ullDocumentId, m_FrameState.GetDocumentRevision(),
        m_FrameState.GetInstalledGeometryEpoch(), DevicePixelSize(),
        devicePixelRatioF(), m_CameraController.GetCameraState(),
        m_OverlaySettings.GetOverlayState(), TakePendingReferenceMesh(),
        uiStuntTicks);
    if (ullRequestId != 0) {
      m_uiPendingStuntTicks = 0;
      m_FrameState.BeginRequest(ullRequestId);
    }
  } else if (m_FrameState.GetDisplayState()
                 == eEdFrameDisplayState::PLACEHOLDER
             && m_FrameState.GetLatestRequestId() == 0) {
    QueueLoadAndRender();
  }
}

//-------------------------------------------------------------------------------------------------

void CTrackPreview::ScheduleCameraRender()
{
  m_bCameraRenderPending = true;
  ArmCameraRenderTimer();
}

//-------------------------------------------------------------------------------------------------

void CTrackPreview::ArmCameraRenderTimer()
{
  if (m_bCameraRenderPending && m_ullCameraRequestId == 0
      && !m_bReloadPending && isVisible()
      && m_FrameState.GetDisplayState() == eEdFrameDisplayState::CURRENT
      && !m_pCameraRenderTimer->isActive()) {
    m_pCameraRenderTimer->start();
  }
}

//-------------------------------------------------------------------------------------------------

void CTrackPreview::QueueCameraRender()
{
  if (!m_bCameraRenderPending || m_ullCameraRequestId != 0
      || m_bReloadPending || !isVisible()
      || m_FrameState.GetDisplayState() != eEdFrameDisplayState::CURRENT) {
    return;
  }

  const uint32_t uiStuntTicks = m_uiPendingStuntTicks;
  const uint64_t ullRequestId = m_pRenderService->EnqueueRender(
      m_ullDocumentId, m_FrameState.GetDocumentRevision(),
      m_FrameState.GetInstalledGeometryEpoch(), DevicePixelSize(),
      devicePixelRatioF(), m_CameraController.GetCameraState(),
      m_OverlaySettings.GetOverlayState(), TakePendingReferenceMesh(),
      uiStuntTicks);
  if (ullRequestId != 0) {
    m_uiPendingStuntTicks = 0;
    m_bCameraRenderPending = false;
    m_ullCameraRequestId = ullRequestId;
    m_FrameState.BeginRequest(ullRequestId);
  }
}

//-------------------------------------------------------------------------------------------------

void CTrackPreview::QueueStuntTick()
{
  if (!m_bAnimateStunts || !isVisible() || m_bReloadPending
      || m_FrameState.GetDisplayState() != eEdFrameDisplayState::CURRENT) {
    return;
  }

  // If rendering falls briefly behind, catch up by applying several fixed
  // ticks to the next frame. Cap the backlog so a stalled renderer cannot
  // later monopolize the worker trying to replay minutes of animation.
  if (m_uiPendingStuntTicks < 8u)
    ++m_uiPendingStuntTicks;
  m_bCameraRenderPending = true;
  if (m_ullCameraRequestId == 0) {
    m_pCameraRenderTimer->stop();
    QueueCameraRender();
  }
}

//-------------------------------------------------------------------------------------------------

void CTrackPreview::OnRenderCompleted(const tEdRenderResult &Result)
{
  if (Result.Tag.ullDocumentId != m_ullDocumentId)
    return;
  if (Result.Tag.ullRequestId == m_ullCameraRequestId)
    m_ullCameraRequestId = 0;
  if (!m_FrameState.ApplyResult(Result)) {
    ArmCameraRenderTimer();
    return;
  }

  if (Result.bHasTowerSnapshot)
    m_Towers = Result.Towers;

  m_bReloadPending = Result.Tag.eResult != ROLLER_ED_RESULT_OK;

  update();
  emit FrameStateChanged();
  if (m_uiPendingStuntTicks != 0 && m_ullCameraRequestId == 0) {
    m_pCameraRenderTimer->stop();
    QueueCameraRender();
  } else {
    ArmCameraRenderTimer();
  }
}

//-------------------------------------------------------------------------------------------------

bool CTrackPreview::ViewFromTower(int iChunkId)
{
  for (const tEdTowerInfo &Tower : m_Towers) {
    if (Tower.uiChunkId != static_cast<uint32_t>(iChunkId))
      continue;

    float fYawDegrees = 0.0f;
    float fPitchDegrees = 0.0f;
    if (!CEditorCameraController::CalculateLookAtOrientation(
            Tower.fWorldPosition, Tower.fAnchorPosition,
            fYawDegrees, fPitchDegrees)) {
      return false;
    }

    m_CameraController.SetPosition(
        Tower.fWorldPosition[0], Tower.fWorldPosition[1],
        Tower.fWorldPosition[2]);
    m_CameraController.SetOrientation(fYawDegrees, fPitchDegrees);
    m_CameraController.ResetMouseTracking();
    ScheduleCameraRender();
    return true;
  }
  return false;
}

//-------------------------------------------------------------------------------------------------

void CTrackPreview::Activate()
{
  m_pEditTimer->stop();
  QueueLoadAndRender();
}

// The selected FATDATA is process-wide editor configuration, not document
// state. Refresh the CPU-side banks for every tab, and reload only the active
// tab because ROLLER's worker owns one scene at a time.
void CTrackPreview::UpdateFatdataFolder()
{
  p->m_track.m_assets.LoadFromDocument(
      p->m_track.m_sTrackFileFolder,
      p->m_track.m_sTextureFile,
      p->m_track.m_sBuildingFile,
      g_pMainWindow->GetFatdataFolder().toStdString());
  if (isVisible())
    QueueLoadAndRender();
}

//-------------------------------------------------------------------------------------------------

void CTrackPreview::RefreshGraphicsSettings()
{
  ScheduleCameraRender();
}

//-------------------------------------------------------------------------------------------------

void CTrackPreview::MarkDocumentEdited()
{
  m_FrameState.MarkDocumentEdited();
  m_bReloadPending = true;
  m_pResizeTimer->stop();
  if (isVisible())
    m_pEditTimer->start();
}

//-------------------------------------------------------------------------------------------------

CTrack *CTrackPreview::GetTrack()
{
  return &p->m_track;
}

//-------------------------------------------------------------------------------------------------

bool CTrackPreview::SaveChangesAndContinue()
{
  if (!m_bUnsavedChanges)
    return true;

  //init
  QString sTrackName = m_sTrackFile.right(m_sTrackFile.size() - m_sTrackFile.lastIndexOf(QDir::separator()) - 1);
  QMessageBox saveDiscardCancelBox(QMessageBox::Warning, "Unsaved Changes",
                                   "There are unsaved changes to " + sTrackName + ". Save them ? ",
                                   QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
                                   this);
  int iButton = saveDiscardCancelBox.exec();

  //cancel
  if (iButton == QMessageBox::Cancel || iButton == QMessageBox::NoButton)
    return false;

  //save
  QString sFilename = m_sTrackFile;
  if (iButton == QMessageBox::Save) {
    if (sFilename.isEmpty()) {
      sFilename = QDir::toNativeSeparators(QFileDialog::getSaveFileName(
        this, "Save Track As", p->m_track.m_sTrackFileFolder.c_str(), "Track Files (*.TRK)"));
    }
    if (!SaveTrack_Internal(sFilename))
      return false;
    g_pMainWindow->m_sLastTrackFilesFolder = sFilename.left(sFilename.lastIndexOf(QDir::separator()));
  }

  m_sTrackFile = sFilename;
  m_bUnsavedChanges = false;

  return true;
}

//-------------------------------------------------------------------------------------------------

bool CTrackPreview::SaveTrack()
{
  if (m_bAlreadySaved) {
    m_bUnsavedChanges = !SaveTrack_Internal(m_sTrackFile);
    if (!m_bUnsavedChanges)
      QueueLoadAndRender();
    g_pMainWindow->UpdateWindow();
    return true;
  } else {
    return SaveTrackAs();
  }
}

//-------------------------------------------------------------------------------------------------

bool CTrackPreview::SaveTrackAs()
{
  //save track
  QString sFilename = QDir::toNativeSeparators(QFileDialog::getSaveFileName(
    this, "Save Track As", p->m_track.m_sTrackFileFolder.c_str(), "Track Files (*.TRK)"));
  if (!SaveTrack_Internal(sFilename))
    return false;

  //save successful, update app
  g_pMainWindow->m_sLastTrackFilesFolder = sFilename.left(sFilename.lastIndexOf(QDir::separator()));
  m_sTrackFile = sFilename;
  m_bUnsavedChanges = false;
  m_bAlreadySaved = true;
  QueueLoadAndRender();
  g_pMainWindow->UpdateWindow();
  return true;
}

//-------------------------------------------------------------------------------------------------

bool CTrackPreview::ExtractCanonicalGeometry(tEdGeometrySnapshot &SnapshotOut)
{
  if (!m_pRenderService) {
    QMessageBox::warning(this, "Export Track",
                         "The render worker is unavailable.");
    return false;
  }

  // An edit debounced but not yet queued would leave the worker scene one
  // revision behind the model the user is looking at. Queueing the reload now
  // puts it ahead of the extraction in the worker's FIFO, so the export sees
  // the current document without a second load.
  if (m_pEditTimer->isActive()) {
    m_pEditTimer->stop();
    QueueEditedTrackReload();
  }

  std::string sExtractError;
  const eRollerEdResult eResult = m_pRenderService->ExtractGeometry(
      m_ullDocumentId, m_FrameState.GetDocumentRevision(), SnapshotOut,
      sExtractError);
  if (eResult != ROLLER_ED_RESULT_OK) {
    QString sMessage = "Could not read the track geometry from ROLLER.";
    if (!sExtractError.empty())
      sMessage += QString("\n\n") + QString::fromStdString(sExtractError);
    QMessageBox::warning(this, "Export Track", sMessage);
    return false;
  }
  return true;
}

//-------------------------------------------------------------------------------------------------

std::vector<tEdExportPaletteEntry> CTrackPreview::BuildExportPalette() const
{
  // Flat palette colours resolve against the document's own palette, which
  // track-assets already owns for the texture export.
  std::vector<tEdExportPaletteEntry> Palette;
  const CPalette *pPalette = p->m_track.m_assets.GetPalette();
  if (!pPalette)
    return Palette;
  Palette.resize(PALETTE_SIZE);
  for (int i = 0; i < PALETTE_SIZE; ++i) {
    Palette[i].byRed = pPalette->m_paletteAy[i].r;
    Palette[i].byGreen = pPalette->m_paletteAy[i].g;
    Palette[i].byBlue = pPalette->m_paletteAy[i].b;
  }
  return Palette;
}

//-------------------------------------------------------------------------------------------------

static tEdExportGeometry ViewOfSnapshot(const tEdGeometrySnapshot &Snapshot)
{
  tEdExportGeometry Geometry;
  Geometry.pVertices = Snapshot.Vertices.data();
  Geometry.uiVertexCount = static_cast<uint32_t>(Snapshot.Vertices.size());
  Geometry.puiIndices = Snapshot.Indices.data();
  Geometry.uiIndexCount = static_cast<uint32_t>(Snapshot.Indices.size());
  Geometry.pPrimitives = Snapshot.Primitives.data();
  Geometry.uiPrimitiveCount =
      static_cast<uint32_t>(Snapshot.Primitives.size());
  Geometry.pMaterials = Snapshot.Materials.data();
  Geometry.uiMaterialCount = static_cast<uint32_t>(Snapshot.Materials.size());
  return Geometry;
}

//-------------------------------------------------------------------------------------------------

bool CTrackPreview::ExportObj_Internal(const QString &sFolder,
                                       const QString &sName,
                                       const QString &sFilename,
                                       bool bExportScenery,
                                       bool bSeparateSections,
                                       bool bSeparateBackFaces)
{
  tEdGeometrySnapshot Snapshot;
  if (!ExtractCanonicalGeometry(Snapshot))
    return false;

  const tEdExportGeometry Geometry = ViewOfSnapshot(Snapshot);
  const std::vector<tEdExportPaletteEntry> Palette = BuildExportPalette();

  const QString sMtlFile = QDir(sFolder).filePath(sName + ".mtl");
  tEdObjExportOptions Options;
  Options.bExportScenery = bExportScenery;
  Options.bSeparateSections = bSeparateSections;
  Options.bSeparateBackFaces = bSeparateBackFaces;
  Options.sBaseName = sName.toStdString();
  Options.sMtlFileName = (sName + ".mtl").toStdString();

  std::string sWriteError;
  if (!CEditorObjExporter::ExportToFiles(
          Geometry, Options, Palette.empty() ? nullptr : Palette.data(),
          static_cast<uint32_t>(Palette.size()),
          QFile::encodeName(sFilename).constData(),
          QFile::encodeName(sMtlFile).constData(), sWriteError)) {
    QString sMessage = "Could not write the exported model.";
    if (!sWriteError.empty())
      sMessage += QString("\n\n") + QString::fromStdString(sWriteError);
    QMessageBox::warning(this, "Export Track", sMessage);
    return false;
  }
  return true;
}

//-------------------------------------------------------------------------------------------------

bool CTrackPreview::ExportGltf_Internal(const QString &sFolder,
                                        const QString &sName,
                                        const QString &sFilename,
                                        bool bExportScenery,
                                        bool bSeparateSections,
                                        bool bSeparateBackFaces)
{
  // The container follows the name the user chose: .glb is one self-contained
  // file, anything else is JSON beside its buffer and the atlas PNGs. Export()
  // has already applied the selected filter's suffix, so a name the user typed
  // bare still resolves to the container they picked.
  const bool bBinary =
      CEditorExportFormats::IsBinaryGltf(sFilename.toStdString());

  // A .glb embeds its images, so the PNGs are a build artifact rather than
  // part of the deliverable and must not be left beside it. A .gltf points at
  // them, so they go next to the model and stay.
  QTemporaryDir TemporaryTextures;
  QString sTextureFolder = sFolder;
  if (bBinary) {
    if (!TemporaryTextures.isValid()) {
      QMessageBox::warning(this, "Export Track",
                           "Could not stage the track textures.");
      return false;
    }
    sTextureFolder = TemporaryTextures.path();
  }
  const QString sTexFile = QDir(sTextureFolder).filePath(sName + ".png");
  const QString sSignTexFile =
      QDir(sTextureFolder).filePath(sName + "_BLD.png");
  if (!p->m_track.m_assets.ExportTextures(
          QFile::encodeName(sTexFile).constData(),
          QFile::encodeName(sSignTexFile).constData())) {
    QMessageBox::warning(this, "Export Track",
                         "Could not write the track textures.");
    return false;
  }

  tEdGeometrySnapshot Snapshot;
  if (!ExtractCanonicalGeometry(Snapshot))
    return false;

  const tEdExportGeometry Geometry = ViewOfSnapshot(Snapshot);
  const std::vector<tEdExportPaletteEntry> Palette = BuildExportPalette();

  tEdGltfExportOptions Options;
  Options.bExportScenery = bExportScenery;
  Options.bSeparateSections = bSeparateSections;
  Options.bSeparateBackFaces = bSeparateBackFaces;
  Options.bBinary = bBinary;
  Options.sBaseName = sName.toStdString();
  Options.sBufferUri = (sName + ".bin").toStdString();

  const uint32_t auiTextureSets[2] = { ROLLER_ED_TEXTURE_SET_TRACK,
                                       ROLLER_ED_TEXTURE_SET_BUILDING_SIGN };
  const QString aTexturePaths[2] = { sTexFile, sSignTexFile };
  for (int i = 0; i < 2; ++i) {
    tEdGltfTextureSource Source;
    Source.uiTextureSet = auiTextureSets[i];
    if (bBinary) {
      QFile Png(aTexturePaths[i]);
      if (!Png.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "Export Track",
                             "Could not read the staged track textures.");
        return false;
      }
      const QByteArray Bytes = Png.readAll();
      Source.PngBytes.assign(
          reinterpret_cast<const uint8_t *>(Bytes.constData()),
          reinterpret_cast<const uint8_t *>(Bytes.constData())
              + Bytes.size());
    } else {
      Source.sUri = CEditorExportConventions::TextureFileName(
          Options.sBaseName, auiTextureSets[i]);
    }
    Options.Textures.push_back(std::move(Source));
  }

  const QString sBinFile = QDir(sFolder).filePath(sName + ".bin");
  std::string sWriteError;
  if (!CEditorGltfExporter::ExportToFiles(
          Geometry, Options, Palette.empty() ? nullptr : Palette.data(),
          static_cast<uint32_t>(Palette.size()),
          QFile::encodeName(sFilename).constData(),
          bBinary ? std::string()
                  : std::string(QFile::encodeName(sBinFile).constData()),
          sWriteError)) {
    QString sMessage = "Could not write the exported model.";
    if (!sWriteError.empty())
      sMessage += QString("\n\n") + QString::fromStdString(sWriteError);
    QMessageBox::warning(this, "Export Track", sMessage);
    return false;
  }
  return true;
}

//-------------------------------------------------------------------------------------------------

bool CTrackPreview::Export(eExportType exportType)
{
  if (!CanExport())
    return false;

  //get export settings
  CExportWizard exportWizard(this);
  if (!exportWizard.exec())
    return false;

  //save track
  const tEdExportFormat &Format = CEditorExportFormats::For(exportType);
  QString sSelectedFilter;
  QString sFilename = QDir::toNativeSeparators(QFileDialog::getSaveFileName(
    this, "Export Track As", p->m_track.m_sTrackFileFolder.c_str(),
    Format.szDialogFilter, &sSelectedFilter));
  if (sFilename.isEmpty())
    return false;

  // E4-S5. Only the Windows native dialog appends the selected filter's
  // extension; Qt's own dialog does not, and the static getSaveFileName sets
  // no default suffix. A bare name would otherwise reach a glTF export as
  // "no .glb suffix", which silently writes JSON when the user asked for the
  // self-contained container.
  sFilename = QString::fromStdString(CEditorExportFormats::ApplyDefaultSuffix(
      sFilename.toStdString(), sSelectedFilter.toStdString(), exportType));

  QString sFolder = sFilename.left(sFilename.lastIndexOf(QDir::separator()));
  QString sName = sFilename.right(sFilename.size() - sFilename.lastIndexOf(QDir::separator()) - 1);
  sName = sName.left(sName.lastIndexOf('.'));

  // E4-S2. glTF owns where its textures land, because a .glb embeds them and
  // must not leave loose PNGs behind.
  if (exportType == eExportType::EXPORT_GLTF) {
    if (!ExportGltf_Internal(sFolder, sName, sFilename,
                             exportWizard.m_bExportSigns,
                             exportWizard.m_bExportSeparate,
                             exportWizard.m_bExportBacks)) {
      return false;
    }
    g_pMainWindow->m_sLastTrackFilesFolder = sFolder;
    return true;
  }

  // E4-S1. OBJ writes its atlas PNGs beside the model and references them.
  QString sTexFile = QDir(sFolder).filePath(sName + ".png");
  QString sSignTexFile = QDir(sFolder).filePath(sName + "_BLD.png");
  if (!p->m_track.m_assets.ExportTextures(
          QFile::encodeName(sTexFile).constData(),
          QFile::encodeName(sSignTexFile).constData())) {
    return false;
  }

  if (!ExportObj_Internal(sFolder, sName, sFilename,
                          exportWizard.m_bExportSigns,
                          exportWizard.m_bExportSeparate,
                          exportWizard.m_bExportBacks)) {
    return false;
  }
  g_pMainWindow->m_sLastTrackFilesFolder = sFolder;
  return true;
}

//-------------------------------------------------------------------------------------------------

bool CTrackPreview::ExportToFolder(eExportType exportType,
                                   const QString &sFolder,
                                   const QString &sName)
{
  if (!CanExport() || sFolder.isEmpty() || sName.isEmpty())
    return false;
  if (!QDir().mkpath(sFolder))
    return false;

  // Batch glTF is deliberately the self-contained container: exporting every
  // retail track already creates many files, and a .glb keeps each result to
  // one portable model. OBJ retains its conventional MTL and atlas sidecars.
  const QString sFilename = QDir(sFolder).filePath(
      sName + (exportType == eExportType::EXPORT_GLTF ? ".glb" : ".obj"));

  if (exportType == eExportType::EXPORT_GLTF) {
    return ExportGltf_Internal(sFolder, sName, sFilename,
                               true, true, true);
  }

  const QString sTexFile = QDir(sFolder).filePath(sName + ".png");
  const QString sSignTexFile = QDir(sFolder).filePath(sName + "_BLD.png");
  if (!p->m_track.m_assets.ExportTextures(
          QFile::encodeName(sTexFile).constData(),
          QFile::encodeName(sSignTexFile).constData())) {
    return false;
  }
  return ExportObj_Internal(sFolder, sName, sFilename, true, true, true);
}

//-------------------------------------------------------------------------------------------------

QString CTrackPreview::GetLastRenderError() const
{
  return QString::fromStdString(m_FrameState.GetErrorText());
}

//-------------------------------------------------------------------------------------------------

QString CTrackPreview::GetTitle(bool bFullPath)
{
  QString sTitle = m_sTrackFile;
  if (!bFullPath)
    sTitle = m_sTrackFile.right(m_sTrackFile.size() - m_sTrackFile.lastIndexOf(QDir::separator()) - 1);
  if (m_bUnsavedChanges)
    sTitle = QString("*") + sTitle;
  return sTitle;
}

//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------

void CTrackPreview::mousePressEvent(QMouseEvent *pEvent)
{
  g_pMainWindow->m_keyMapper.QtMousePressEvent(pEvent);
  setFocus();
}

//-------------------------------------------------------------------------------------------------

void CTrackPreview::mouseReleaseEvent(QMouseEvent *pEvent)
{
  g_pMainWindow->m_keyMapper.QtMouseReleaseEvent(pEvent);
}

//-------------------------------------------------------------------------------------------------

void CTrackPreview::mouseMoveEvent(QMouseEvent *pEvent)
{
  (void)(pEvent);
  setFocus();
}

//-------------------------------------------------------------------------------------------------

void CTrackPreview::keyPressEvent(QKeyEvent *pEvent)
{
  g_pMainWindow->m_keyMapper.QtKeyPressEvent(pEvent);
}

//-------------------------------------------------------------------------------------------------

void CTrackPreview::keyReleaseEvent(QKeyEvent *pEvent)
{
  g_pMainWindow->m_keyMapper.QtKeyReleaseEvent(pEvent);
}

//-------------------------------------------------------------------------------------------------

bool CTrackPreview::SaveTrack_Internal(const QString &sFilename)
{
  if (sFilename.isEmpty())
    return false;

  std::vector<uint8> trackData;
  std::vector<uint8> mangledData;
  p->m_track.GetTrackData(trackData);

  std::vector<uint8> *pOutData;
  //if (bIsMangled) {
  //  MangleFile(trackData, mangledData);
  //  pOutData = &mangledData;
  //} else {
  pOutData = &trackData;
//}

  QFile file(sFilename);
  file.resize(0);
  if (file.open(QIODevice::ReadWrite)) {
    QTextStream stream(&file);
    for (int i = 0; (size_t)i < pOutData->size(); ++i) {
      stream << (char)((*pOutData)[i]);
    }
    file.close();
  }

  return true;
}

//-------------------------------------------------------------------------------------------------

void CTrackPreview::UpdateReferenceModelPos_Internal()
{
  // E3A-S7. The transform lives in the mesh payload rather than in a matrix
  // the editor builds, so moving the model is the same one-upload path as
  // loading it.
  ScheduleReferenceMeshUpload();
}

//-------------------------------------------------------------------------------------------------
