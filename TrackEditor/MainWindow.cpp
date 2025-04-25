#include <glew.h>
#include "TrackEditor.h"
#include "MainWindow.h"
#include "qmessagebox.h"
#include "qfiledialog.h"
#include "qsettings.h"
#include "qdesktopwidget.h"
#include "Track.h"
#include "LogDialog.h"
#include "Palette.h"
#include "Texture.h"
#include "TilePicker.h"
#include "EditSurfaceDialog.h"
#include "EditSeriesDialog.h"
#include "TrackPreview.h"
#include "DebugChunkData.h"
#include "GlobalTrackSettings.h"
#include "qdockwidget.h"
#include "DisplaySettings.h"
#include "EditGeometryWidget.h"
#include "EditSignWidget.h"
#include "EditAudioWidget.h"
#include "EditStuntWidget.h"
#include "qtextstream.h"
#include "QtHelpers.h"
#include "Logging.h"
#include "NewTrackDialog.h"
#include "PreferencesDialog.h"
#include "AssignBacksDialog.h"
#include "NoclipComponent.h"
#include "GameClock.h"
#include "GameInput.h"
#include "qtimer.h"
#include "MathHelpers.h"
#if defined (IS_WINDOWS)
  #include <Windows.h>
#endif
//-------------------------------------------------------------------------------------------------
#if defined(_DEBUG) && defined(IS_WINDOWS)
  #define new new(_CLIENT_BLOCK, __FILE__, __LINE__)
#endif
//-------------------------------------------------------------------------------------------------
CMainWindow *g_pMainWindow = NULL;
//-------------------------------------------------------------------------------------------------

static void LogMessageCbStatic(const char *szMsg, int iLen)
{
  (void)(iLen);
  g_pMainWindow->LogMessage(szMsg);
}

//-------------------------------------------------------------------------------------------------

tPreferences::tPreferences()
  : iHistoryMaxSize(DEFAULT_HISTORY_MAX_SIZE)
  , bPasteNewChunks(true)
  , bCopyRelativeYaw(true)
  , bCopyRelativePitch(false)
  , bCopyRelativeRoll(false)
  , bPasteDirection(true)
  , bPasteGeometry(true)
  , bPasteTextures(true)
  , bPasteSurfaceData(true)
  , bPasteAIBehavior(true)
  , bPasteDrawOrder(true)
  , bPasteSigns(true)
  , bPasteAudio(true)
{ };

//-------------------------------------------------------------------------------------------------

class CMainWindowPrivate
{
public:
  CMainWindowPrivate(CMainWindow *pMainWindow)
    : m_logDialog(pMainWindow)
  {};
  ~CMainWindowPrivate() 
  {};
  
  CLogDialog m_logDialog;

  QDockWidget *m_pDebugDataDockWidget;
  CDebugChunkData *m_pDebugData;
  QDockWidget *m_pGlobalSettingsDockWidget;
  QDockWidget *m_pEditSeriesDockWidget;
  QDockWidget *m_pDisplaySettingsDockWidget;
  QDockWidget *m_pEditGeometryDockWidget;
  QDockWidget *m_pEditSignDockWidget;
  QDockWidget *m_pEditAudioDockWidget;
  QDockWidget *m_pEditStuntDockWidget;
  CDisplaySettings *m_pDisplaySettings;
  QAction *m_pDebugAction;
  std::vector<CTrackPreview *> m_previewAy;
  CChunkAy m_clipBoard;
};

//-------------------------------------------------------------------------------------------------

CMainWindow::CMainWindow(const QString &sAppPath, float fDesktopScale)
  : QMainWindow(NULL)
  , m_sAppPath(sAppPath)
  , m_sLastTrackFilesFolder("")
  , m_fDesktopScale(fDesktopScale)
  , m_iNewTrackNum(0)
{
  //init
  Logging::SetWhipLibLoggingCallback(LogMessageCbStatic);
  p = new CMainWindowPrivate(this);
  g_pMainWindow = this;
  m_sSettingsFile = m_sAppPath + "/TrackEditor.ini";
  m_sSettingsFile = QDir::toNativeSeparators(m_sSettingsFile);
  setupUi(this);
  p->m_logDialog.hide();
  twViewer->setTabsClosable(true);
  lblChunkWarning->hide();
  lblStuntWarning->hide();
  CGameClock::GetGameClock().Init();
  CGameInput::GetGameInput().Init(&m_keyMapper);

  //setup dock widgets
  p->m_pDebugDataDockWidget = new QDockWidget("Debug Chunk Data", this);
  p->m_pDebugDataDockWidget->setObjectName("EditChunkData");
  p->m_pDebugDataDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
  p->m_pDebugData = new CDebugChunkData(p->m_pDebugDataDockWidget);
  p->m_pDebugDataDockWidget->setWidget(p->m_pDebugData);

  p->m_pGlobalSettingsDockWidget = new QDockWidget("Global Track Settings", this);
  p->m_pGlobalSettingsDockWidget->setObjectName("GlobalTrackSettings");
  p->m_pGlobalSettingsDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
  p->m_pGlobalSettingsDockWidget->setWidget(new CGlobalTrackSettings(p->m_pGlobalSettingsDockWidget));

  p->m_pEditSeriesDockWidget = new QDockWidget("Edit Series...", this);
  p->m_pEditSeriesDockWidget->setObjectName("EditSeries");
  p->m_pEditSeriesDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
  p->m_pEditSeriesDockWidget->setWidget(new CEditSeriesDialog(p->m_pEditSeriesDockWidget));

  p->m_pDisplaySettingsDockWidget = new QDockWidget("Display Settings", this);
  p->m_pDisplaySettingsDockWidget->setObjectName("DisplaySettings");
  p->m_pDisplaySettingsDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
  p->m_pDisplaySettings = new CDisplaySettings(p->m_pDisplaySettingsDockWidget);
  p->m_pDisplaySettingsDockWidget->setWidget(p->m_pDisplaySettings);

  p->m_pEditGeometryDockWidget = new QDockWidget("Edit Chunk Data", this);
  p->m_pEditGeometryDockWidget->setObjectName("EditGeometry");
  p->m_pEditGeometryDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
  p->m_pEditGeometryDockWidget->setWidget(new CEditGeometryWidget(p->m_pEditGeometryDockWidget));

  p->m_pEditSignDockWidget = new QDockWidget("Edit Signs", this);
  p->m_pEditSignDockWidget->setObjectName("EditSigns");
  p->m_pEditSignDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
  p->m_pEditSignDockWidget->setWidget(new CEditSignWidget(p->m_pEditSignDockWidget));

  p->m_pEditAudioDockWidget = new QDockWidget("Edit Audio", this);
  p->m_pEditAudioDockWidget->setObjectName("EditAudio");
  p->m_pEditAudioDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
  p->m_pEditAudioDockWidget->setWidget(new CEditAudioWidget(p->m_pEditAudioDockWidget));

  p->m_pEditStuntDockWidget = new QDockWidget("Edit Stunts", this);
  p->m_pEditStuntDockWidget->setObjectName("EditStunts");
  p->m_pEditStuntDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
  p->m_pEditStuntDockWidget->setWidget(new CEditStuntWidget(p->m_pEditStuntDockWidget));

  //setup view menu
  menuView->addAction(p->m_pEditGeometryDockWidget->toggleViewAction());
  menuView->addAction(p->m_pEditSignDockWidget->toggleViewAction());
  menuView->addAction(p->m_pEditAudioDockWidget->toggleViewAction());
  menuView->addAction(p->m_pEditStuntDockWidget->toggleViewAction());
  menuView->addAction(p->m_pEditSeriesDockWidget->toggleViewAction());
  menuView->addAction(p->m_pGlobalSettingsDockWidget->toggleViewAction());
  menuView->addAction(p->m_pDisplaySettingsDockWidget->toggleViewAction());
  QAction *pBacksAction = new QAction("Assign Backs...", menuView);
  menuView->addAction(pBacksAction);
  menuView->addSeparator();
  menuView->addAction(p->m_pDebugDataDockWidget->toggleViewAction());
  p->m_pDebugAction = new QAction("Debug Log", menuView);
  p->m_pDebugAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_G));
  menuView->addAction(p->m_pDebugAction);

  //deselect action, will be cleaned up by qt parent
  QAction *pDeselectAction = new QAction(this);
  pDeselectAction->setShortcut(QKeySequence(Qt::Key_Escape));
  addAction(pDeselectAction);

  //setup history timer
  m_pSaveHistoryTimer = new QTimer(this);
  m_pSaveHistoryTimer->setSingleShot(true);
  m_pSaveHistoryTimer->setInterval(250);
  connect(m_pSaveHistoryTimer, &QTimer::timeout, this, &CMainWindow::OnSaveHistoryTimer, Qt::QueuedConnection);

  //setup stunt timer
  m_pStuntTimer = new QTimer(this);
  m_pStuntTimer->setInterval(28);
  connect(m_pStuntTimer, &QTimer::timeout, this, &CMainWindow::OnStuntTimer);
  m_pStuntTimer->start();

  //setup zero timer
  m_pZeroTimer = new QTimer(this);
  m_pZeroTimer->setInterval(0);
  connect(m_pZeroTimer, &QTimer::timeout, this, &CMainWindow::OnZeroTimer);
  m_pZeroTimer->start();

  //signals
  connect(this, &CMainWindow::LogMsgSig, this, &CMainWindow::OnLogMsg, Qt::QueuedConnection);
  connect(actNew, &QAction::triggered, this, &CMainWindow::OnNewTrack);
  connect(actLoad, &QAction::triggered, this, &CMainWindow::OnLoadTrack);
  connect(actSave, &QAction::triggered, this, &CMainWindow::OnSaveTrack);
  connect(actSaveAs, &QAction::triggered, this, &CMainWindow::OnSaveTrackAs);
  connect(actExportFBX, &QAction::triggered, this, &CMainWindow::OnExportFBX);
  connect(actExportOBJ, &QAction::triggered, this, &CMainWindow::OnExportOBJ);
  connect(actUndo, &QAction::triggered, this, &CMainWindow::OnUndo);
  connect(actRedo, &QAction::triggered, this, &CMainWindow::OnRedo);
  connect(actCut, &QAction::triggered, this, &CMainWindow::OnCut);
  connect(actCopy, &QAction::triggered, this, &CMainWindow::OnCopy);
  connect(actPaste, &QAction::triggered, this, &CMainWindow::OnPaste);
  connect(actDelete, &QAction::triggered, this, &CMainWindow::OnDeleteChunkClicked);
  connect(actSelectAll, &QAction::triggered, this, &CMainWindow::OnSelectAll);
  connect(actMirror, &QAction::triggered, this, &CMainWindow::OnMirror);
  connect(pDeselectAction, &QAction::triggered, this, &CMainWindow::OnDeselect);
  connect(pBacksAction, &QAction::triggered, this, &CMainWindow::OnBacks);
  connect(p->m_pDebugAction, &QAction::triggered, this, &CMainWindow::OnDebug);
  connect(actPreferences, &QAction::triggered, this, &CMainWindow::OnPreferences);
  connect(actAbout, &QAction::triggered, this, &CMainWindow::OnAbout);
  connect(twViewer, &QTabWidget::tabCloseRequested, this, &CMainWindow::OnTabCloseRequested);
  connect(twViewer, &QTabWidget::currentChanged, this, &CMainWindow::OnTabChanged);

  connect(sbSelChunksFrom, SIGNAL(valueChanged(int)), this, SLOT(OnSelChunksFromChanged(int)));
  connect(sbSelChunksTo, SIGNAL(valueChanged(int)), this, SLOT(OnSelChunksToChanged(int)));
  connect(ckTo, &QCheckBox::toggled, this, &CMainWindow::OnToChecked);
  connect(pbDelete, &QPushButton::clicked, this, &CMainWindow::OnDeleteChunkClicked);
  connect(pbAddChunk, &QPushButton::clicked, this, &CMainWindow::OnAddChunkClicked);

  connect(p->m_pDisplaySettings, &CDisplaySettings::UpdatePreviewSig, this, &CMainWindow::OnUpdatePreview);
  connect(p->m_pDisplaySettings, &CDisplaySettings::AttachLastCheckedSig, this, &CMainWindow::OnAttachLast);
  connect(p->m_pDisplaySettings, &CDisplaySettings::OpenReferenceModelSig, this, &CMainWindow::OnOpenReferenceModel);

  //open window
  LoadSettings();
}

