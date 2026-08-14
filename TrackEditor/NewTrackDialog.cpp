#include "TrackEditor.h"
#include "NewTrackDialog.h"
#include "qdir.h"
#include "qfiledialog.h"
#include "qfileinfo.h"
#include "qmessagebox.h"
#include "MainWindow.h"
#include "editor_track_loader.h"
//-------------------------------------------------------------------------------------------------
#if defined(_DEBUG) && defined(IS_WINDOWS)
  #define new new(_CLIENT_BLOCK, __FILE__, __LINE__)
#endif
//-------------------------------------------------------------------------------------------------
namespace
{
bool HasDrhFile(const QString &sRoot)
{
  if (sRoot.isEmpty())
    return false;

  return !QDir(sRoot).entryList(
      QStringList() << "*.DRH", QDir::Files).isEmpty();
}

bool HasFileInRoot(const QString &sRoot, const QString &sFilename)
{
  if (sRoot.isEmpty() || sFilename.isEmpty())
    return false;

  return QFileInfo(QDir(sRoot).filePath(sFilename)).isFile();
}
}
//-------------------------------------------------------------------------------------------------

CNewTrackDialog::CNewTrackDialog(QWidget *pParent, int iNewTrackNum)
  : QDialog(pParent)
{
  setupUi(this);

  leDir->setText(g_pMainWindow->m_sLastTrackFilesFolder);
  leName->setText("NEWTRACK" + QString::number(iNewTrackNum));
  leTex->setText("TRACK1.DRH");
  leBld->setText("BUILDING.DRH");
  const int iMaxAssetNameLength =
      static_cast<int>(ED_TRACK_ASSET_NAME_CAPACITY - 1u);
  leTex->setMaxLength(iMaxAssetNameLength);
  leBld->setMaxLength(iMaxAssetNameLength);

  connect(pbBrowse, &QPushButton::clicked, this, &CNewTrackDialog::BrowseClicked);
  connect(pbOk, &QPushButton::clicked, this, &CNewTrackDialog::OkClicked);
  connect(pbCancel, &QPushButton::clicked, this, &CNewTrackDialog::reject);

  UpdateDialog();
}

//-------------------------------------------------------------------------------------------------

CNewTrackDialog::~CNewTrackDialog()
{
}

//-------------------------------------------------------------------------------------------------

QString CNewTrackDialog::GetFilename()
{
  return leDir->text() + QDir::separator() + leName->text() + ".TRK";
}

//-------------------------------------------------------------------------------------------------

QString CNewTrackDialog::GetTex()
{
  return leTex->text();
}

//-------------------------------------------------------------------------------------------------

QString CNewTrackDialog::GetBld()
{
  return leBld->text();
}

//-------------------------------------------------------------------------------------------------

void CNewTrackDialog::BrowseClicked()
{
  QString sDir = QDir::toNativeSeparators(QFileDialog::getExistingDirectory(
    this, "Choose Track Directory", leDir->text()));
  leDir->setText(sDir);

  UpdateDialog();
}

//-------------------------------------------------------------------------------------------------

void CNewTrackDialog::OkClicked()
{
  if (leName->text().isEmpty())
    QMessageBox::critical(this, "Error", "Name cannot be empty");
  else
    accept();
}

//-------------------------------------------------------------------------------------------------

void CNewTrackDialog::UpdateDialog()
{
  const QString sTrackFolder = leDir->text();
  const QString sFatdataFolder = g_pMainWindow->GetFatdataFolder();
  const bool bHasDrh = HasDrhFile(sTrackFolder)
      || HasDrhFile(sFatdataFolder);
  const bool bHasPalette = HasFileInRoot(sTrackFolder, "PALETTE.PAL")
      || HasFileInRoot(sFatdataFolder, "PALETTE.PAL");

  lblNoTex->setText(
      "No *.DRH files found in the track directory or FATDATA!");
  lblNoTex->setVisible(!bHasDrh);
  lblPalNotFound->setText(
      "PALETTE.PAL not found in the track directory or FATDATA!");
  lblPalNotFound->setVisible(!bHasPalette);
}

//-------------------------------------------------------------------------------------------------
