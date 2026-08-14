#include "GlobalTrackSettings.h"
#include "Track.h"
#include "MainWindow.h"
#include "QtHelpers.h"
#include "editor_track_loader.h"
#include "qdir.h"
#include "qfileinfo.h"
//-------------------------------------------------------------------------------------------------
#if defined(_DEBUG) && defined(IS_WINDOWS)
#define new new(_CLIENT_BLOCK, __FILE__, __LINE__)
#endif
//-------------------------------------------------------------------------------------------------
class CGlobalTrackSettingsPrivate
{
public:
  CGlobalTrackSettingsPrivate()
  {
  };
  ~CGlobalTrackSettingsPrivate()
  {
  };

  //header values
  QString sFloorDepth;

  //selected texture values
  QString sTex;
  QString sBld;
  QString sBackVal;

  //selected info values
  QString sTrackNumber;
  QString sImpossibleLaps;
  QString sHardLaps;
  QString sTrickyLaps;
  QString sMediumLaps;
  QString sEasyLaps;
  QString sGirlieLaps;
  QString sTrackMapSize;
  QString sTrackMapFidelity;
  QString sPreviewSize;
};
//-------------------------------------------------------------------------------------------------

CGlobalTrackSettings::CGlobalTrackSettings(QWidget *pParent)
  : QWidget(pParent)
{
  p = new CGlobalTrackSettingsPrivate;
  setupUi(this);

  const int iMaxAssetNameLength =
      static_cast<int>(ED_TRACK_ASSET_NAME_CAPACITY - 1u);
  leTex->setMaxLength(iMaxAssetNameLength);
  leBld->setMaxLength(iMaxAssetNameLength);
  lblNoTex->setVisible(false);

  connect(g_pMainWindow, &CMainWindow::UpdateWindowSig, this, &CGlobalTrackSettings::OnUpdateWindow);

  connect(pbApplyInfo, &QPushButton::clicked, this, &CGlobalTrackSettings::OnApplyInfoClicked);
  connect(pbRevertInfo, &QPushButton::clicked, this, &CGlobalTrackSettings::OnCancelInfoClicked);

  connect(leFloorDepth, &QLineEdit::textChanged, this, &CGlobalTrackSettings::UpdateInfoEditMode);
  connect(leTex, &QLineEdit::textChanged, this, &CGlobalTrackSettings::UpdateInfoEditMode);
  connect(leBld, &QLineEdit::textChanged, this, &CGlobalTrackSettings::UpdateInfoEditMode);
  connect(leTrackNum, &QLineEdit::textChanged, this, &CGlobalTrackSettings::UpdateInfoEditMode);
  connect(leImpossibleLaps, &QLineEdit::textChanged, this, &CGlobalTrackSettings::UpdateInfoEditMode);
  connect(leHardLaps, &QLineEdit::textChanged, this, &CGlobalTrackSettings::UpdateInfoEditMode);
  connect(leTrickyLaps, &QLineEdit::textChanged, this, &CGlobalTrackSettings::UpdateInfoEditMode);
  connect(leMediumLaps, &QLineEdit::textChanged, this, &CGlobalTrackSettings::UpdateInfoEditMode);
  connect(leEasyLaps, &QLineEdit::textChanged, this, &CGlobalTrackSettings::UpdateInfoEditMode);
  connect(leGirlieLaps, &QLineEdit::textChanged, this, &CGlobalTrackSettings::UpdateInfoEditMode);
  connect(leMapSize, &QLineEdit::textChanged, this, &CGlobalTrackSettings::UpdateInfoEditMode);
  connect(leMapFidelity, &QLineEdit::textChanged, this, &CGlobalTrackSettings::UpdateInfoEditMode);
  connect(lePreviewSize, &QLineEdit::textChanged, this, &CGlobalTrackSettings::UpdateInfoEditMode);
}

//-------------------------------------------------------------------------------------------------

CGlobalTrackSettings::~CGlobalTrackSettings()
{
  if (p) {
    delete p;
    p = NULL;
  }
}

//-------------------------------------------------------------------------------------------------

