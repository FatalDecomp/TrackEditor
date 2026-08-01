#include "TrackPreview.h"
#include "EditorRenderService.h"
#include "MainWindow.h"
#include "Track.h"
#include "DisplaySettings.h"
#include "ShapeData.h"
#include "ShapeFactory.h"
#include "Texture.h"
#if TRACKEDITOR_ENABLE_FBX
#include "FBXExporter.h"
#endif
#include "ObjExporter.h"
#include "CarHelpers.h"
#include "ExportWizard.h"
#include "qevent.h"
#include "qdir.h"
#include "qmessagebox.h"
#include "qfiledialog.h"
#include "qfile.h"
#include "qfileinfo.h"
#include "qpainter.h"
#include "qtimer.h"
#include "qtextstream.h"
#include <fstream>
//-------------------------------------------------------------------------------------------------
#if defined(_DEBUG) && defined(IS_WINDOWS)
#define new new(_CLIENT_BLOCK, __FILE__, __LINE__)
#endif
//-------------------------------------------------------------------------------------------------

class CTrackPreviewPrivate
{
public:
  CTrackPreviewPrivate()
    : m_pShader(NULL)
  {
  }

  CShader *m_pShader;
  CTrack m_track;
  CHistoryAy m_historyAy;
};

//-------------------------------------------------------------------------------------------------

