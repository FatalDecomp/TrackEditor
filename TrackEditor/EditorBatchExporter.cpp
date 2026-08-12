#include "EditorBatchExporter.h"

#include "EditorCarModel.h"
#include "EditorGltfExporter.h"
#include "EditorObjExporter.h"
#include "EditorRenderService.h"
#include "TrackPreview.h"
#include "Palette.h"
#include "Texture.h"

#include <QApplication>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QProgressDialog>
#include <QTemporaryDir>
#include <QTimer>

#include <vector>

namespace
{
constexpr uint32_t kCarTextureSet = 1u;

QString FindFile(const QString &sFolder, const QString &sName)
{
  const QFileInfoList Files = QDir(sFolder).entryInfoList(
      QDir::Files | QDir::Readable, QDir::Name | QDir::IgnoreCase);
  for (const QFileInfo &File : Files) {
    if (File.fileName().compare(sName, Qt::CaseInsensitive) == 0)
      return File.absoluteFilePath();
  }
  return QString();
}

QFileInfoList TrackFiles(const QString &sFolder)
{
  QFileInfoList Tracks;
  const QFileInfoList Files = QDir(sFolder).entryInfoList(
      QDir::Files | QDir::Readable, QDir::Name | QDir::IgnoreCase);
  for (const QFileInfo &File : Files) {
    if (File.suffix().compare("trk", Qt::CaseInsensitive) == 0)
      Tracks.push_back(File);
  }
  return Tracks;
}

std::vector<tEdExportPaletteEntry> ExportPalette(const CPalette &Palette)
{
  std::vector<tEdExportPaletteEntry> Result(PALETTE_SIZE);
  for (uint32_t i = 0; i < PALETTE_SIZE; ++i) {
    Result[i].byRed = Palette.m_paletteAy[i].r;
    Result[i].byGreen = Palette.m_paletteAy[i].g;
    Result[i].byBlue = Palette.m_paletteAy[i].b;
  }
  return Result;
}

bool WaitForTrack(CTrackPreview &Preview, const QString &sTrackFile,
                  QString &sError)
{
  QEventLoop Loop;
  bool bCompleted = false;
  const QMetaObject::Connection Completion = QObject::connect(
      &Preview, &CTrackPreview::FrameStateChanged, &Loop, [&]() {
        bCompleted = true;
        Loop.quit();
      });

  if (!Preview.LoadTrack(sTrackFile)) {
    QObject::disconnect(Completion);
    sError = "could not parse the track file";
    return false;
  }

  if (!bCompleted) {
    QTimer::singleShot(60000, &Loop, &QEventLoop::quit);
    Loop.exec();
  }
  QObject::disconnect(Completion);

  if (!bCompleted) {
    sError = "the ROLLER render worker timed out while loading the track";
    return false;
  }
  if (!Preview.CanExport()) {
    sError = Preview.GetLastRenderError();
    if (sError.isEmpty())
      sError = "ROLLER could not prepare canonical track geometry";
    return false;
  }
  return true;
}

bool ExportCar(const QString &sFatdataFolder, const QString &sCarsFolder,
               eExportType exportType, uint32_t uiDesign,
               CPalette &Palette,
               const std::vector<tEdExportPaletteEntry> &ExportColours,
               QString &sError)
{
  const char *szName = CEditorCarModel::Name(uiDesign);
  const char *szTexture = CEditorCarModel::TextureFileName(uiDesign);
  if (!szName || !szTexture) {
    sError = "unknown car design";
    return false;
  }

  const QString sName = QString::fromLatin1(szName);
  const QString sTextureFile = FindFile(
      sFatdataFolder, QString::fromLatin1(szTexture));
  if (sTextureFile.isEmpty()) {
    sError = QString("missing %1").arg(QString::fromLatin1(szTexture));
    return false;
  }

  CTexture Texture;
  if (!Texture.LoadTexture(QFile::encodeName(sTextureFile).constData(),
                           &Palette)) {
    sError = QString("could not read %1").arg(QFileInfo(sTextureFile).fileName());
    return false;
  }

  tEdCarGeometry OwnedGeometry;
  std::string sBuildError;
  if (!CEditorCarModel::Build(uiDesign,
                              static_cast<uint32_t>(Texture.GetNumTiles()),
                              OwnedGeometry, sBuildError)) {
    sError = QString::fromStdString(sBuildError);
    return false;
  }
  const tEdExportGeometry Geometry = OwnedGeometry.View();

  if (exportType == eExportType::EXPORT_OBJ) {
    const QString sTextureOutput = QDir(sCarsFolder).filePath(sName + ".png");
    if (!Texture.ExportToPngFile(
            QFile::encodeName(sTextureOutput).constData())) {
      sError = "could not write the car texture atlas";
      return false;
    }

    tEdObjExportOptions Options;
    Options.sSingleObjectName = sName.toStdString();
    Options.bExportScenery = true;
    Options.bSeparateSections = false;
    Options.bSeparateBackFaces = true;
    Options.sBaseName = sName.toStdString();
    Options.sMtlFileName = (sName + ".mtl").toStdString();
    std::string sWriteError;
    if (!CEditorObjExporter::ExportToFiles(
            Geometry, Options, ExportColours.data(),
            static_cast<uint32_t>(ExportColours.size()),
            QFile::encodeName(QDir(sCarsFolder).filePath(sName + ".obj"))
                .constData(),
            QFile::encodeName(QDir(sCarsFolder).filePath(sName + ".mtl"))
                .constData(),
            sWriteError)) {
      sError = QString::fromStdString(sWriteError);
      return false;
    }
    return true;
  }

  QTemporaryDir Temporary;
  if (!Temporary.isValid()) {
    sError = "could not stage the car texture atlas";
    return false;
  }
  const QString sStagedTexture = QDir(Temporary.path()).filePath(sName + ".png");
  if (!Texture.ExportToPngFile(QFile::encodeName(sStagedTexture).constData())) {
    sError = "could not stage the car texture atlas";
    return false;
  }
  QFile Png(sStagedTexture);
  if (!Png.open(QIODevice::ReadOnly)) {
    sError = "could not read the staged car texture atlas";
    return false;
  }
  const QByteArray PngBytes = Png.readAll();

  tEdGltfExportOptions Options;
  Options.sSingleObjectName = sName.toStdString();
  Options.bExportScenery = true;
  Options.bSeparateSections = false;
  Options.bSeparateBackFaces = true;
  Options.bDoubleSidedMaterials = true;
  Options.bBinary = true;
  Options.sBaseName = sName.toStdString();
  tEdGltfTextureSource TextureSource;
  TextureSource.uiTextureSet = kCarTextureSet;
  TextureSource.PngBytes.assign(
      reinterpret_cast<const uint8_t *>(PngBytes.constData()),
      reinterpret_cast<const uint8_t *>(PngBytes.constData())
          + PngBytes.size());
  Options.Textures.push_back(std::move(TextureSource));

  std::string sWriteError;
  if (!CEditorGltfExporter::ExportToFiles(
          Geometry, Options, ExportColours.data(),
          static_cast<uint32_t>(ExportColours.size()),
          QFile::encodeName(QDir(sCarsFolder).filePath(sName + ".glb"))
              .constData(),
          std::string(), sWriteError)) {
    sError = QString::fromStdString(sWriteError);
    return false;
  }
  return true;
}
}