void CGlobalTrackSettings::OnUpdateWindow()
{
  if (!g_pMainWindow->GetCurrentTrack())
    return;

  UpdateInfoSelection();
  UpdateInfoEditMode();
}

//-------------------------------------------------------------------------------------------------

void CGlobalTrackSettings::OnApplyInfoClicked()
{
  if (!g_pMainWindow->GetCurrentTrack())
    return;

  g_pMainWindow->GetCurrentTrack()->m_raceInfo.iTrackNumber = leTrackNum->text().toInt();
  g_pMainWindow->GetCurrentTrack()->m_raceInfo.iImpossibleLaps = leImpossibleLaps->text().toInt();
  g_pMainWindow->GetCurrentTrack()->m_raceInfo.iHardLaps = leHardLaps->text().toInt();
  g_pMainWindow->GetCurrentTrack()->m_raceInfo.iTrickyLaps = leTrickyLaps->text().toInt();
  g_pMainWindow->GetCurrentTrack()->m_raceInfo.iMediumLaps = leMediumLaps->text().toInt();
  g_pMainWindow->GetCurrentTrack()->m_raceInfo.iEasyLaps = leEasyLaps->text().toInt();
  g_pMainWindow->GetCurrentTrack()->m_raceInfo.iGirlieLaps = leGirlieLaps->text().toInt();
  g_pMainWindow->GetCurrentTrack()->m_raceInfo.dTrackMapSize = leMapSize->text().toDouble();
  g_pMainWindow->GetCurrentTrack()->m_raceInfo.iTrackMapFidelity = leMapFidelity->text().toInt();
  g_pMainWindow->GetCurrentTrack()->m_raceInfo.dPreviewSize = lePreviewSize->text().toDouble();
  g_pMainWindow->GetCurrentTrack()->m_sTextureFile = leTex->text().toLatin1().constData();
  g_pMainWindow->GetCurrentTrack()->m_sBuildingFile = leBld->text().toLatin1().constData();
  g_pMainWindow->GetCurrentTrack()->m_header.iFloorDepth = leFloorDepth->text().toInt();

  g_pMainWindow->SaveHistory("Applied global track settings");
  CTrack *pTrack = g_pMainWindow->GetCurrentTrack();
  pTrack->m_assets.LoadFromDocument(
      pTrack->m_sTrackFileFolder, pTrack->m_sTextureFile,
      pTrack->m_sBuildingFile,
      g_pMainWindow->GetFatdataFolder().toStdString());
  g_pMainWindow->UpdateWindow(true);
}

//-------------------------------------------------------------------------------------------------

void CGlobalTrackSettings::OnCancelInfoClicked()
{
  RevertInfo();
}

//-------------------------------------------------------------------------------------------------

void CGlobalTrackSettings::UpdateInfoEditMode()
{
  bool bEditMode = false;
  if (leFloorDepth->text().compare(p->sFloorDepth) != 0
      || leTex->text().compare(p->sTex) != 0
      || leBld->text().compare(p->sBld) != 0
      || leTrackNum->text().compare(p->sTrackNumber) != 0
      || leImpossibleLaps->text().compare(p->sImpossibleLaps) != 0
      || leHardLaps->text().compare(p->sHardLaps) != 0
      || leTrickyLaps->text().compare(p->sTrickyLaps) != 0
      || leMediumLaps->text().compare(p->sMediumLaps) != 0
      || leEasyLaps->text().compare(p->sEasyLaps) != 0
      || leGirlieLaps->text().compare(p->sGirlieLaps) != 0
      || leMapFidelity->text().compare(p->sTrackMapFidelity) != 0
      || leMapSize->text().compare(p->sTrackMapSize) != 0
      || lePreviewSize->text().compare(p->sPreviewSize) != 0)
  bEditMode = true;

  pbApplyInfo->setEnabled(bEditMode);
  pbRevertInfo->setEnabled(bEditMode);
}

//-------------------------------------------------------------------------------------------------