//-------------------------------------------------------------------------------------------------

CMainWindow::~CMainWindow()
{
}

//-------------------------------------------------------------------------------------------------

void CMainWindow::closeEvent(QCloseEvent *pEvent)
{
  if (!SaveChangesAndContinue()) {
    pEvent->ignore();
    return;
  }
  pEvent->accept();

  m_pZeroTimer->stop();
  SaveSettings();

  //cleanup
  twViewer->blockSignals(true);
  for (int i = 0; i < (int)p->m_previewAy.size(); ++i) {
    p->m_previewAy[i]->makeCurrent();
    delete p->m_previewAy[i];
  }
  p->m_previewAy.clear();
  twViewer->blockSignals(false);

  QMainWindow::closeEvent(pEvent);

  if (p) {
    delete p;
    p = NULL;
  }
}

//-------------------------------------------------------------------------------------------------

void CMainWindow::LogMessage(const QString &sMsg)
{
  //log messages can come from anywhere like OpenGL
  //so we need to put this onto the GUI thread
  emit LogMsgSig(sMsg);
}

//-------------------------------------------------------------------------------------------------

void CMainWindow::SaveHistory(const QString &sDescription)
{
  GetCurrentPreview()->m_bUnsavedChanges = true;
  m_sHistoryDescription = sDescription;
  m_pSaveHistoryTimer->start();
}

//-------------------------------------------------------------------------------------------------

void CMainWindow::OnLogMsg(QString sMsg)
{
  if (p)
    p->m_logDialog.LogMessage(sMsg);
}

//-------------------------------------------------------------------------------------------------

void CMainWindow::OnNewTrack()
{
  CNewTrackDialog dlg(this, ++m_iNewTrackNum);
  if (dlg.exec()) {
    CTrackPreview *pPreview = new CTrackPreview(this, dlg.GetFilename());
    pPreview->GetTrack()->m_sBuildingFile = dlg.GetBld().toLatin1().constData();
    pPreview->GetTrack()->m_sTextureFile = dlg.GetTex().toLatin1().constData();
    pPreview->SaveHistory("New track created");
    m_sLastTrackFilesFolder = dlg.GetFilename().left(dlg.GetFilename().lastIndexOf(QDir::separator()));
    //add to array and create preview window
    p->m_previewAy.push_back(pPreview);
    twViewer->addTab(pPreview, pPreview->GetTitle(false));
    twViewer->setCurrentIndex((int)p->m_previewAy.size() - 1);
  }
}

//-------------------------------------------------------------------------------------------------

void CMainWindow::OnLoadTrack()
{
  //load track
  QString sFilename = QDir::toNativeSeparators(QFileDialog::getOpenFileName(
    this, "Load Track", m_sLastTrackFilesFolder, QString("Track Files (*.TRK)")));
  if (sFilename.isEmpty())
    return;

  for (int i = 0; i < (int)p->m_previewAy.size(); ++i) {
    if (p->m_previewAy[i]->GetFilename().compare(sFilename) == 0) {
      twViewer->setCurrentIndex(i);
      return;
    }
  }

  CTrackPreview *pPreview = new CTrackPreview(this);
  if (!pPreview->LoadTrack(sFilename)) {
    //load failed
    delete pPreview;
  } else { //load successful    
    //update variables
    m_sLastTrackFilesFolder = sFilename.left(sFilename.lastIndexOf(QDir::separator()));

    //add to array and create preview window
    p->m_previewAy.push_back(pPreview);
    twViewer->addTab(pPreview, pPreview->GetTitle(false));
    twViewer->setCurrentIndex((int)p->m_previewAy.size() - 1);
  }
}

//-------------------------------------------------------------------------------------------------

void CMainWindow::OnSaveTrack()
{
  if (GetCurrentPreview())
    GetCurrentPreview()->SaveTrack();
}

//-------------------------------------------------------------------------------------------------

void CMainWindow::OnSaveTrackAs()
{
  if (GetCurrentPreview())
    GetCurrentPreview()->SaveTrackAs();
}

//-------------------------------------------------------------------------------------------------

void CMainWindow::OnExportFBX()
{
  if (GetCurrentPreview())
    GetCurrentPreview()->Export(eExportType::EXPORT_FBX);
}

//-------------------------------------------------------------------------------------------------

void CMainWindow::OnExportOBJ()
{
  if (GetCurrentPreview())
    GetCurrentPreview()->Export(eExportType::EXPORT_OBJ);
}

//-------------------------------------------------------------------------------------------------

void CMainWindow::OnUndo()
{
  if (GetCurrentPreview()) {
    GetCurrentPreview()->Undo();
  }
  UpdateWindow();
}

//-------------------------------------------------------------------------------------------------

void CMainWindow::OnRedo()
{
  if (GetCurrentPreview())
    GetCurrentPreview()->Redo();
  UpdateWindow();
}

//-------------------------------------------------------------------------------------------------

void CMainWindow::OnCut()
{
  OnCopy();
  OnDeleteChunkClicked();
}

//-------------------------------------------------------------------------------------------------

void CMainWindow::OnCopy()
{
  if (!GetCurrentTrack())
    return;

  if (sbSelChunksTo->value() >= (int)GetCurrentTrack()->m_chunkAy.size() 
      || sbSelChunksFrom->value() >= (int)GetCurrentTrack()->m_chunkAy.size())
    return;

  p->m_clipBoard.clear();

  int iPrevChunk = sbSelChunksFrom->value() - 1;
  if (iPrevChunk < 0)
    iPrevChunk = (int)GetCurrentTrack()->m_chunkAy.size() - 1;

  for (int i = sbSelChunksFrom->value(); i <= sbSelChunksTo->value(); ++i) {
    p->m_clipBoard.push_back(GetCurrentTrack()->m_chunkAy[i]);
    if (m_preferences.bCopyRelativeYaw)
      p->m_clipBoard[p->m_clipBoard.size() - 1].dYaw = p->m_clipBoard[p->m_clipBoard.size() - 1].dYaw - GetCurrentTrack()->m_chunkAy[iPrevChunk].dYaw;
    if (m_preferences.bCopyRelativePitch)
      p->m_clipBoard[p->m_clipBoard.size() - 1].dPitch = p->m_clipBoard[p->m_clipBoard.size() - 1].dPitch - GetCurrentTrack()->m_chunkAy[iPrevChunk].dPitch;
    if (m_preferences.bCopyRelativeRoll)
      p->m_clipBoard[p->m_clipBoard.size() - 1].dRoll = p->m_clipBoard[p->m_clipBoard.size() - 1].dRoll - GetCurrentTrack()->m_chunkAy[iPrevChunk].dRoll;
  }
}

//-------------------------------------------------------------------------------------------------