CEditorBatchExporter::CEditorBatchExporter(
    QWidget *pParent, CEditorRenderService *pRenderService)
  : m_pParent(pParent)
  , m_pRenderService(pRenderService)
{
}

bool CEditorBatchExporter::IsFatdataFolder(const QString &sFolder)
{
  return !FindFile(sFolder, "PALETTE.PAL").isEmpty()
      && !TrackFiles(sFolder).isEmpty();
}

tEdBatchExportResult CEditorBatchExporter::Export(
    const QString &sFatdataFolder, const QString &sOutputFolder,
    eExportType exportType)
{
  tEdBatchExportResult Result;
  const QFileInfoList Tracks = TrackFiles(sFatdataFolder);
  const QString sTracksFolder = QDir(sOutputFolder).filePath("Tracks");
  const QString sCarsFolder = QDir(sOutputFolder).filePath("Cars");
  if (!QDir().mkpath(sTracksFolder) || !QDir().mkpath(sCarsFolder)) {
    Result.iFailed = 1;
    Result.Failures << "Could not create the Tracks and Cars output folders.";
    return Result;
  }

  const int iTotal = Tracks.size() + static_cast<int>(CEditorCarModel::Count());
  QProgressDialog Progress("Preparing batch export...", "Cancel", 0, iTotal,
                           m_pParent);
  Progress.setWindowTitle("Export all tracks and cars");
  Progress.setWindowModality(Qt::WindowModal);
  Progress.setMinimumDuration(0);
  int iProgress = 0;

  for (const QFileInfo &Track : Tracks) {
    Progress.setLabelText("Exporting track " + Track.fileName());
    QApplication::processEvents();
    if (Progress.wasCanceled()) {
      Result.bCancelled = true;
      break;
    }

    CTrackPreview Preview(m_pParent, m_pRenderService);
    Preview.resize(640, 480);
    QString sError;
    const bool bLoaded = WaitForTrack(Preview, Track.absoluteFilePath(), sError);
    const bool bExported = bLoaded && Preview.ExportToFolder(
        exportType, sTracksFolder, Track.completeBaseName());
    if (bExported) {
      Result.iSucceeded++;
    } else {
      Result.iFailed++;
      if (sError.isEmpty())
        sError = "the model or its texture atlases could not be written";
      Result.Failures << QString("%1: %2").arg(Track.fileName(), sError);
    }
    Progress.setValue(++iProgress);
  }

  CPalette Palette;
  const QString sPaletteFile = FindFile(sFatdataFolder, "PALETTE.PAL");
  const bool bPaletteLoaded = !Result.bCancelled
      && !sPaletteFile.isEmpty()
      && Palette.LoadPalette(QFile::encodeName(sPaletteFile).constData());
  const std::vector<tEdExportPaletteEntry> Colours = bPaletteLoaded
      ? ExportPalette(Palette) : std::vector<tEdExportPaletteEntry>();

  for (uint32_t uiDesign = 0;
       !Result.bCancelled && uiDesign < CEditorCarModel::Count();
       ++uiDesign) {
    const QString sName = QString::fromLatin1(CEditorCarModel::Name(uiDesign));
    Progress.setLabelText("Exporting car " + sName);
    QApplication::processEvents();
    if (Progress.wasCanceled()) {
      Result.bCancelled = true;
      break;
    }

    QString sError;
    const bool bExported = bPaletteLoaded
        && ExportCar(sFatdataFolder, sCarsFolder, exportType, uiDesign,
                     Palette, Colours, sError);
    if (bExported) {
      Result.iSucceeded++;
    } else {
      Result.iFailed++;
      if (!bPaletteLoaded)
        sError = "could not read PALETTE.PAL";
      Result.Failures << QString("%1: %2").arg(sName, sError);
    }
    Progress.setValue(++iProgress);
  }

  Progress.setValue(iTotal);
  return Result;
}
