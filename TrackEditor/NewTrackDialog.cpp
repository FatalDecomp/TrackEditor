#include "TrackEditor.h"
#include "NewTrackDialog.h"
#include "qdir.h"
#include "qfiledialog.h"
#include "qdiriterator.h"
#include "qfileinfo.h"
#include "qmessagebox.h"
#include "MainWindow.h"
//-------------------------------------------------------------------------------------------------
#if defined(_DEBUG) && defined(IS_WINDOWS)
  #define new new(_CLIENT_BLOCK, __FILE__, __LINE__)
#endif
//-------------------------------------------------------------------------------------------------

CNewTrackDialog::CNewTrackDialog(QWidget *pParent, int iNewTrackNum)
  : QDialog(pParent)
{
  setupUi(this);

  leDir->setText(g_pMainWindow->m_sLastTrackFilesFolder);
  leName->setText("NEWTRACK" + QString::number(iNewTrackNum));

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
  return cbTex->currentText();
}

//-------------------------------------------------------------------------------------------------

QString CNewTrackDialog::GetBld()
{
  return cbBld->currentText();
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
  cbBld->clear();
  cbTex->clear();
  QDirIterator it(leDir->text(), QStringList() << "*.DRH", QDir::Files);
  while (it.hasNext()) {
    QString sNext = it.next();
    int iPos = sNext.lastIndexOf('\\');
    if (iPos < 0)
      iPos = sNext.lastIndexOf('/');
    sNext = sNext.right(sNext.size() - iPos - 1);
    cbBld->addItem(sNext);
    cbTex->addItem(sNext);
    if (sNext.compare("BUILDING.DRH", Qt::CaseInsensitive) == 0)
      cbBld->setCurrentIndex(cbBld->count() - 1);
    else if (sNext.compare("TRACK1.DRH", Qt::CaseInsensitive) == 0)
      cbTex->setCurrentIndex(cbTex->count() - 1);
  }
  QFileInfo palFile(leDir->text() + QDir::separator() + "PALETTE.PAL");

  lblNoTex->setVisible(cbTex->count() == 0);
  lblPalNotFound->setVisible(!palFile.exists());
}

//-------------------------------------------------------------------------------------------------