void CMainWindow::OnPaste()
{
  if (!GetCurrentTrack())
    return;

  if (sbSelChunksTo->value() > (int)GetCurrentTrack()->m_chunkAy.size()
      || sbSelChunksFrom->value() > (int)GetCurrentTrack()->m_chunkAy.size())
    return;

  tGeometryChunk prevChunk;
  memset(&prevChunk, 0, sizeof(prevChunk));
  if (!GetCurrentTrack()->m_chunkAy.empty()) {
    int iPrevChunk = sbSelChunksFrom->value() - 1;
    if (iPrevChunk < 0)
      iPrevChunk = (int)GetCurrentTrack()->m_chunkAy.size() - 1;

    prevChunk = GetCurrentTrack()->m_chunkAy[iPrevChunk];
  }

  if (m_preferences.bPasteNewChunks) {
    //update clipboard
    for (int i = 0; i < (int)p->m_clipBoard.size(); ++i) {
      if (m_preferences.bCopyRelativeYaw)
        p->m_clipBoard[i].dYaw = p->m_clipBoard[i].dYaw + prevChunk.dYaw;
      if (m_preferences.bCopyRelativePitch)
        p->m_clipBoard[i].dPitch = p->m_clipBoard[i].dPitch + prevChunk.dPitch;
      if (m_preferences.bCopyRelativeRoll)
        p->m_clipBoard[i].dRoll = p->m_clipBoard[i].dRoll + prevChunk.dRoll;
      p->m_clipBoard[i].dYaw = MathHelpers::ConstrainAngle(p->m_clipBoard[i].dYaw);
      p->m_clipBoard[i].dPitch = MathHelpers::ConstrainAngle(p->m_clipBoard[i].dPitch);
      p->m_clipBoard[i].dRoll = MathHelpers::ConstrainAngle(p->m_clipBoard[i].dRoll);

      if (!m_preferences.bPasteDirection) {
        p->m_clipBoard[i].iLength               = prevChunk.iLength;
        p->m_clipBoard[i].dYaw                  = prevChunk.dYaw;
        p->m_clipBoard[i].dPitch                = prevChunk.dPitch;
        p->m_clipBoard[i].dRoll                 = prevChunk.dRoll;
      }
      if (!m_preferences.bPasteGeometry) {
        p->m_clipBoard[i].iLeftShoulderWidth    = prevChunk.iLeftShoulderWidth;
        p->m_clipBoard[i].iLeftLaneWidth        = prevChunk.iLeftLaneWidth;
        p->m_clipBoard[i].iRightLaneWidth       = prevChunk.iRightLaneWidth;
        p->m_clipBoard[i].iRightShoulderWidth   = prevChunk.iRightShoulderWidth;
        p->m_clipBoard[i].iLeftShoulderHeight   = prevChunk.iLeftShoulderHeight;
        p->m_clipBoard[i].iRightShoulderHeight  = prevChunk.iRightShoulderHeight;
        p->m_clipBoard[i].iLUOuterWallHOffset   = prevChunk.iLUOuterWallHOffset;
        p->m_clipBoard[i].iLLOuterWallHOffset   = prevChunk.iLLOuterWallHOffset;
        p->m_clipBoard[i].iLOuterFloorHOffset   = prevChunk.iLOuterFloorHOffset;
        p->m_clipBoard[i].iROuterFloorHOffset   = prevChunk.iROuterFloorHOffset;
        p->m_clipBoard[i].iRLOuterWallHOffset   = prevChunk.iRLOuterWallHOffset;
        p->m_clipBoard[i].iRUOuterWallHOffset   = prevChunk.iRUOuterWallHOffset;
        p->m_clipBoard[i].iLUOuterWallHeight    = prevChunk.iLUOuterWallHeight;
        p->m_clipBoard[i].iLLOuterWallHeight    = prevChunk.iLLOuterWallHeight;
        p->m_clipBoard[i].iLOuterFloorHeight    = prevChunk.iLOuterFloorHeight;
        p->m_clipBoard[i].iROuterFloorHeight    = prevChunk.iROuterFloorHeight;
        p->m_clipBoard[i].iRLOuterWallHeight    = prevChunk.iRLOuterWallHeight;
        p->m_clipBoard[i].iRUOuterWallHeight    = prevChunk.iRUOuterWallHeight;
        p->m_clipBoard[i].iRoofHeight           = prevChunk.iRoofHeight;
    
      }
      if (!m_preferences.bPasteTextures) {
        uint32 uiLeftSurfaceType      = CTrack::GetSignedBitValueFromInt(p->m_clipBoard[i].iLeftSurfaceType     ) & ~SURFACE_MASK_TEXTURE_INDEX;
        uint32 uiCenterSurfaceType    = CTrack::GetSignedBitValueFromInt(p->m_clipBoard[i].iCenterSurfaceType   ) & ~SURFACE_MASK_TEXTURE_INDEX;
        uint32 uiRightSurfaceType     = CTrack::GetSignedBitValueFromInt(p->m_clipBoard[i].iRightSurfaceType    ) & ~SURFACE_MASK_TEXTURE_INDEX;
        uint32 uiLeftWallType         = CTrack::GetSignedBitValueFromInt(p->m_clipBoard[i].iLeftWallType        ) & ~SURFACE_MASK_TEXTURE_INDEX;
        uint32 uiRightWallType        = CTrack::GetSignedBitValueFromInt(p->m_clipBoard[i].iRightWallType       ) & ~SURFACE_MASK_TEXTURE_INDEX;
        uint32 uiRoofType             = CTrack::GetSignedBitValueFromInt(p->m_clipBoard[i].iRoofType            ) & ~SURFACE_MASK_TEXTURE_INDEX;
        uint32 uiLUOuterWallType      = CTrack::GetSignedBitValueFromInt(p->m_clipBoard[i].iLUOuterWallType     ) & ~SURFACE_MASK_TEXTURE_INDEX;
        uint32 uiLLOuterWallType      = CTrack::GetSignedBitValueFromInt(p->m_clipBoard[i].iLLOuterWallType     ) & ~SURFACE_MASK_TEXTURE_INDEX;
        uint32 uiOuterFloorType       = CTrack::GetSignedBitValueFromInt(p->m_clipBoard[i].iOuterFloorType      ) & ~SURFACE_MASK_TEXTURE_INDEX;
        uint32 uiRLOuterWallType      = CTrack::GetSignedBitValueFromInt(p->m_clipBoard[i].iRLOuterWallType     ) & ~SURFACE_MASK_TEXTURE_INDEX;
        uint32 uiRUOuterWallType      = CTrack::GetSignedBitValueFromInt(p->m_clipBoard[i].iRUOuterWallType     ) & ~SURFACE_MASK_TEXTURE_INDEX;
        uint32 uiEnvironmentFloorType = CTrack::GetSignedBitValueFromInt(p->m_clipBoard[i].iEnvironmentFloorType) & ~SURFACE_MASK_TEXTURE_INDEX;
        uiLeftSurfaceType      |= CTrack::GetSignedBitValueFromInt(prevChunk.iLeftSurfaceType)      & SURFACE_MASK_TEXTURE_INDEX;
        uiCenterSurfaceType    |= CTrack::GetSignedBitValueFromInt(prevChunk.iCenterSurfaceType)    & SURFACE_MASK_TEXTURE_INDEX;
        uiRightSurfaceType     |= CTrack::GetSignedBitValueFromInt(prevChunk.iRightSurfaceType)     & SURFACE_MASK_TEXTURE_INDEX;
        uiLeftWallType         |= CTrack::GetSignedBitValueFromInt(prevChunk.iLeftWallType)         & SURFACE_MASK_TEXTURE_INDEX;
        uiRightWallType        |= CTrack::GetSignedBitValueFromInt(prevChunk.iRightWallType)        & SURFACE_MASK_TEXTURE_INDEX;
        uiRoofType             |= CTrack::GetSignedBitValueFromInt(prevChunk.iRoofType)             & SURFACE_MASK_TEXTURE_INDEX;
        uiLUOuterWallType      |= CTrack::GetSignedBitValueFromInt(prevChunk.iLUOuterWallType)      & SURFACE_MASK_TEXTURE_INDEX;
        uiLLOuterWallType      |= CTrack::GetSignedBitValueFromInt(prevChunk.iLLOuterWallType)      & SURFACE_MASK_TEXTURE_INDEX;
        uiOuterFloorType       |= CTrack::GetSignedBitValueFromInt(prevChunk.iOuterFloorType)       & SURFACE_MASK_TEXTURE_INDEX;
        uiRLOuterWallType      |= CTrack::GetSignedBitValueFromInt(prevChunk.iRLOuterWallType)      & SURFACE_MASK_TEXTURE_INDEX;
        uiRUOuterWallType      |= CTrack::GetSignedBitValueFromInt(prevChunk.iRUOuterWallType)      & SURFACE_MASK_TEXTURE_INDEX;
        uiEnvironmentFloorType |= CTrack::GetSignedBitValueFromInt(prevChunk.iEnvironmentFloorType) & SURFACE_MASK_TEXTURE_INDEX;
        p->m_clipBoard[i].iLeftSurfaceType      = CTrack::GetIntValueFromSignedBit(uiLeftSurfaceType     );
        p->m_clipBoard[i].iCenterSurfaceType    = CTrack::GetIntValueFromSignedBit(uiCenterSurfaceType   );
        p->m_clipBoard[i].iRightSurfaceType     = CTrack::GetIntValueFromSignedBit(uiRightSurfaceType    );
        p->m_clipBoard[i].iLeftWallType         = CTrack::GetIntValueFromSignedBit(uiLeftWallType        );
        p->m_clipBoard[i].iRightWallType        = CTrack::GetIntValueFromSignedBit(uiRightWallType       );
        p->m_clipBoard[i].iRoofType             = CTrack::GetIntValueFromSignedBit(uiRoofType            );
        p->m_clipBoard[i].iLUOuterWallType      = CTrack::GetIntValueFromSignedBit(uiLUOuterWallType     );
        p->m_clipBoard[i].iLLOuterWallType      = CTrack::GetIntValueFromSignedBit(uiLLOuterWallType     );
        p->m_clipBoard[i].iOuterFloorType       = CTrack::GetIntValueFromSignedBit(uiOuterFloorType      );
        p->m_clipBoard[i].iRLOuterWallType      = CTrack::GetIntValueFromSignedBit(uiRLOuterWallType     );
        p->m_clipBoard[i].iRUOuterWallType      = CTrack::GetIntValueFromSignedBit(uiRUOuterWallType     );
        p->m_clipBoard[i].iEnvironmentFloorType = CTrack::GetIntValueFromSignedBit(uiEnvironmentFloorType);
      }
      if (!m_preferences.bPasteSurfaceData) {
        p->m_clipBoard[i].iTrackGrip            = prevChunk.iTrackGrip;
        p->m_clipBoard[i].iLeftShoulderGrip     = prevChunk.iLeftShoulderGrip;
        p->m_clipBoard[i].iRightShoulderGrip    = prevChunk.iRightShoulderGrip;
        uint32 uiLeftSurfaceType      = CTrack::GetSignedBitValueFromInt(p->m_clipBoard[i].iLeftSurfaceType     ) & ~SURFACE_MASK_FLAGS;
        uint32 uiCenterSurfaceType    = CTrack::GetSignedBitValueFromInt(p->m_clipBoard[i].iCenterSurfaceType   ) & ~SURFACE_MASK_FLAGS;
        uint32 uiRightSurfaceType     = CTrack::GetSignedBitValueFromInt(p->m_clipBoard[i].iRightSurfaceType    ) & ~SURFACE_MASK_FLAGS;
        uint32 uiLeftWallType         = CTrack::GetSignedBitValueFromInt(p->m_clipBoard[i].iLeftWallType        ) & ~SURFACE_MASK_FLAGS;
        uint32 uiRightWallType        = CTrack::GetSignedBitValueFromInt(p->m_clipBoard[i].iRightWallType       ) & ~SURFACE_MASK_FLAGS;
        uint32 uiRoofType             = CTrack::GetSignedBitValueFromInt(p->m_clipBoard[i].iRoofType            ) & ~SURFACE_MASK_FLAGS;
        uint32 uiLUOuterWallType      = CTrack::GetSignedBitValueFromInt(p->m_clipBoard[i].iLUOuterWallType     ) & ~SURFACE_MASK_FLAGS;
        uint32 uiLLOuterWallType      = CTrack::GetSignedBitValueFromInt(p->m_clipBoard[i].iLLOuterWallType     ) & ~SURFACE_MASK_FLAGS;
        uint32 uiOuterFloorType       = CTrack::GetSignedBitValueFromInt(p->m_clipBoard[i].iOuterFloorType      ) & ~SURFACE_MASK_FLAGS;
        uint32 uiRLOuterWallType      = CTrack::GetSignedBitValueFromInt(p->m_clipBoard[i].iRLOuterWallType     ) & ~SURFACE_MASK_FLAGS;
        uint32 uiRUOuterWallType      = CTrack::GetSignedBitValueFromInt(p->m_clipBoard[i].iRUOuterWallType     ) & ~SURFACE_MASK_FLAGS;
        uint32 uiEnvironmentFloorType = CTrack::GetSignedBitValueFromInt(p->m_clipBoard[i].iEnvironmentFloorType) & ~SURFACE_MASK_FLAGS;
        uiLeftSurfaceType      |= CTrack::GetSignedBitValueFromInt(prevChunk.iLeftSurfaceType)      & SURFACE_MASK_FLAGS;
        uiCenterSurfaceType    |= CTrack::GetSignedBitValueFromInt(prevChunk.iCenterSurfaceType)    & SURFACE_MASK_FLAGS;
        uiRightSurfaceType     |= CTrack::GetSignedBitValueFromInt(prevChunk.iRightSurfaceType)     & SURFACE_MASK_FLAGS;
        uiLeftWallType         |= CTrack::GetSignedBitValueFromInt(prevChunk.iLeftWallType)         & SURFACE_MASK_FLAGS;
        uiRightWallType        |= CTrack::GetSignedBitValueFromInt(prevChunk.iRightWallType)        & SURFACE_MASK_FLAGS;
        uiRoofType             |= CTrack::GetSignedBitValueFromInt(prevChunk.iRoofType)             & SURFACE_MASK_FLAGS;
        uiLUOuterWallType      |= CTrack::GetSignedBitValueFromInt(prevChunk.iLUOuterWallType)      & SURFACE_MASK_FLAGS;
        uiLLOuterWallType      |= CTrack::GetSignedBitValueFromInt(prevChunk.iLLOuterWallType)      & SURFACE_MASK_FLAGS;
        uiOuterFloorType       |= CTrack::GetSignedBitValueFromInt(prevChunk.iOuterFloorType)       & SURFACE_MASK_FLAGS;
        uiRLOuterWallType      |= CTrack::GetSignedBitValueFromInt(prevChunk.iRLOuterWallType)      & SURFACE_MASK_FLAGS;
        uiRUOuterWallType      |= CTrack::GetSignedBitValueFromInt(prevChunk.iRUOuterWallType)      & SURFACE_MASK_FLAGS;
        uiEnvironmentFloorType |= CTrack::GetSignedBitValueFromInt(prevChunk.iEnvironmentFloorType) & SURFACE_MASK_FLAGS;
        p->m_clipBoard[i].iLeftSurfaceType      = CTrack::GetIntValueFromSignedBit(uiLeftSurfaceType     );
        p->m_clipBoard[i].iCenterSurfaceType    = CTrack::GetIntValueFromSignedBit(uiCenterSurfaceType   );
        p->m_clipBoard[i].iRightSurfaceType     = CTrack::GetIntValueFromSignedBit(uiRightSurfaceType    );
        p->m_clipBoard[i].iLeftWallType         = CTrack::GetIntValueFromSignedBit(uiLeftWallType        );
        p->m_clipBoard[i].iRightWallType        = CTrack::GetIntValueFromSignedBit(uiRightWallType       );
        p->m_clipBoard[i].iRoofType             = CTrack::GetIntValueFromSignedBit(uiRoofType            );
        p->m_clipBoard[i].iLUOuterWallType      = CTrack::GetIntValueFromSignedBit(uiLUOuterWallType     );
        p->m_clipBoard[i].iLLOuterWallType      = CTrack::GetIntValueFromSignedBit(uiLLOuterWallType     );
        p->m_clipBoard[i].iOuterFloorType       = CTrack::GetIntValueFromSignedBit(uiOuterFloorType      );
        p->m_clipBoard[i].iRLOuterWallType      = CTrack::GetIntValueFromSignedBit(uiRLOuterWallType     );
        p->m_clipBoard[i].iRUOuterWallType      = CTrack::GetIntValueFromSignedBit(uiRUOuterWallType     );
        p->m_clipBoard[i].iEnvironmentFloorType = CTrack::GetIntValueFromSignedBit(uiEnvironmentFloorType);
      }
      if (!m_preferences.bPasteAIBehavior) {
        p->m_clipBoard[i].iAIAccuracy = prevChunk.iAIAccuracy;
        p->m_clipBoard[i].iAIMaxSpeed = prevChunk.iAIMaxSpeed;
        p->m_clipBoard[i].iAILine1    = prevChunk.iAILine1;
        p->m_clipBoard[i].iAILine2    = prevChunk.iAILine2;
        p->m_clipBoard[i].iAILine3    = prevChunk.iAILine3;
        p->m_clipBoard[i].iAILine4    = prevChunk.iAILine4;
      }
      if (!m_preferences.bPasteDrawOrder) {
        p->m_clipBoard[i].iDrawOrder1 = 0;
        p->m_clipBoard[i].iDrawOrder2 = 0;
        p->m_clipBoard[i].iDrawOrder3 = 0;
        p->m_clipBoard[i].iUnk37      = 0;
        p->m_clipBoard[i].iUnk38      = 0;
        p->m_clipBoard[i].iUnk39      = 0;
        p->m_clipBoard[i].iUnk40      = 0;
        p->m_clipBoard[i].iUnk41      = 0;
        p->m_clipBoard[i].iUnk42      = 0;
        p->m_clipBoard[i].iUnk43      = 0;
        p->m_clipBoard[i].iUnk44      = 0;
        p->m_clipBoard[i].iUnk45      = 0;
        p->m_clipBoard[i].iUnk46      = 0;
        p->m_clipBoard[i].iUnk47      = 0;
        p->m_clipBoard[i].iUnk48      = 0;
        p->m_clipBoard[i].iUnk49      = 0;
        p->m_clipBoard[i].iUnk50      = 0;
      }
      if (!m_preferences.bPasteAudio) {
        p->m_clipBoard[i].iAudioAboveTrigger = 0;
        p->m_clipBoard[i].iAudioTriggerSpeed = 0;
        p->m_clipBoard[i].iAudioBelowTrigger = 0;
      }
      if (!m_preferences.bPasteSigns) {
        p->m_clipBoard[i].iSignType         = -1;
        p->m_clipBoard[i].iSignHorizOffset  = 0;
        p->m_clipBoard[i].iSignVertOffset   = 0;
        p->m_clipBoard[i].dSignYaw          = 0;
        p->m_clipBoard[i].dSignPitch        = 0;
        p->m_clipBoard[i].dSignRoll         = 0;
        p->m_clipBoard[i].iSignTexture      = 0;
      }
    }

    //delete selected chunks
    if (sbSelChunksTo->value() != sbSelChunksFrom->value())
      OnDeleteChunkClicked();
    //insert new chunks
    for (int i = 0; i < (int)p->m_clipBoard.size(); ++i) {
      GetCurrentTrack()->m_chunkAy.insert(GetCurrentTrack()->m_chunkAy.begin() + i + sbSelChunksFrom->value(), p->m_clipBoard[i]);
    }
  } else {
    //update clipboard
    for (int i = 0; i < (int)p->m_clipBoard.size(); ++i) {
      if (m_preferences.bCopyRelativeYaw)
        p->m_clipBoard[i].dYaw = p->m_clipBoard[i].dYaw + prevChunk.dYaw;
      if (m_preferences.bCopyRelativePitch)
        p->m_clipBoard[i].dPitch = p->m_clipBoard[i].dPitch + prevChunk.dPitch;
      if (m_preferences.bCopyRelativeRoll)
        p->m_clipBoard[i].dRoll = p->m_clipBoard[i].dRoll + prevChunk.dRoll;
      p->m_clipBoard[i].dYaw = MathHelpers::ConstrainAngle(p->m_clipBoard[i].dYaw);
      p->m_clipBoard[i].dPitch = MathHelpers::ConstrainAngle(p->m_clipBoard[i].dPitch);
      p->m_clipBoard[i].dRoll = MathHelpers::ConstrainAngle(p->m_clipBoard[i].dRoll);
    }

    //overwrite data in existing chunks
    int iTrackIdx = sbSelChunksFrom->value(), iClipboardIdx = 0;
    for (; iTrackIdx < sbSelChunksTo->value(); ++iTrackIdx) {
      if (m_preferences.bPasteDirection) {
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iLength               = p->m_clipBoard[iClipboardIdx].iLength             ;
        GetCurrentTrack()->m_chunkAy[iTrackIdx].dYaw                  = p->m_clipBoard[iClipboardIdx].dYaw                ;
        GetCurrentTrack()->m_chunkAy[iTrackIdx].dPitch                = p->m_clipBoard[iClipboardIdx].dPitch              ;
        GetCurrentTrack()->m_chunkAy[iTrackIdx].dRoll                 = p->m_clipBoard[iClipboardIdx].dRoll               ;
      }
      if (m_preferences.bPasteGeometry) {
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iLeftShoulderWidth    = p->m_clipBoard[iClipboardIdx].iLeftShoulderWidth  ;
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iLeftLaneWidth        = p->m_clipBoard[iClipboardIdx].iLeftLaneWidth      ;
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iRightLaneWidth       = p->m_clipBoard[iClipboardIdx].iRightLaneWidth     ;
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iRightShoulderWidth   = p->m_clipBoard[iClipboardIdx].iRightShoulderWidth ;
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iLeftShoulderHeight   = p->m_clipBoard[iClipboardIdx].iLeftShoulderHeight ;
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iRightShoulderHeight  = p->m_clipBoard[iClipboardIdx].iRightShoulderHeight;
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iLUOuterWallHOffset   = p->m_clipBoard[iClipboardIdx].iLUOuterWallHOffset ;
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iLLOuterWallHOffset   = p->m_clipBoard[iClipboardIdx].iLLOuterWallHOffset ;
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iLOuterFloorHOffset   = p->m_clipBoard[iClipboardIdx].iLOuterFloorHOffset ;
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iROuterFloorHOffset   = p->m_clipBoard[iClipboardIdx].iROuterFloorHOffset ;
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iRLOuterWallHOffset   = p->m_clipBoard[iClipboardIdx].iRLOuterWallHOffset ;
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iRUOuterWallHOffset   = p->m_clipBoard[iClipboardIdx].iRUOuterWallHOffset ;
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iLUOuterWallHeight    = p->m_clipBoard[iClipboardIdx].iLUOuterWallHeight  ;
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iLLOuterWallHeight    = p->m_clipBoard[iClipboardIdx].iLLOuterWallHeight  ;
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iLOuterFloorHeight    = p->m_clipBoard[iClipboardIdx].iLOuterFloorHeight  ;
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iROuterFloorHeight    = p->m_clipBoard[iClipboardIdx].iROuterFloorHeight  ;
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iRLOuterWallHeight    = p->m_clipBoard[iClipboardIdx].iRLOuterWallHeight  ;
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iRUOuterWallHeight    = p->m_clipBoard[iClipboardIdx].iRUOuterWallHeight  ;
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iRoofHeight           = p->m_clipBoard[iClipboardIdx].iRoofHeight         ;
      }
      if (m_preferences.bPasteTextures) {
        uint32 uiLeftSurfaceType      = CTrack::GetSignedBitValueFromInt(GetCurrentTrack()->m_chunkAy[iTrackIdx].iLeftSurfaceType     ) & ~SURFACE_MASK_TEXTURE_INDEX;
        uint32 uiCenterSurfaceType    = CTrack::GetSignedBitValueFromInt(GetCurrentTrack()->m_chunkAy[iTrackIdx].iCenterSurfaceType   ) & ~SURFACE_MASK_TEXTURE_INDEX;
        uint32 uiRightSurfaceType     = CTrack::GetSignedBitValueFromInt(GetCurrentTrack()->m_chunkAy[iTrackIdx].iRightSurfaceType    ) & ~SURFACE_MASK_TEXTURE_INDEX;
        uint32 uiLeftWallType         = CTrack::GetSignedBitValueFromInt(GetCurrentTrack()->m_chunkAy[iTrackIdx].iLeftWallType        ) & ~SURFACE_MASK_TEXTURE_INDEX;
        uint32 uiRightWallType        = CTrack::GetSignedBitValueFromInt(GetCurrentTrack()->m_chunkAy[iTrackIdx].iRightWallType       ) & ~SURFACE_MASK_TEXTURE_INDEX;
        uint32 uiRoofType             = CTrack::GetSignedBitValueFromInt(GetCurrentTrack()->m_chunkAy[iTrackIdx].iRoofType            ) & ~SURFACE_MASK_TEXTURE_INDEX;
        uint32 uiLUOuterWallType      = CTrack::GetSignedBitValueFromInt(GetCurrentTrack()->m_chunkAy[iTrackIdx].iLUOuterWallType     ) & ~SURFACE_MASK_TEXTURE_INDEX;
        uint32 uiLLOuterWallType      = CTrack::GetSignedBitValueFromInt(GetCurrentTrack()->m_chunkAy[iTrackIdx].iLLOuterWallType     ) & ~SURFACE_MASK_TEXTURE_INDEX;
        uint32 uiOuterFloorType       = CTrack::GetSignedBitValueFromInt(GetCurrentTrack()->m_chunkAy[iTrackIdx].iOuterFloorType      ) & ~SURFACE_MASK_TEXTURE_INDEX;
        uint32 uiRLOuterWallType      = CTrack::GetSignedBitValueFromInt(GetCurrentTrack()->m_chunkAy[iTrackIdx].iRLOuterWallType     ) & ~SURFACE_MASK_TEXTURE_INDEX;
        uint32 uiRUOuterWallType      = CTrack::GetSignedBitValueFromInt(GetCurrentTrack()->m_chunkAy[iTrackIdx].iRUOuterWallType     ) & ~SURFACE_MASK_TEXTURE_INDEX;
        uint32 uiEnvironmentFloorType = CTrack::GetSignedBitValueFromInt(GetCurrentTrack()->m_chunkAy[iTrackIdx].iEnvironmentFloorType) & ~SURFACE_MASK_TEXTURE_INDEX;
        uiLeftSurfaceType      |= CTrack::GetSignedBitValueFromInt(p->m_clipBoard[iClipboardIdx].iLeftSurfaceType     ) & SURFACE_MASK_TEXTURE_INDEX;
        uiCenterSurfaceType    |= CTrack::GetSignedBitValueFromInt(p->m_clipBoard[iClipboardIdx].iCenterSurfaceType   ) & SURFACE_MASK_TEXTURE_INDEX;
        uiRightSurfaceType     |= CTrack::GetSignedBitValueFromInt(p->m_clipBoard[iClipboardIdx].iRightSurfaceType    ) & SURFACE_MASK_TEXTURE_INDEX;
        uiLeftWallType         |= CTrack::GetSignedBitValueFromInt(p->m_clipBoard[iClipboardIdx].iLeftWallType        ) & SURFACE_MASK_TEXTURE_INDEX;
        uiRightWallType        |= CTrack::GetSignedBitValueFromInt(p->m_clipBoard[iClipboardIdx].iRightWallType       ) & SURFACE_MASK_TEXTURE_INDEX;
        uiRoofType             |= CTrack::GetSignedBitValueFromInt(p->m_clipBoard[iClipboardIdx].iRoofType            ) & SURFACE_MASK_TEXTURE_INDEX;
        uiLUOuterWallType      |= CTrack::GetSignedBitValueFromInt(p->m_clipBoard[iClipboardIdx].iLUOuterWallType     ) & SURFACE_MASK_TEXTURE_INDEX;
        uiLLOuterWallType      |= CTrack::GetSignedBitValueFromInt(p->m_clipBoard[iClipboardIdx].iLLOuterWallType     ) & SURFACE_MASK_TEXTURE_INDEX;
        uiOuterFloorType       |= CTrack::GetSignedBitValueFromInt(p->m_clipBoard[iClipboardIdx].iOuterFloorType      ) & SURFACE_MASK_TEXTURE_INDEX;
        uiRLOuterWallType      |= CTrack::GetSignedBitValueFromInt(p->m_clipBoard[iClipboardIdx].iRLOuterWallType     ) & SURFACE_MASK_TEXTURE_INDEX;
        uiRUOuterWallType      |= CTrack::GetSignedBitValueFromInt(p->m_clipBoard[iClipboardIdx].iRUOuterWallType     ) & SURFACE_MASK_TEXTURE_INDEX;
        uiEnvironmentFloorType |= CTrack::GetSignedBitValueFromInt(p->m_clipBoard[iClipboardIdx].iEnvironmentFloorType) & SURFACE_MASK_TEXTURE_INDEX;
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iLeftSurfaceType      = CTrack::GetIntValueFromSignedBit(uiLeftSurfaceType     );
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iCenterSurfaceType    = CTrack::GetIntValueFromSignedBit(uiCenterSurfaceType   );
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iRightSurfaceType     = CTrack::GetIntValueFromSignedBit(uiRightSurfaceType    );
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iLeftWallType         = CTrack::GetIntValueFromSignedBit(uiLeftWallType        );
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iRightWallType        = CTrack::GetIntValueFromSignedBit(uiRightWallType       );
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iRoofType             = CTrack::GetIntValueFromSignedBit(uiRoofType            );
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iLUOuterWallType      = CTrack::GetIntValueFromSignedBit(uiLUOuterWallType     );
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iLLOuterWallType      = CTrack::GetIntValueFromSignedBit(uiLLOuterWallType     );
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iOuterFloorType       = CTrack::GetIntValueFromSignedBit(uiOuterFloorType      );
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iRLOuterWallType      = CTrack::GetIntValueFromSignedBit(uiRLOuterWallType     );
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iRUOuterWallType      = CTrack::GetIntValueFromSignedBit(uiRUOuterWallType     );
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iEnvironmentFloorType = CTrack::GetIntValueFromSignedBit(uiEnvironmentFloorType);
      }
      if (m_preferences.bPasteSurfaceData) {
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iTrackGrip = p->m_clipBoard[iClipboardIdx].iTrackGrip;
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iLeftShoulderGrip = p->m_clipBoard[iClipboardIdx].iLeftShoulderGrip;
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iRightShoulderGrip = p->m_clipBoard[iClipboardIdx].iRightShoulderGrip;

        uint32 uiLeftSurfaceType      = CTrack::GetSignedBitValueFromInt(GetCurrentTrack()->m_chunkAy[iTrackIdx].iLeftSurfaceType     ) & ~SURFACE_MASK_FLAGS;
        uint32 uiCenterSurfaceType    = CTrack::GetSignedBitValueFromInt(GetCurrentTrack()->m_chunkAy[iTrackIdx].iCenterSurfaceType   ) & ~SURFACE_MASK_FLAGS;
        uint32 uiRightSurfaceType     = CTrack::GetSignedBitValueFromInt(GetCurrentTrack()->m_chunkAy[iTrackIdx].iRightSurfaceType    ) & ~SURFACE_MASK_FLAGS;
        uint32 uiLeftWallType         = CTrack::GetSignedBitValueFromInt(GetCurrentTrack()->m_chunkAy[iTrackIdx].iLeftWallType        ) & ~SURFACE_MASK_FLAGS;
        uint32 uiRightWallType        = CTrack::GetSignedBitValueFromInt(GetCurrentTrack()->m_chunkAy[iTrackIdx].iRightWallType       ) & ~SURFACE_MASK_FLAGS;
        uint32 uiRoofType             = CTrack::GetSignedBitValueFromInt(GetCurrentTrack()->m_chunkAy[iTrackIdx].iRoofType            ) & ~SURFACE_MASK_FLAGS;
        uint32 uiLUOuterWallType      = CTrack::GetSignedBitValueFromInt(GetCurrentTrack()->m_chunkAy[iTrackIdx].iLUOuterWallType     ) & ~SURFACE_MASK_FLAGS;
        uint32 uiLLOuterWallType      = CTrack::GetSignedBitValueFromInt(GetCurrentTrack()->m_chunkAy[iTrackIdx].iLLOuterWallType     ) & ~SURFACE_MASK_FLAGS;
        uint32 uiOuterFloorType       = CTrack::GetSignedBitValueFromInt(GetCurrentTrack()->m_chunkAy[iTrackIdx].iOuterFloorType      ) & ~SURFACE_MASK_FLAGS;
        uint32 uiRLOuterWallType      = CTrack::GetSignedBitValueFromInt(GetCurrentTrack()->m_chunkAy[iTrackIdx].iRLOuterWallType     ) & ~SURFACE_MASK_FLAGS;
        uint32 uiRUOuterWallType      = CTrack::GetSignedBitValueFromInt(GetCurrentTrack()->m_chunkAy[iTrackIdx].iRUOuterWallType     ) & ~SURFACE_MASK_FLAGS;
        uint32 uiEnvironmentFloorType = CTrack::GetSignedBitValueFromInt(GetCurrentTrack()->m_chunkAy[iTrackIdx].iEnvironmentFloorType) & ~SURFACE_MASK_FLAGS;
        uiLeftSurfaceType      |= CTrack::GetSignedBitValueFromInt(p->m_clipBoard[iClipboardIdx].iLeftSurfaceType     ) & SURFACE_MASK_FLAGS;
        uiCenterSurfaceType    |= CTrack::GetSignedBitValueFromInt(p->m_clipBoard[iClipboardIdx].iCenterSurfaceType   ) & SURFACE_MASK_FLAGS;
        uiRightSurfaceType     |= CTrack::GetSignedBitValueFromInt(p->m_clipBoard[iClipboardIdx].iRightSurfaceType    ) & SURFACE_MASK_FLAGS;
        uiLeftWallType         |= CTrack::GetSignedBitValueFromInt(p->m_clipBoard[iClipboardIdx].iLeftWallType        ) & SURFACE_MASK_FLAGS;
        uiRightWallType        |= CTrack::GetSignedBitValueFromInt(p->m_clipBoard[iClipboardIdx].iRightWallType       ) & SURFACE_MASK_FLAGS;
        uiRoofType             |= CTrack::GetSignedBitValueFromInt(p->m_clipBoard[iClipboardIdx].iRoofType            ) & SURFACE_MASK_FLAGS;
        uiLUOuterWallType      |= CTrack::GetSignedBitValueFromInt(p->m_clipBoard[iClipboardIdx].iLUOuterWallType     ) & SURFACE_MASK_FLAGS;
        uiLLOuterWallType      |= CTrack::GetSignedBitValueFromInt(p->m_clipBoard[iClipboardIdx].iLLOuterWallType     ) & SURFACE_MASK_FLAGS;
        uiOuterFloorType       |= CTrack::GetSignedBitValueFromInt(p->m_clipBoard[iClipboardIdx].iOuterFloorType      ) & SURFACE_MASK_FLAGS;
        uiRLOuterWallType      |= CTrack::GetSignedBitValueFromInt(p->m_clipBoard[iClipboardIdx].iRLOuterWallType     ) & SURFACE_MASK_FLAGS;
        uiRUOuterWallType      |= CTrack::GetSignedBitValueFromInt(p->m_clipBoard[iClipboardIdx].iRUOuterWallType     ) & SURFACE_MASK_FLAGS;
        uiEnvironmentFloorType |= CTrack::GetSignedBitValueFromInt(p->m_clipBoard[iClipboardIdx].iEnvironmentFloorType) & SURFACE_MASK_FLAGS;
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iLeftSurfaceType      = CTrack::GetIntValueFromSignedBit(uiLeftSurfaceType     );
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iCenterSurfaceType    = CTrack::GetIntValueFromSignedBit(uiCenterSurfaceType   );
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iRightSurfaceType     = CTrack::GetIntValueFromSignedBit(uiRightSurfaceType    );
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iLeftWallType         = CTrack::GetIntValueFromSignedBit(uiLeftWallType        );
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iRightWallType        = CTrack::GetIntValueFromSignedBit(uiRightWallType       );
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iRoofType             = CTrack::GetIntValueFromSignedBit(uiRoofType            );
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iLUOuterWallType      = CTrack::GetIntValueFromSignedBit(uiLUOuterWallType     );
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iLLOuterWallType      = CTrack::GetIntValueFromSignedBit(uiLLOuterWallType     );
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iOuterFloorType       = CTrack::GetIntValueFromSignedBit(uiOuterFloorType      );
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iRLOuterWallType      = CTrack::GetIntValueFromSignedBit(uiRLOuterWallType     );
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iRUOuterWallType      = CTrack::GetIntValueFromSignedBit(uiRUOuterWallType     );
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iEnvironmentFloorType = CTrack::GetIntValueFromSignedBit(uiEnvironmentFloorType);
      }
      if (m_preferences.bPasteAIBehavior) {
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iAIAccuracy = p->m_clipBoard[iClipboardIdx].iAIAccuracy;
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iAIMaxSpeed = p->m_clipBoard[iClipboardIdx].iAIMaxSpeed;
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iAILine1    = p->m_clipBoard[iClipboardIdx].iAILine1   ;
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iAILine2    = p->m_clipBoard[iClipboardIdx].iAILine2   ;
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iAILine3    = p->m_clipBoard[iClipboardIdx].iAILine3   ;
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iAILine4    = p->m_clipBoard[iClipboardIdx].iAILine4   ;
      }
      if (m_preferences.bPasteDrawOrder) {
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iDrawOrder1 = p->m_clipBoard[iClipboardIdx].iDrawOrder1;
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iDrawOrder2 = p->m_clipBoard[iClipboardIdx].iDrawOrder2;
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iDrawOrder3 = p->m_clipBoard[iClipboardIdx].iDrawOrder3;
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iUnk37      = p->m_clipBoard[iClipboardIdx].iUnk37     ;
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iUnk38      = p->m_clipBoard[iClipboardIdx].iUnk38     ;
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iUnk39      = p->m_clipBoard[iClipboardIdx].iUnk39     ;
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iUnk40      = p->m_clipBoard[iClipboardIdx].iUnk40     ;
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iUnk41      = p->m_clipBoard[iClipboardIdx].iUnk41     ;
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iUnk42      = p->m_clipBoard[iClipboardIdx].iUnk42     ;
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iUnk43      = p->m_clipBoard[iClipboardIdx].iUnk43     ;
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iUnk44      = p->m_clipBoard[iClipboardIdx].iUnk44     ;
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iUnk45      = p->m_clipBoard[iClipboardIdx].iUnk45     ;
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iUnk46      = p->m_clipBoard[iClipboardIdx].iUnk46     ;
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iUnk47      = p->m_clipBoard[iClipboardIdx].iUnk47     ;
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iUnk48      = p->m_clipBoard[iClipboardIdx].iUnk48     ;
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iUnk49      = p->m_clipBoard[iClipboardIdx].iUnk49     ;
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iUnk50      = p->m_clipBoard[iClipboardIdx].iUnk50     ;
      }
      if (m_preferences.bPasteAudio) {
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iAudioAboveTrigger = p->m_clipBoard[iClipboardIdx].iAudioAboveTrigger;
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iAudioTriggerSpeed = p->m_clipBoard[iClipboardIdx].iAudioTriggerSpeed;
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iAudioBelowTrigger = p->m_clipBoard[iClipboardIdx].iAudioBelowTrigger;
      }
      if (m_preferences.bPasteSigns) {
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iSignType        = p->m_clipBoard[iClipboardIdx].iSignType       ;
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iSignHorizOffset = p->m_clipBoard[iClipboardIdx].iSignHorizOffset;
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iSignVertOffset  = p->m_clipBoard[iClipboardIdx].iSignVertOffset ;
        GetCurrentTrack()->m_chunkAy[iTrackIdx].dSignYaw         = p->m_clipBoard[iClipboardIdx].dSignYaw        ;
        GetCurrentTrack()->m_chunkAy[iTrackIdx].dSignPitch       = p->m_clipBoard[iClipboardIdx].dSignPitch      ;
        GetCurrentTrack()->m_chunkAy[iTrackIdx].dSignRoll        = p->m_clipBoard[iClipboardIdx].dSignRoll       ;
        GetCurrentTrack()->m_chunkAy[iTrackIdx].iSignTexture     = p->m_clipBoard[iClipboardIdx].iSignTexture    ;
      }

      ++iClipboardIdx;
      if (iClipboardIdx >= p->m_clipBoard.size())
        iClipboardIdx = 0;
    }
  }

  int iSelect = sbSelChunksFrom->value() + (int)p->m_clipBoard.size();
  BLOCK_SIG_AND_DO(ckTo, setChecked(false));
  BLOCK_SIG_AND_DO(sbSelChunksFrom, setRange(0, (int)GetCurrentTrack()->m_chunkAy.size() - 1));
  BLOCK_SIG_AND_DO(sbSelChunksTo, setRange(0, (int)GetCurrentTrack()->m_chunkAy.size() - 1));
  BLOCK_SIG_AND_DO(sbSelChunksFrom, setValue(iSelect));
  BLOCK_SIG_AND_DO(sbSelChunksTo, setValue(iSelect));
  GetCurrentPreview()->m_bUnsavedChanges = true;
  GetCurrentPreview()->SaveHistory("Pasted " + QString::number(p->m_clipBoard.size()) + " geometry chunks");
  g_pMainWindow->UpdateWindow();
}

