#include "GlobalTrackSettings.h"
#include "Track.h"
#include "MainWindow.h"
#include "QtHelpers.h"
#include "qdiriterator.h"
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

  connect(g_pMainWindow, &CMainWindow::UpdateWindowSig, this, &CGlobalTrackSettings::OnUpdateWindow);

  connect(pbApplyInfo, &QPushButton::clicked, this, &CGlobalTrackSettings::OnApplyInfoClicked);
  connect(pbRevertInfo, &QPushButton::clicked, this, &CGlobalTrackSettings::OnCancelInfoClicked);

  connect(leFloorDepth, &QLineEdit::textChanged, this, &CGlobalTrackSettings::UpdateInfoEditMode);
  connect(cbTex, SIGNAL(currentIndexChanged(int)), this, SLOT(UpdateInfoEditMode()));
  connect(cbBld, SIGNAL(currentIndexChanged(int)), this, SLOT(UpdateInfoEditMode()));
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
  g_pMainWindow->GetCurrentTrack()->m_sTextureFile = cbTex->currentText().toLatin1().constData();
  g_pMainWindow->GetCurrentTrack()->m_sBuildingFile = cbBld->currentText().toLatin1().constData();
  g_pMainWindow->GetCurrentTrack()->m_header.iFloorDepth = leFloorDepth->text().toInt();

  g_pMainWindow->SaveHistory("Applied global track settings");
  g_pMainWindow->GetCurrentTrack()->LoadTextures();
  g_pMainWindow->UpdateWindow();
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
  bool bMixedData = false;
  QtHelpers::UpdateLEEditMode(bEditMode, bMixedData, leFloorDepth, p->sFloorDepth);
  if (cbTex->currentText().compare(p->sTex) != 0 || cbBld->currentText().compare(p->sBld) != 0)
    bEditMode = true;
  QtHelpers::UpdateLEEditMode(bEditMode, bMixedData, leTrackNum, p->sTrackNumber);
  QtHelpers::UpdateLEEditMode(bEditMode, bMixedData, leImpossibleLaps, p->sImpossibleLaps);
  QtHelpers::UpdateLEEditMode(bEditMode, bMixedData, leHardLaps, p->sHardLaps);
  QtHelpers::UpdateLEEditMode(bEditMode, bMixedData, leTrickyLaps, p->sTrickyLaps);
  QtHelpers::UpdateLEEditMode(bEditMode, bMixedData, leMediumLaps, p->sMediumLaps);
  QtHelpers::UpdateLEEditMode(bEditMode, bMixedData, leEasyLaps, p->sEasyLaps);
  QtHelpers::UpdateLEEditMode(bEditMode, bMixedData, leGirlieLaps, p->sGirlieLaps);
  QtHelpers::UpdateLEEditMode(bEditMode, bMixedData, leMapFidelity, p->sTrackMapFidelity);
  QtHelpers::UpdateLEEditMode(bEditMode, bMixedData, leMapSize, p->sTrackMapSize);
  QtHelpers::UpdateLEEditMode(bEditMode, bMixedData, lePreviewSize, p->sPreviewSize);

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
  UpdateTextures();
  QtHelpers::UpdateLEWithSelectionValue(leFloorDepth, p->sFloorDepth);
  BLOCK_SIG_AND_DO(cbTex, setCurrentIndex(cbTex->findText(p->sTex)));
  BLOCK_SIG_AND_DO(cbBld, setCurrentIndex(cbBld->findText(p->sBld)));
  QtHelpers::UpdateLEWithSelectionValue(leTrackNum, p->sTrackNumber);
  QtHelpers::UpdateLEWithSelectionValue(leImpossibleLaps, p->sImpossibleLaps);
  QtHelpers::UpdateLEWithSelectionValue(leHardLaps, p->sHardLaps);
  QtHelpers::UpdateLEWithSelectionValue(leTrickyLaps, p->sTrickyLaps);
  QtHelpers::UpdateLEWithSelectionValue(leMediumLaps, p->sMediumLaps);
  QtHelpers::UpdateLEWithSelectionValue(leEasyLaps, p->sEasyLaps);
  QtHelpers::UpdateLEWithSelectionValue(leGirlieLaps, p->sGirlieLaps);
  QtHelpers::UpdateLEWithSelectionValue(leMapSize, p->sTrackMapSize);
  QtHelpers::UpdateLEWithSelectionValue(leMapFidelity, p->sTrackMapFidelity);
  QtHelpers::UpdateLEWithSelectionValue(lePreviewSize, p->sPreviewSize);

  pbApplyInfo->setEnabled(false);
  pbRevertInfo->setEnabled(false);
}

//-------------------------------------------------------------------------------------------------

void CGlobalTrackSettings::UpdateTextures()
{
  cbBld->clear();
  cbTex->clear();
  QDirIterator it(g_pMainWindow->GetCurrentTrack()->m_sTrackFileFolder.c_str(), QStringList() << "*.DRH", QDir::Files);
  while (it.hasNext()) {
    QString sNext = it.next();
    int iPos = sNext.lastIndexOf('\\');
    if (iPos < 0)
      iPos = sNext.lastIndexOf('/');
    sNext = sNext.right(sNext.size() - iPos - 1);
    cbBld->addItem(sNext, sNext);
    cbTex->addItem(sNext, sNext);
  }
  QFileInfo palFile(QString(g_pMainWindow->GetCurrentTrack()->m_sTrackFileFolder.c_str())
                    + QDir::separator() + "PALETTE.PAL");

  lblNoTex->setVisible(cbTex->count() == 0);
  lblPalNotFound->setVisible(!palFile.exists());
}

//-------------------------------------------------------------------------------------------------