CTrackPreview::CTrackPreview(QWidget *pParent,
                             CEditorRenderService *pRenderService,
                             const QString &sTrackFile)
  : QWidget(pParent)
  , m_uiShowModels(0)
  , m_carModel(eWhipModel::CAR_XZIZIN)
  , m_carAILine(eShapeSection::AILINE1)
  , m_bMillionPlus(false)
  , m_bAttachLast(false)
  , m_iScale(1)
  , m_bUnsavedChanges(true)
  , m_bAlreadySaved(false)
  , m_sTrackFile(sTrackFile)
  , m_iSelFrom(0)
  , m_iSelTo(0)
  , m_bToChecked(false)
  , m_sLastCarTex("")
  , m_iHistoryIndex(0)
  , m_sReferenceModelFile("")
  , m_dRefYaw(0.0)
  , m_dRefPitch(0.0)
  , m_dRefRoll(0.0)
  , m_iRefX(0)
  , m_iRefY(0)
  , m_iRefZ(0)
  , m_dRefScale(1.0)
  , m_pRenderService(pRenderService)
  , m_ullDocumentId(CEditorRenderIds::NextDocumentId())
  , m_FrameState(m_ullDocumentId)
  , m_Camera{}
  , m_pResizeTimer(new QTimer(this))
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

  m_Camera.uiStructSize = sizeof(m_Camera);
  m_Camera.uiVersion = ROLLER_ED_CAMERA_STATE_VERSION;
  m_Camera.fPosition[0] = -4000.0f;
  m_Camera.fPosition[1] = 0.0f;
  m_Camera.fPosition[2] = 1600.0f;
  m_Camera.fYawDegrees = 0.0f;
  m_Camera.fPitchDegrees = -25.0f;

  if (!sTrackFile.isEmpty()) {
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

void CTrackPreview::UpdateCameraPos()
{
  // Camera input is wired to roller-core by E3-S3. E3-S1 deliberately does
  // not render from this UI timer; frames are produced only by the worker.
}

//-------------------------------------------------------------------------------------------------

bool CTrackPreview::LoadTrack(const QString &sFilename)
{
  m_sTrackFile = sFilename;
  const QByteArray EncodedFilename = QFile::encodeName(sFilename);
  bool bSuccess = p->m_track.LoadTrack(EncodedFilename.constData());
  if (bSuccess) {
    p->m_track.GenerateTrackMath();
    if (!p->m_track.m_chunkAy.empty()) {
      const glm::vec3 &Center = p->m_track.m_chunkAy.front().math.center;
      m_Camera.fPosition[0] = Center.x - 4000.0f;
      m_Camera.fPosition[1] = Center.y;
      m_Camera.fPosition[2] = Center.z + 1600.0f;
    }
    p->m_historyAy.clear();
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

void CTrackPreview::UpdateTrack(bool bUpdatingStunt)
{
  (void)bUpdatingStunt;
  update();
}

//-------------------------------------------------------------------------------------------------

void CTrackPreview::ShowModels(uint32 uiShowModels)
{
  m_uiShowModels = uiShowModels;
  update();
}

//-------------------------------------------------------------------------------------------------

void CTrackPreview::UpdateGeometrySelection()
{
  update();
}

//-------------------------------------------------------------------------------------------------

void CTrackPreview::SaveHistory(const QString &sDescription, bool bDocumentEdit)
{
  if (bDocumentEdit)
    MarkDocumentEdited();
  tTrackHistory history;
  history.sDescription = sDescription.toLatin1().constData();
  p->m_track.GetTrackData(history.byteAy);

  while (m_iHistoryIndex < (int)p->m_historyAy.size() - 1) {
    p->m_historyAy.pop_back();
  }

  p->m_historyAy.push_back(history);

  while ((int)p->m_historyAy.size() > g_pMainWindow->m_preferences.iHistoryMaxSize) {
    p->m_historyAy.erase(p->m_historyAy.begin());
  }

  m_iHistoryIndex = (int)p->m_historyAy.size() - 1;
}

//-------------------------------------------------------------------------------------------------

void CTrackPreview::Undo()
{
  if (p->m_historyAy.empty()) return;

  m_iHistoryIndex--;
  if (m_iHistoryIndex < 0)
    m_iHistoryIndex = 0;

  tTrackHistory *pHistory = &p->m_historyAy[m_iHistoryIndex];
  LoadHistory(pHistory);
}

//-------------------------------------------------------------------------------------------------

void CTrackPreview::Redo()
{
  if (p->m_historyAy.empty()) return;

  m_iHistoryIndex++;
  if (m_iHistoryIndex >= (int)p->m_historyAy.size())
    m_iHistoryIndex = (int)p->m_historyAy.size() - 1;

  tTrackHistory *pHistory = &p->m_historyAy[m_iHistoryIndex];
  LoadHistory(pHistory);
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
  UpdateReferenceModelPos_Internal();
}

//-------------------------------------------------------------------------------------------------

void CTrackPreview::UpdateReferenceModelTexture()
{
  update();
}

//-------------------------------------------------------------------------------------------------

void CTrackPreview::LoadHistory(const tTrackHistory *pHistory)
{
  int iSize = (int)pHistory->byteAy.size();
  uint8 *byData = new uint8[iSize];
  for (int i = 0; i < iSize; ++i)
    byData[i] = pHistory->byteAy[i];

  p->m_track.ClearData();
  p->m_track.ProcessTrackData(byData, iSize);

  delete[] byData;
  MarkDocumentEdited();
}

//-------------------------------------------------------------------------------------------------

void CTrackPreview::UpdateCar(eWhipModel carModel, eShapeSection aiLine, bool bMillionPlus)
{
  m_carModel = carModel;
  m_carAILine = aiLine;
  m_bMillionPlus = bMillionPlus;
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
  QMessageBox::information(
      this, "Reference Model",
      "Reference-mesh display will return with the roller-core overlay stories.");
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
  if (!m_pRenderService || m_sTrackFile.isEmpty() || m_bUnsavedChanges)
    return;

  const QString sAssetRoot = QFileInfo(m_sTrackFile).absolutePath();
  const uint64_t ullRequestId = m_pRenderService->EnqueueLoadAndRender(
      m_ullDocumentId, m_FrameState.GetDocumentRevision(), m_sTrackFile,
      sAssetRoot, DevicePixelSize(), devicePixelRatioF(), m_Camera);
  if (ullRequestId != 0)
    m_FrameState.BeginRequest(ullRequestId);
}

//-------------------------------------------------------------------------------------------------

void CTrackPreview::QueueResizeRender()
{
  if (!m_pRenderService || m_bUnsavedChanges)
    return;

  if (m_FrameState.GetDisplayState() == eEdFrameDisplayState::CURRENT) {
    const uint64_t ullRequestId = m_pRenderService->EnqueueRender(
        m_ullDocumentId, m_FrameState.GetDocumentRevision(),
        m_FrameState.GetInstalledGeometryEpoch(), DevicePixelSize(),
        devicePixelRatioF(), m_Camera);
    if (ullRequestId != 0)
      m_FrameState.BeginRequest(ullRequestId);
  } else if (m_FrameState.GetDisplayState()
             == eEdFrameDisplayState::PLACEHOLDER) {
    QueueLoadAndRender();
  }
}

//-------------------------------------------------------------------------------------------------

void CTrackPreview::OnRenderCompleted(const tEdRenderResult &Result)
{
  if (Result.Tag.ullDocumentId != m_ullDocumentId)
    return;
  if (!m_FrameState.ApplyResult(Result))
    return;

  update();
  emit FrameStateChanged();
}

//-------------------------------------------------------------------------------------------------

void CTrackPreview::Activate()
{
  QueueLoadAndRender();
}

//-------------------------------------------------------------------------------------------------

void CTrackPreview::MarkDocumentEdited()
{
  m_FrameState.MarkDocumentEdited();
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

bool CTrackPreview::Export(eExportType exportType)
{
  if (!CanExport())
    return false;

#if !TRACKEDITOR_ENABLE_FBX
  if (exportType == eExportType::EXPORT_FBX)
    return false;
#endif

  //get export settings
  CExportWizard exportWizard(this, exportType);
  if (!exportWizard.exec())
    return false;

  //save track
  QString sFilter = "";
  switch (exportType) {
    case eExportType::EXPORT_FBX:
      sFilter = "FBX Files (*.fbx)";
      break;
    case eExportType::EXPORT_OBJ:
      sFilter = "OBJ Files (*.obj)";
      break;
  }
  QString sFilename = QDir::toNativeSeparators(QFileDialog::getSaveFileName(
    this, "Export Track As", p->m_track.m_sTrackFileFolder.c_str(), sFilter));
  QString sFolder = sFilename.left(sFilename.lastIndexOf(QDir::separator()));
  QString sName = sFilename.right(sFilename.size() - sFilename.lastIndexOf(QDir::separator()) - 1);
  sName = sName.left(sName.lastIndexOf('.'));

  //make texture file
  QString sTexFile = sFolder + "\\" + sName + ".png";
  p->m_track.m_pTex->ExportToPngFile(sTexFile.toLatin1().constData());

  //make sign texture file
  QString sSignTexFile = sFolder + "\\" + sName + "_BLD.png";
  p->m_track.m_pBld->ExportToPngFile(sSignTexFile.toLatin1().constData());

  //main models will have fronts only if backs are separate only
  eBackModeling backModeling = eBackModeling::FRONTS_AND_BACKS;
  if (exportWizard.m_bExportBacks)
    backModeling = eBackModeling::FRONTS;

  //generate models
  std::vector<CShapeData *> signAy;
  std::vector<CShapeData *> signBackAy;
  std::vector<std::pair<std::string, CShapeData *>> trackSectionAy;
  if (exportWizard.m_bExportSeparate) {
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
    CShapeData *pCenterBack = NULL;
    CShapeData *pLShoulderBack = NULL;
    CShapeData *pRShoulderBack = NULL;
    CShapeData *pLWallBack = NULL;
    CShapeData *pRWallBack = NULL;
    CShapeData *pRoofBack = NULL;
    CShapeData *pOWallFloorBack = NULL;
    CShapeData *pLLOWallBack = NULL;
    CShapeData *pRLOWallBack = NULL;
    CShapeData *pLUOWallBack = NULL;
    CShapeData *pRUOWallBack = NULL;

    CShapeFactory::GetShapeFactory().MakeAILine(      &pCenterLine,      p->m_pShader, &p->m_track, eShapeSection::CENTERLINE, true);
    CShapeFactory::GetShapeFactory().MakeAILine(      &pAILine1,         p->m_pShader, &p->m_track, eShapeSection::CARLINE1,   true);
    CShapeFactory::GetShapeFactory().MakeAILine(      &pAILine2,         p->m_pShader, &p->m_track, eShapeSection::CARLINE2,   true);
    CShapeFactory::GetShapeFactory().MakeAILine(      &pAILine3,         p->m_pShader, &p->m_track, eShapeSection::CARLINE3,   true);
    CShapeFactory::GetShapeFactory().MakeAILine(      &pAILine4,         p->m_pShader, &p->m_track, eShapeSection::CARLINE4,   true);
    CShapeFactory::GetShapeFactory().MakeTrackSurface(&pCenterSurf,      p->m_pShader, &p->m_track, eShapeSection::CENTER,     true, false, backModeling);
    CShapeFactory::GetShapeFactory().MakeTrackSurface(&pLShoulderSurf,   p->m_pShader, &p->m_track, eShapeSection::LSHOULDER,  true, false, backModeling);
    CShapeFactory::GetShapeFactory().MakeTrackSurface(&pRShoulderSurf,   p->m_pShader, &p->m_track, eShapeSection::RSHOULDER,  true, false, backModeling);
    CShapeFactory::GetShapeFactory().MakeTrackSurface(&pLWallSurf,       p->m_pShader, &p->m_track, eShapeSection::LWALL,      true, false, backModeling);
    CShapeFactory::GetShapeFactory().MakeTrackSurface(&pRWallSurf,       p->m_pShader, &p->m_track, eShapeSection::RWALL,      true, false, backModeling);
    CShapeFactory::GetShapeFactory().MakeTrackSurface(&pRoofSurf,        p->m_pShader, &p->m_track, eShapeSection::ROOF,       true, false, backModeling);
    CShapeFactory::GetShapeFactory().MakeTrackSurface(&pOWallFloorSurf,  p->m_pShader, &p->m_track, eShapeSection::OWALLFLOOR, true, false, backModeling);
    CShapeFactory::GetShapeFactory().MakeTrackSurface(&pLLOWallSurf,     p->m_pShader, &p->m_track, eShapeSection::LLOWALL,    true, false, backModeling);
    CShapeFactory::GetShapeFactory().MakeTrackSurface(&pRLOWallSurf,     p->m_pShader, &p->m_track, eShapeSection::RLOWALL,    true, false, backModeling);
    CShapeFactory::GetShapeFactory().MakeTrackSurface(&pLUOWallSurf,     p->m_pShader, &p->m_track, eShapeSection::LUOWALL,    true, false, backModeling);
    CShapeFactory::GetShapeFactory().MakeTrackSurface(&pRUOWallSurf,     p->m_pShader, &p->m_track, eShapeSection::RUOWALL,    true, false, backModeling);
    if (exportWizard.m_bExportBacks) {
      CShapeFactory::GetShapeFactory().MakeTrackSurface(&pCenterBack,      p->m_pShader, &p->m_track, eShapeSection::CENTER,     true, false, eBackModeling::BACKS);
      CShapeFactory::GetShapeFactory().MakeTrackSurface(&pLShoulderBack,   p->m_pShader, &p->m_track, eShapeSection::LSHOULDER,  true, false, eBackModeling::BACKS);
      CShapeFactory::GetShapeFactory().MakeTrackSurface(&pRShoulderBack,   p->m_pShader, &p->m_track, eShapeSection::RSHOULDER,  true, false, eBackModeling::BACKS);
      CShapeFactory::GetShapeFactory().MakeTrackSurface(&pLWallBack,       p->m_pShader, &p->m_track, eShapeSection::LWALL,      true, false, eBackModeling::BACKS);
      CShapeFactory::GetShapeFactory().MakeTrackSurface(&pRWallBack,       p->m_pShader, &p->m_track, eShapeSection::RWALL,      true, false, eBackModeling::BACKS);
      CShapeFactory::GetShapeFactory().MakeTrackSurface(&pRoofBack,        p->m_pShader, &p->m_track, eShapeSection::ROOF,       true, false, eBackModeling::BACKS);
      CShapeFactory::GetShapeFactory().MakeTrackSurface(&pOWallFloorBack,  p->m_pShader, &p->m_track, eShapeSection::OWALLFLOOR, true, false, eBackModeling::BACKS);
      CShapeFactory::GetShapeFactory().MakeTrackSurface(&pLLOWallBack,     p->m_pShader, &p->m_track, eShapeSection::LLOWALL,    true, false, eBackModeling::BACKS);
      CShapeFactory::GetShapeFactory().MakeTrackSurface(&pRLOWallBack,     p->m_pShader, &p->m_track, eShapeSection::RLOWALL,    true, false, eBackModeling::BACKS);
      CShapeFactory::GetShapeFactory().MakeTrackSurface(&pLUOWallBack,     p->m_pShader, &p->m_track, eShapeSection::LUOWALL,    true, false, eBackModeling::BACKS);
      CShapeFactory::GetShapeFactory().MakeTrackSurface(&pRUOWallBack,     p->m_pShader, &p->m_track, eShapeSection::RUOWALL,    true, false, eBackModeling::BACKS);
    }

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
    if (exportWizard.m_bExportBacks) {
      trackSectionAy.push_back(std::make_pair("Center (Back)", pCenterBack));
      trackSectionAy.push_back(std::make_pair("Left Shoulder (Back)", pLShoulderBack));
      trackSectionAy.push_back(std::make_pair("Right Shoulder (Back)", pRShoulderBack));
      trackSectionAy.push_back(std::make_pair("Left Wall (Back)", pLWallBack));
      trackSectionAy.push_back(std::make_pair("Right Wall (Back)", pRWallBack));
      trackSectionAy.push_back(std::make_pair("Roof (Back)", pRoofBack));
      trackSectionAy.push_back(std::make_pair("Outer Wall Floor (Back)", pOWallFloorBack));
      trackSectionAy.push_back(std::make_pair("Left Lower Outer Wall (Back)", pLLOWallBack));
      trackSectionAy.push_back(std::make_pair("Right Lower Outer Wall (Back)", pRLOWallBack));
      trackSectionAy.push_back(std::make_pair("Left Upper Outer Wall (Back)", pLUOWallBack));
      trackSectionAy.push_back(std::make_pair("Right Upper Outer Wall (Back)", pRUOWallBack));
    }
  } else {
    CShapeData *pExportTrack = NULL;
    CShapeData *pExportBacks = NULL;

    CShapeFactory::GetShapeFactory().MakeTrackSurface(&pExportTrack, p->m_pShader, &p->m_track, eShapeSection::EXPORT, true, false, backModeling);
    trackSectionAy.push_back(std::make_pair("Track", pExportTrack));

    if (exportWizard.m_bExportBacks) {
      CShapeFactory::GetShapeFactory().MakeTrackSurface(&pExportBacks, p->m_pShader, &p->m_track, eShapeSection::EXPORT, true, false, eBackModeling::BACKS);
      trackSectionAy.push_back(std::make_pair("Track (Back)", pExportBacks));
    }
  }

  for (std::vector<std::pair<std::string, CShapeData *>>::iterator it = trackSectionAy.begin(); it != trackSectionAy.end(); ++it)
    it->second->FlipTexCoordsForExport();

  if (exportWizard.m_bExportSigns) {
    CShapeFactory::GetShapeFactory().MakeSigns(p->m_pShader, &p->m_track, signAy, backModeling);
    for (std::vector<CShapeData *>::iterator it = signAy.begin(); it != signAy.end(); ++it) {
      (*it)->TransformVertsForExport(); //signs need to be moved to the right position on track, this is normally done in the shader
      (*it)->FlipTexCoordsForExport();
    }
    if (exportWizard.m_bExportBacks) {
      CShapeFactory::GetShapeFactory().MakeSigns(p->m_pShader, &p->m_track, signBackAy, eBackModeling::BACKS);
      for (std::vector<CShapeData *>::iterator it = signBackAy.begin(); it != signBackAy.end(); ++it) {
        (*it)->TransformVertsForExport(); //signs need to be moved to the right position on track, this is normally done in the shader
        (*it)->FlipTexCoordsForExport();
      }
    }
  }

  //export
  bool bExported = false;
  switch (exportType) {
    case eExportType::EXPORT_FBX:
#if TRACKEDITOR_ENABLE_FBX
      bExported = CFBXExporter::GetFBXExporter().ExportTrack(trackSectionAy,
                                                             signAy,
                                                             signBackAy,
                                                             sName.toLatin1().constData(),
                                                             sFilename.toLatin1().constData(),
                                                             sTexFile.toLatin1().constData(),
                                                             sSignTexFile.toLatin1().constData());
#endif
      break;
    case eExportType::EXPORT_OBJ:
      bExported = CObjExporter::GetObjExporter().ExportTrack(trackSectionAy,
                                                             signAy,
                                                             signBackAy,
                                                             sFolder.toLatin1().constData(),
                                                             sName.toLatin1().constData(),
                                                             sFilename.toLatin1().constData());
      break;
  }

  //cleanup
  for (std::vector<std::pair<std::string, CShapeData *>>::iterator it = trackSectionAy.begin(); it != trackSectionAy.end(); ++it)
    delete it->second;
  for (std::vector<CShapeData *>::iterator it = signAy.begin(); it != signAy.end(); ++it)
    delete *it;

  if (!bExported)
    return false;

  g_pMainWindow->m_sLastTrackFilesFolder = sFolder;
  return true;
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

  std::vector<uint8> data;
  std::vector<uint8> mangledData;
  p->m_track.GetTrackData(data);

  std::vector<uint8> *pOutData;
  //if (bIsMangled) {
  //  MangleFile(data, mangledData);
  //  pOutData = &mangledData;
  //} else {
  pOutData = &data;
//}

  QFile file(sFilename);
  file.resize(0);
  if (file.open(QIODevice::ReadWrite)) {
    QTextStream stream(&file);
    for (int i = 0; i < pOutData->size(); ++i) {
      stream << (char)((*pOutData)[i]);
    }
    file.close();
  }

  return true;
}

//-------------------------------------------------------------------------------------------------

void CTrackPreview::UpdateReferenceModelPos_Internal()
{
  update();
}

//-------------------------------------------------------------------------------------------------