//-------------------------------------------------------------------------------------------------

void CMainWindow::OnSelectAll()
{
  BLOCK_SIG_AND_DO(sbSelChunksFrom, setValue(0));
  ckTo->setChecked(true);
  BLOCK_SIG_AND_DO(sbSelChunksTo, setValue((int)GetCurrentTrack()->m_chunkAy.size() - 1));
  UpdateGeometrySelection();
}

//-------------------------------------------------------------------------------------------------

void CMainWindow::OnMirror()
{
  if (!GetCurrentTrack())
    return;

  if (sbSelChunksTo->value() > (int)GetCurrentTrack()->m_chunkAy.size()
      || sbSelChunksFrom->value() > (int)GetCurrentTrack()->m_chunkAy.size())
    return;

  //store all relative yaw values
  std::vector<double> relativeYawAy;
  relativeYawAy.push_back(0.0);
  for (int i = 1; i < (int)GetCurrentTrack()->m_chunkAy.size(); ++i) {
    double dRelativeYaw = GetCurrentTrack()->m_chunkAy[i].dYaw - GetCurrentTrack()->m_chunkAy[i - 1].dYaw;
    relativeYawAy.push_back(dRelativeYaw);
  }

  for (int i = sbSelChunksFrom->value(); i <= sbSelChunksTo->value(); ++i) {
    //invert relative yaw of selected chunks
    relativeYawAy[i] = MathHelpers::ConstrainAngle(relativeYawAy[i] * -1);

    //invert values of selected chunks
    GetCurrentTrack()->m_chunkAy[i].dYaw = MathHelpers::ConstrainAngle(GetCurrentTrack()->m_chunkAy[i].dYaw * -1);
    GetCurrentTrack()->m_chunkAy[i].dRoll = MathHelpers::ConstrainAngle(GetCurrentTrack()->m_chunkAy[i].dRoll * -1);
    GetCurrentTrack()->m_chunkAy[i].iSignHorizOffset = GetCurrentTrack()->m_chunkAy[i].iSignHorizOffset * -1;
    GetCurrentTrack()->m_chunkAy[i].dSignYaw = MathHelpers::ConstrainAngle(GetCurrentTrack()->m_chunkAy[i].dSignYaw * -1);
    GetCurrentTrack()->m_chunkAy[i].dSignRoll = MathHelpers::ConstrainAngle(GetCurrentTrack()->m_chunkAy[i].dSignRoll * -1);

    //store left side values
    int iLeftShoulderWidth  = GetCurrentTrack()->m_chunkAy[i].iLeftShoulderWidth;
    int iLeftLaneWidth      = GetCurrentTrack()->m_chunkAy[i].iLeftLaneWidth;
    int iLeftShoulderHeight = GetCurrentTrack()->m_chunkAy[i].iLeftShoulderHeight;
    int iLeftShoulderGrip   = GetCurrentTrack()->m_chunkAy[i].iLeftShoulderGrip;
    int iLUOuterWallHOffset = GetCurrentTrack()->m_chunkAy[i].iLUOuterWallHOffset;
    int iLLOuterWallHOffset = GetCurrentTrack()->m_chunkAy[i].iLLOuterWallHOffset;
    int iLOuterFloorHOffset = GetCurrentTrack()->m_chunkAy[i].iLOuterFloorHOffset;
    int iLUOuterWallHeight  = GetCurrentTrack()->m_chunkAy[i].iLUOuterWallHeight;
    int iLLOuterWallHeight  = GetCurrentTrack()->m_chunkAy[i].iLLOuterWallHeight;
    int iLOuterFloorHeight  = GetCurrentTrack()->m_chunkAy[i].iLOuterFloorHeight;
    int iLeftSurfaceType    = GetCurrentTrack()->m_chunkAy[i].iLeftSurfaceType;
    int iLeftWallType       = GetCurrentTrack()->m_chunkAy[i].iLeftWallType;
    int iLUOuterWallType    = GetCurrentTrack()->m_chunkAy[i].iLUOuterWallType;
    int iLLOuterWallType    = GetCurrentTrack()->m_chunkAy[i].iLLOuterWallType;
    //apply right side values to left side
    GetCurrentTrack()->m_chunkAy[i].iLeftShoulderWidth = GetCurrentTrack()->m_chunkAy[i].iRightShoulderWidth;
    GetCurrentTrack()->m_chunkAy[i].iLeftLaneWidth = GetCurrentTrack()->m_chunkAy[i].iRightLaneWidth;
    GetCurrentTrack()->m_chunkAy[i].iLeftShoulderHeight = GetCurrentTrack()->m_chunkAy[i].iRightShoulderHeight;
    GetCurrentTrack()->m_chunkAy[i].iLeftShoulderGrip = GetCurrentTrack()->m_chunkAy[i].iRightShoulderGrip;
    GetCurrentTrack()->m_chunkAy[i].iLUOuterWallHOffset = GetCurrentTrack()->m_chunkAy[i].iRUOuterWallHOffset;
    GetCurrentTrack()->m_chunkAy[i].iLLOuterWallHOffset = GetCurrentTrack()->m_chunkAy[i].iRLOuterWallHOffset;
    GetCurrentTrack()->m_chunkAy[i].iLOuterFloorHOffset = GetCurrentTrack()->m_chunkAy[i].iROuterFloorHOffset;
    GetCurrentTrack()->m_chunkAy[i].iLUOuterWallHeight = GetCurrentTrack()->m_chunkAy[i].iRUOuterWallHeight;
    GetCurrentTrack()->m_chunkAy[i].iLLOuterWallHeight = GetCurrentTrack()->m_chunkAy[i].iRLOuterWallHeight;
    GetCurrentTrack()->m_chunkAy[i].iLOuterFloorHeight = GetCurrentTrack()->m_chunkAy[i].iROuterFloorHeight;
    GetCurrentTrack()->m_chunkAy[i].iLeftSurfaceType = MirrorSurfaceType(GetCurrentTrack()->m_chunkAy[i].iRightSurfaceType);
    GetCurrentTrack()->m_chunkAy[i].iLeftWallType    = MirrorSurfaceType(GetCurrentTrack()->m_chunkAy[i].iRightWallType);
    GetCurrentTrack()->m_chunkAy[i].iLUOuterWallType = MirrorSurfaceType(GetCurrentTrack()->m_chunkAy[i].iRUOuterWallType);
    GetCurrentTrack()->m_chunkAy[i].iLLOuterWallType = MirrorSurfaceType(GetCurrentTrack()->m_chunkAy[i].iRLOuterWallType);
    //apply left side values to right side
    GetCurrentTrack()->m_chunkAy[i].iRightShoulderWidth   = iLeftShoulderWidth ;
    GetCurrentTrack()->m_chunkAy[i].iRightLaneWidth       = iLeftLaneWidth     ;
    GetCurrentTrack()->m_chunkAy[i].iRightShoulderHeight  = iLeftShoulderHeight;
    GetCurrentTrack()->m_chunkAy[i].iRightShoulderGrip    = iLeftShoulderGrip  ;
    GetCurrentTrack()->m_chunkAy[i].iRUOuterWallHOffset   = iLUOuterWallHOffset;
    GetCurrentTrack()->m_chunkAy[i].iRLOuterWallHOffset   = iLLOuterWallHOffset;
    GetCurrentTrack()->m_chunkAy[i].iROuterFloorHOffset   = iLOuterFloorHOffset;
    GetCurrentTrack()->m_chunkAy[i].iRUOuterWallHeight    = iLUOuterWallHeight ;
    GetCurrentTrack()->m_chunkAy[i].iRLOuterWallHeight    = iLLOuterWallHeight ;
    GetCurrentTrack()->m_chunkAy[i].iROuterFloorHeight    = iLOuterFloorHeight ;
    GetCurrentTrack()->m_chunkAy[i].iRightSurfaceType     = MirrorSurfaceType(iLeftSurfaceType);
    GetCurrentTrack()->m_chunkAy[i].iRightWallType        = MirrorSurfaceType(iLeftWallType   );
    GetCurrentTrack()->m_chunkAy[i].iRUOuterWallType      = MirrorSurfaceType(iLUOuterWallType);
    GetCurrentTrack()->m_chunkAy[i].iRLOuterWallType      = MirrorSurfaceType(iLLOuterWallType);
  }

  //apply yaw updates to entire track
  for (int i = 1; i < (int)GetCurrentTrack()->m_chunkAy.size(); ++i) {
    GetCurrentTrack()->m_chunkAy[i].dYaw = MathHelpers::ConstrainAngle(GetCurrentTrack()->m_chunkAy[i - 1].dYaw + relativeYawAy[i]);
  }

  GetCurrentPreview()->m_bUnsavedChanges = true;
  GetCurrentPreview()->SaveHistory("Pasted " + QString::number(p->m_clipBoard.size()) + " geometry chunks");
  g_pMainWindow->UpdateWindow();
}