void CGlobalTrackSettings::UpdateInfoSelection()
{
  p->sFloorDepth = QString::number(g_pMainWindow->GetCurrentTrack()->m_header.iFloorDepth);
  p->sTex = g_pMainWindow->GetCurrentTrack()->m_sTextureFile.c_str();
  p->sBld = g_pMainWindow->GetCurrentTrack()->m_sBuildingFile.c_str();
  p->sTrackNumber = QString::number(g_pMainWindow->GetCurrentTrack()->m_raceInfo.iTrackNumber);
  p->sImpossibleLaps = QString::number(g_pMainWindow->GetCurrentTrack()->m_raceInfo.iImpossibleLaps);
  p->sHardLaps = QString::number(g_pMainWindow->GetCurrentTrack()->m_raceInfo.iHardLaps);
  p->sTrickyLaps = QString::number(g_pMainWindow->GetCurrentTrack()->m_raceInfo.iTrickyLaps);
  p->sMediumLaps = QString::number(g_pMainWindow->GetCurrentTrack()->m_raceInfo.iMediumLaps);
  p->sEasyLaps = QString::number(g_pMainWindow->GetCurrentTrack()->m_raceInfo.iEasyLaps);
  p->sGirlieLaps = QString::number(g_pMainWindow->GetCurrentTrack()->m_raceInfo.iGirlieLaps);
  p->sTrackMapSize = QString::number(g_pMainWindow->GetCurrentTrack()->m_raceInfo.dTrackMapSize, 'f', 2);
  p->sTrackMapFidelity = QString::number(g_pMainWindow->GetCurrentTrack()->m_raceInfo.iTrackMapFidelity);
  p->sPreviewSize = QString::number(g_pMainWindow->GetCurrentTrack()->m_raceInfo.dPreviewSize, 'f', 2);

  RevertInfo();
}

//-------------------------------------------------------------------------------------------------

void CGlobalTrackSettings::RevertInfo()
{
  UpdateAssetStatus();
  BLOCK_SIG_AND_DO(leFloorDepth, setText(p->sFloorDepth));
  BLOCK_SIG_AND_DO(leTex, setText(p->sTex));
  BLOCK_SIG_AND_DO(leBld, setText(p->sBld));
  BLOCK_SIG_AND_DO(leTrackNum, setText(p->sTrackNumber));
  BLOCK_SIG_AND_DO(leImpossibleLaps, setText(p->sImpossibleLaps));
  BLOCK_SIG_AND_DO(leHardLaps, setText(p->sHardLaps));
  BLOCK_SIG_AND_DO(leTrickyLaps, setText(p->sTrickyLaps));
  BLOCK_SIG_AND_DO(leMediumLaps, setText(p->sMediumLaps));
  BLOCK_SIG_AND_DO(leEasyLaps, setText(p->sEasyLaps));
  BLOCK_SIG_AND_DO(leGirlieLaps, setText(p->sGirlieLaps));
  BLOCK_SIG_AND_DO(leMapSize, setText(p->sTrackMapSize));
  BLOCK_SIG_AND_DO(leMapFidelity, setText(p->sTrackMapFidelity));
  BLOCK_SIG_AND_DO(lePreviewSize, setText(p->sPreviewSize));

  pbApplyInfo->setEnabled(false);
  pbRevertInfo->setEnabled(false);
}

//-------------------------------------------------------------------------------------------------

void CGlobalTrackSettings::UpdateAssetStatus()
{
  const QString sDocumentAssetRoot =
      QString::fromStdString(g_pMainWindow->GetCurrentTrack()->m_sTrackFileFolder);
  const QString sFatdataAssetRoot = g_pMainWindow->GetFatdataFolder();
  const QFileInfo DocumentPalette(
      QDir(sDocumentAssetRoot).filePath("PALETTE.PAL"));
  const QFileInfo FatdataPalette(
      sFatdataAssetRoot.isEmpty()
          ? QString()
          : QDir(sFatdataAssetRoot).filePath("PALETTE.PAL"));

  lblNoTex->setVisible(false);
  lblPalNotFound->setVisible(
      !DocumentPalette.exists() && !FatdataPalette.exists());
}

//-------------------------------------------------------------------------------------------------