//-------------------------------------------------------------------------------------------------

int CMainWindow::MirrorSurfaceType(int iSurfaceType)
{
  if (iSurfaceType != -1) {
    uint32 uiSurfaceType = CTrack::GetSignedBitValueFromInt(iSurfaceType);
    if (uiSurfaceType & SURFACE_FLAG_FLIP_VERT)
      uiSurfaceType &= ~SURFACE_FLAG_FLIP_VERT;
    else
      uiSurfaceType |= SURFACE_FLAG_FLIP_VERT;
    return CTrack::GetIntValueFromSignedBit(uiSurfaceType);
  }
  return iSurfaceType;
}

//-------------------------------------------------------------------------------------------------

void CMainWindow::OnDeselect()
{
  ckTo->setChecked(false);
}

//-------------------------------------------------------------------------------------------------

void CMainWindow::OnBacks()
{
  if (GetCurrentTrack()) {
    CAssignBacksDialog dlg(this, GetCurrentTrack());
    if (dlg.exec()) {
      if (GetCurrentPreview()) {
        GetCurrentPreview()->m_bUnsavedChanges = true;
        GetCurrentPreview()->SaveHistory("Assigned backs");
      }
      UpdateWindow();
    }
  }
}

//-------------------------------------------------------------------------------------------------

void CMainWindow::OnPreferences()
{
  CPreferencesDialog dlg(this);
  dlg.exec();
}

//-------------------------------------------------------------------------------------------------

void CMainWindow::OnDebug()
{
  p->m_logDialog.raise();
  p->m_logDialog.show();
  p->m_logDialog.setFocus();
  p->m_logDialog.resize(QDesktopWidget().availableGeometry(this).size() * 0.3);
}

//-------------------------------------------------------------------------------------------------

void CMainWindow::OnAbout()
{
  QMessageBox::information(this, "Git Gud", "Click to pan. WASD to move. R or E is up, F or Q is down.");
}

//-------------------------------------------------------------------------------------------------

void CMainWindow::OnTabCloseRequested(int iIndex)
{
  if (iIndex > p->m_previewAy.size()) return;

  twViewer->blockSignals(true);
  if (p->m_previewAy[iIndex]->SaveChangesAndContinue()) {
    p->m_previewAy[iIndex]->makeCurrent();
    delete p->m_previewAy[iIndex];
    p->m_previewAy.erase(p->m_previewAy.begin() + iIndex);
  }
  twViewer->blockSignals(false);

  OnTabChanged(twViewer->currentIndex());
}

//-------------------------------------------------------------------------------------------------

void CMainWindow::OnTabChanged(int iIndex)
{
  if (iIndex > p->m_previewAy.size()) return;

  eWhipModel carModel;
  eShapeSection aiLine;
  bool bMillionPlus;
  uint32 uiShowModels = p->m_pDisplaySettings->GetDisplaySettings(carModel, aiLine, bMillionPlus);
  p->m_previewAy[iIndex]->makeCurrent();
  p->m_previewAy[iIndex]->ShowModels(uiShowModels);
  p->m_previewAy[iIndex]->UpdateCar(carModel, aiLine, bMillionPlus);
  p->m_previewAy[iIndex]->AttachLast(p->m_pDisplaySettings->GetAttachLast());
  BLOCK_SIG_AND_DO(sbSelChunksFrom, setRange(0, (int)GetCurrentTrack()->m_chunkAy.size() - 1));
  BLOCK_SIG_AND_DO(sbSelChunksTo, setRange(0, (int)GetCurrentTrack()->m_chunkAy.size() - 1));
  BLOCK_SIG_AND_DO(sbSelChunksFrom, setValue(p->m_previewAy[iIndex]->m_iSelFrom));
  BLOCK_SIG_AND_DO(sbSelChunksTo, setValue(p->m_previewAy[iIndex]->m_iSelTo));
  BLOCK_SIG_AND_DO(ckTo, setChecked(p->m_previewAy[iIndex]->m_bToChecked));

  UpdateWindow();
}

//-------------------------------------------------------------------------------------------------

void CMainWindow::OnSelChunksFromChanged(int iValue)
{
  if (!ckTo->isChecked() || sbSelChunksTo->value() < iValue) {
    BLOCK_SIG_AND_DO(sbSelChunksTo, setValue(iValue));
  }
  UpdateGeometrySelection();
}

//-------------------------------------------------------------------------------------------------

void CMainWindow::OnSelChunksToChanged(int iValue)
{
  if (sbSelChunksFrom->value() > iValue) {
    BLOCK_SIG_AND_DO(sbSelChunksFrom, setValue(iValue));
  }
  UpdateGeometrySelection();
}

//-------------------------------------------------------------------------------------------------

void CMainWindow::OnToChecked(bool bChecked)
{
  sbSelChunksTo->setEnabled(bChecked);
  if (!bChecked) {
    sbSelChunksTo->setValue(sbSelChunksFrom->value());
  }
}

//-------------------------------------------------------------------------------------------------

void CMainWindow::OnDeleteChunkClicked()
{
  if (!GetCurrentTrack() || GetCurrentTrack()->m_chunkAy.empty()) return;
  if (sbSelChunksFrom->value() > sbSelChunksTo->value() || sbSelChunksTo->value() > GetCurrentTrack()->m_chunkAy.size()) {
    assert(0);
    return;
  }

  GetCurrentTrack()->m_chunkAy.erase(
    GetCurrentTrack()->m_chunkAy.begin() + sbSelChunksFrom->value(),
    GetCurrentTrack()->m_chunkAy.begin() + sbSelChunksTo->value() + 1);

  GetCurrentPreview()->m_bUnsavedChanges = true;
  GetCurrentPreview()->SaveHistory("Deleted geometry chunk");
  g_pMainWindow->UpdateWindow();
  BLOCK_SIG_AND_DO(sbSelChunksTo, setValue(sbSelChunksFrom->value()));
  BLOCK_SIG_AND_DO(ckTo, setChecked(false));
  UpdateGeometrySelection();
}

//-------------------------------------------------------------------------------------------------

void CMainWindow::OnAddChunkClicked()
{
  if (!GetCurrentTrack()) return;

  int iSelPos = sbSelChunksFrom->value();
  tGeometryChunk newChunk;
  if (iSelPos < GetCurrentTrack()->m_chunkAy.size())
    newChunk = GetCurrentTrack()->m_chunkAy[iSelPos];
  else
    memset(&newChunk, 0, sizeof(newChunk));

  if (GetCurrentTrack()->m_chunkAy.empty())
    GetCurrentTrack()->m_chunkAy.push_back(newChunk);
  else
    GetCurrentTrack()->m_chunkAy.insert(GetCurrentTrack()->m_chunkAy.begin() + iSelPos + 1, newChunk);

  GetCurrentPreview()->m_bUnsavedChanges = true;
  GetCurrentPreview()->SaveHistory("Added geometry chunk");
  g_pMainWindow->UpdateWindow();
  BLOCK_SIG_AND_DO(sbSelChunksTo, setValue(iSelPos + 1));
  BLOCK_SIG_AND_DO(sbSelChunksFrom, setValue(iSelPos + 1));
}

//-------------------------------------------------------------------------------------------------

void CMainWindow::OnAttachLast(bool bChecked)
{
  if (!GetCurrentPreview()) return;

  GetCurrentPreview()->AttachLast(bChecked);
}

//-------------------------------------------------------------------------------------------------

void CMainWindow::OnOpenReferenceModel()
{
  if (!GetCurrentPreview()) return;

  GetCurrentPreview()->OpenReferenceModel();
}

//-------------------------------------------------------------------------------------------------

void CMainWindow::OnUpdatePreview()
{
  if (!GetCurrentPreview()) return;

  eWhipModel carModel;
  eShapeSection aiLine;
  bool bMillionPlus;
  uint32 uiShowModels = p->m_pDisplaySettings->GetDisplaySettings(carModel, aiLine, bMillionPlus);
  GetCurrentPreview()->ShowModels(uiShowModels);
  GetCurrentPreview()->UpdateCar(carModel, aiLine, bMillionPlus);
}

//-------------------------------------------------------------------------------------------------

void CMainWindow::OnSaveHistoryTimer()
{
  if (GetCurrentPreview())
    GetCurrentPreview()->SaveHistory(m_sHistoryDescription);
}

//-------------------------------------------------------------------------------------------------

void CMainWindow::OnStuntTimer()
{
  eWhipModel carModel;
  eShapeSection aiLine;
  bool bMillionPlus;
  if (p && p->m_pDisplaySettings && p->m_pDisplaySettings->GetDisplaySettings(carModel, aiLine, bMillionPlus) & ANIMATE_STUNTS) {
    if (GetCurrentTrack())
      GetCurrentTrack()->UpdateStunts();
    if (GetCurrentPreview())
      GetCurrentPreview()->UpdateTrack(true);
  }
}

//-------------------------------------------------------------------------------------------------

void CMainWindow::OnZeroTimer()
{
  CGameClock::GetGameClock().NewFrame();
  CGameInput::GetGameInput().Update();
  if (GetCurrentPreview())
    GetCurrentPreview()->UpdateCameraPos();
}

//-------------------------------------------------------------------------------------------------

void CMainWindow::LoadSettings()
{
  QSettings settings(m_sSettingsFile, QSettings::IniFormat);
  m_sLastTrackFilesFolder = settings.value("track_folder", m_sLastTrackFilesFolder).toString();

  //window geometry
  if (settings.contains("window_geometry") && settings.contains("window_state")) {
    QByteArray geometry = saveGeometry();
    QByteArray state = saveState();
    geometry = settings.value("window_geometry", geometry).toByteArray();
    state = settings.value("window_state", state).toByteArray();
    restoreGeometry(geometry);
    restoreState(state);
  } else {
    move(10, 10);
    resize(QDesktopWidget().availableGeometry(this).size() * 0.8);
  }

  //setup dock widgets
  if (settings.contains("show_debug_data")
      && settings.contains("show_global_settings")
      && settings.contains("show_edit_series")
      && settings.contains("show_display_settings")
      && settings.contains("show_edit_geometry")
      && settings.contains("show_edit_sign")
      && settings.contains("show_edit_audio")
      && settings.contains("show_edit_stunt")) {
    bool bShowDebugData = false;
    bool bShowGlobalSettings = false;
    bool bShowEditSeries = false;
    bool bShowDisplaySettings = false;
    bool bShowEditGeometry = false;
    bool bShowEditSign = false;
    bool bShowEditAudio = false;
    bool bShowEditStunt = false;
    bShowDebugData = settings.value("show_debug_data", bShowDebugData).toBool();
    bShowGlobalSettings = settings.value("show_global_settings", bShowGlobalSettings).toBool();
    bShowEditSeries = settings.value("show_edit_series", bShowEditSeries).toBool();
    bShowDisplaySettings = settings.value("show_display_settings", bShowDisplaySettings).toBool();
    bShowEditGeometry = settings.value("show_edit_geometry", bShowEditGeometry).toBool();
    bShowEditSign = settings.value("show_edit_sign", bShowEditSign).toBool();
    bShowEditAudio = settings.value("show_edit_audio", bShowEditAudio).toBool();
    bShowEditStunt = settings.value("show_edit_stunt", bShowEditStunt).toBool();
    p->m_pDebugDataDockWidget->setVisible(bShowDebugData);
    p->m_pGlobalSettingsDockWidget->setVisible(bShowGlobalSettings);
    p->m_pEditSeriesDockWidget->setVisible(bShowEditSeries);
    p->m_pDisplaySettingsDockWidget->setVisible(bShowDisplaySettings);
    p->m_pEditGeometryDockWidget->setVisible(bShowEditGeometry);
    p->m_pEditSignDockWidget->setVisible(bShowEditSign);
    p->m_pEditAudioDockWidget->setVisible(bShowEditAudio);
    p->m_pEditStuntDockWidget->setVisible(bShowEditStunt);
    restoreDockWidget(p->m_pDebugDataDockWidget);
    restoreDockWidget(p->m_pGlobalSettingsDockWidget);
    restoreDockWidget(p->m_pEditSeriesDockWidget);
    restoreDockWidget(p->m_pDisplaySettingsDockWidget);
    restoreDockWidget(p->m_pEditGeometryDockWidget);
    restoreDockWidget(p->m_pEditSignDockWidget);
    restoreDockWidget(p->m_pEditAudioDockWidget);
    restoreDockWidget(p->m_pEditStuntDockWidget);
  } else {
    addDockWidget(Qt::LeftDockWidgetArea, p->m_pDebugDataDockWidget);
    addDockWidget(Qt::RightDockWidgetArea, p->m_pGlobalSettingsDockWidget);
    addDockWidget(Qt::RightDockWidgetArea, p->m_pEditSeriesDockWidget);
    addDockWidget(Qt::RightDockWidgetArea, p->m_pDisplaySettingsDockWidget);
    addDockWidget(Qt::LeftDockWidgetArea, p->m_pEditGeometryDockWidget);
    addDockWidget(Qt::RightDockWidgetArea, p->m_pEditSignDockWidget);
    addDockWidget(Qt::RightDockWidgetArea, p->m_pEditAudioDockWidget);
    addDockWidget(Qt::RightDockWidgetArea, p->m_pEditStuntDockWidget);
    p->m_pDebugDataDockWidget->setVisible(false);
    p->m_pGlobalSettingsDockWidget->setVisible(false);
    p->m_pEditSeriesDockWidget->setVisible(false);
    p->m_pDisplaySettingsDockWidget->setVisible(false);
    p->m_pEditGeometryDockWidget->setVisible(false);
    p->m_pEditSignDockWidget->setVisible(false);
    p->m_pEditAudioDockWidget->setVisible(false);
    p->m_pEditStuntDockWidget->setVisible(false);
  }

  //get default display settings
  eWhipModel carModel;
  eShapeSection aiLine;
  bool bMillionPlus;
  uint32 uiShowModels = p->m_pDisplaySettings->GetDisplaySettings(carModel, aiLine, bMillionPlus);
  bool bAttachLast = p->m_pDisplaySettings->GetAttachLast();
  int iCameraSpeed = (int)CNoclipComponent::s_fMovementSpeed;
  //load display settings
  uiShowModels = settings.value("show_models", uiShowModels).toUInt();
  carModel = (eWhipModel)settings.value("car_model", (int)carModel).toInt();
  aiLine = (eShapeSection)settings.value("car_pos", (int)aiLine).toInt();
  bMillionPlus = settings.value("wrong_way", bMillionPlus).toBool();
  bAttachLast = settings.value("attach_last", bAttachLast).toBool();
  iCameraSpeed = settings.value("camera_speed", iCameraSpeed).toInt();
  //apply display settings
  p->m_pDisplaySettings->SetDisplaySettings(uiShowModels, carModel, aiLine, bMillionPlus);
  p->m_pDisplaySettings->SetAttachLast(bAttachLast);
  p->m_pDisplaySettings->SetCameraSpeed(iCameraSpeed);

  //preferences
  m_preferences.iHistoryMaxSize = settings.value("history_max_size", m_preferences.iHistoryMaxSize).toInt();
  m_preferences.bPasteNewChunks = settings.value("paste_new_chunks", m_preferences.bPasteNewChunks).toBool();
  m_preferences.bCopyRelativeYaw = settings.value("copy_relative_yaw", m_preferences.bCopyRelativeYaw).toBool();
  m_preferences.bCopyRelativePitch = settings.value("copy_relative_pitch", m_preferences.bCopyRelativePitch).toBool();
  m_preferences.bCopyRelativeRoll = settings.value("copy_relative_roll", m_preferences.bCopyRelativeRoll).toBool();
  m_preferences.bPasteDirection = settings.value("paste_direction", m_preferences.bPasteDirection).toBool();
  m_preferences.bPasteGeometry = settings.value("paste_geometry", m_preferences.bPasteGeometry).toBool();
  m_preferences.bPasteTextures = settings.value("paste_textures", m_preferences.bPasteTextures).toBool();
  m_preferences.bPasteSurfaceData = settings.value("paste_surface", m_preferences.bPasteSurfaceData).toBool();
  m_preferences.bPasteAIBehavior = settings.value("paste_ai", m_preferences.bPasteAIBehavior).toBool();
  m_preferences.bPasteDrawOrder = settings.value("paste_draw_order", m_preferences.bPasteDrawOrder).toBool();
  m_preferences.bPasteSigns = settings.value("paste_signs", m_preferences.bPasteSigns).toBool();
  m_preferences.bPasteAudio = settings.value("paste_audio", m_preferences.bPasteAudio).toBool();

  show();
}

//-------------------------------------------------------------------------------------------------

void CMainWindow::SaveSettings()
{
  QSettings settings(m_sSettingsFile, QSettings::IniFormat);
  settings.setValue("track_folder", m_sLastTrackFilesFolder);

  eWhipModel carModel;
  eShapeSection aiLine;
  bool bMillionPlus;

  QByteArray geometry = saveGeometry();
  QByteArray state = saveState();
  settings.setValue("window_geometry", geometry);
  settings.setValue("window_state", state);
  settings.setValue("show_debug_data", p->m_pDebugDataDockWidget->isVisible());
  settings.setValue("show_global_settings", p->m_pGlobalSettingsDockWidget->isVisible());
  settings.setValue("show_edit_series", p->m_pEditSeriesDockWidget->isVisible());
  settings.setValue("show_display_settings", p->m_pDisplaySettingsDockWidget->isVisible());
  settings.setValue("show_edit_geometry", p->m_pEditGeometryDockWidget->isVisible());
  settings.setValue("show_edit_sign", p->m_pEditSignDockWidget->isVisible());
  settings.setValue("show_edit_audio", p->m_pEditAudioDockWidget->isVisible());
  settings.setValue("show_edit_stunt", p->m_pEditStuntDockWidget->isVisible());
  settings.setValue("show_models", p->m_pDisplaySettings->GetDisplaySettings(carModel, aiLine, bMillionPlus));
  settings.setValue("car_model", (int)carModel);
  settings.setValue("car_pos", (int)aiLine);
  settings.setValue("wrong_way", bMillionPlus);
  settings.setValue("attach_last", p->m_pDisplaySettings->GetAttachLast());
  settings.setValue("camera_speed", p->m_pDisplaySettings->GetCameraSpeed());
  settings.setValue("history_max_size", m_preferences.iHistoryMaxSize);
  settings.setValue("paste_new_chunks", m_preferences.bPasteNewChunks);
  settings.setValue("copy_relative_yaw", m_preferences.bCopyRelativeYaw);
  settings.setValue("copy_relative_pitch", m_preferences.bCopyRelativePitch);
  settings.setValue("copy_relative_roll", m_preferences.bCopyRelativeRoll);
  settings.setValue("paste_direction", m_preferences.bPasteDirection);
  settings.setValue("paste_geometry", m_preferences.bPasteGeometry);
  settings.setValue("paste_textures", m_preferences.bPasteTextures);
  settings.setValue("paste_surface", m_preferences.bPasteSurfaceData);
  settings.setValue("paste_ai", m_preferences.bPasteAIBehavior);
  settings.setValue("paste_draw_order", m_preferences.bPasteDrawOrder);
  settings.setValue("paste_signs", m_preferences.bPasteSigns);
  settings.setValue("paste_audio", m_preferences.bPasteAudio);
}

//-------------------------------------------------------------------------------------------------

bool CMainWindow::SaveChangesAndContinue()
{
  bool bRet = true;
  for (int i = 0; i < p->m_previewAy.size(); ++i)
    bRet &= p->m_previewAy[i]->SaveChangesAndContinue();

  UpdateWindow();
  return bRet;
}

//-------------------------------------------------------------------------------------------------

void CMainWindow::UpdateWindow(bool bUpdatingTextures)
{
  //update title bar
  QString sTitle = "Track Editor";
  QString sCurrentTab = "";
  for (int i = 0; i < (int)p->m_previewAy.size(); ++i) {
    if (i == twViewer->currentIndex())
      sCurrentTab = p->m_previewAy[i]->GetTitle(true) + " - ";
    twViewer->setTabText(i, p->m_previewAy[i]->GetTitle(false));
  }
  setWindowTitle(sCurrentTab + sTitle);

  if (GetCurrentTrack())
    GetCurrentTrack()->GenerateTrackMath();

  if (GetCurrentPreview()) {
    if (bUpdatingTextures)
      GetCurrentPreview()->DeleteEnvirFloor();
    GetCurrentPreview()->UpdateTrack();

    BLOCK_SIG_AND_DO(sbSelChunksFrom, setRange(0, (int)GetCurrentTrack()->m_chunkAy.size() - 1));
    BLOCK_SIG_AND_DO(sbSelChunksTo, setRange(0, (int)GetCurrentTrack()->m_chunkAy.size() - 1));
    leChunkCount->setText(QString::number(GetCurrentTrack()->m_chunkAy.size()));
    lblChunkWarning->setVisible(GetCurrentTrack()->m_chunkAy.size() >= 500);
    lblStuntWarning->setVisible(GetCurrentTrack()->HasPitchedStunt());
  }
  UpdateGeometrySelection();
  emit UpdateWindowSig();
}

//-------------------------------------------------------------------------------------------------

void CMainWindow::UpdateGeometrySelection()
{
  if (GetCurrentPreview()) {
    GetCurrentPreview()->m_iSelFrom = sbSelChunksFrom->value();
    GetCurrentPreview()->m_iSelTo = sbSelChunksTo->value();
    GetCurrentPreview()->m_bToChecked = ckTo->isChecked();
    GetCurrentPreview()->UpdateGeometrySelection();
  }
  emit UpdateGeometrySelectionSig(sbSelChunksFrom->value(), sbSelChunksTo->value());
}

//-------------------------------------------------------------------------------------------------

void CMainWindow::InsertUIUpdate(int iInsertVal)
{
  if (GetCurrentTrack()) {
    BLOCK_SIG_AND_DO(sbSelChunksFrom, setRange(0, (int)GetCurrentTrack()->m_chunkAy.size() - 1));
    BLOCK_SIG_AND_DO(sbSelChunksTo, setRange(0, (int)GetCurrentTrack()->m_chunkAy.size() - 1));
  }
  BLOCK_SIG_AND_DO(sbSelChunksTo, setValue(sbSelChunksFrom->value() + iInsertVal - 1));
  BLOCK_SIG_AND_DO(ckTo, setChecked(iInsertVal > 1));
}

//-------------------------------------------------------------------------------------------------

int CMainWindow::GetSelFrom()
{
  return sbSelChunksFrom->value();
}

//-------------------------------------------------------------------------------------------------

int CMainWindow::GetSelTo()
{
  return sbSelChunksTo->value();
}

//-------------------------------------------------------------------------------------------------

CTrack *CMainWindow::GetCurrentTrack()
{
  if (!GetCurrentPreview()) return NULL;
  return GetCurrentPreview()->GetTrack();
}

//-------------------------------------------------------------------------------------------------

CTrackPreview *CMainWindow::GetCurrentPreview()
{
  if (p->m_previewAy.empty() || twViewer->count() == 0 || twViewer->currentIndex() >= (int)p->m_previewAy.size()) return NULL;
  return p->m_previewAy[twViewer->currentIndex()];
}

//-------------------------------------------------------------------------------------------------
