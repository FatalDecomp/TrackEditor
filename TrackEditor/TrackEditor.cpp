#include "TrackEditor.h"
#include "EditorRenderService.h"
#include "EditorBatchExporter.h"
#include "MainWindow.h"
#include "qapplication.h"
#include "qscreen.h"
#include <qstring.h>
#include "qdir.h"
#include "qmessagebox.h"
#include <QtCore>
//-------------------------------------------------------------------------------------------------
#if defined(_DEBUG) && defined(IS_WINDOWS)
  #define new new(_CLIENT_BLOCK, __FILE__, __LINE__)
#endif
//-------------------------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
#if defined(_DEBUG) && defined(IS_WINDOWS)
  _set_error_mode(_OUT_TO_MSGBOX);
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

  QApplication app(argc, argv);
  if (app.arguments().contains("--cmake-smoke-test"))
    return 0;

  const QString sAppPath = QCoreApplication::applicationDirPath();
  tRollerEdBootstrapInfo BootstrapInfo = {};
  BootstrapInfo.uiStructSize = sizeof(BootstrapInfo);
  BootstrapInfo.uiVersion = ROLLER_ED_BOOTSTRAP_INFO_VERSION;
  const eRollerEdResult eBootstrapResult = RollerEd_Bootstrap(&BootstrapInfo);
  if (eBootstrapResult != ROLLER_ED_RESULT_OK) {
    QMessageBox::critical(nullptr, "Track Editor",
                          QString("roller-core bootstrap failed (%1)")
                              .arg(eBootstrapResult));
    return 1;
  }

  CEditorRenderService RenderService(sAppPath);
  RenderService.Start();
  // Qt 6 removed QDesktopWidget; the primary screen carries the same logical
  // DPI QApplication::desktop() reported. Qt 6 also always enables high-DPI
  // scaling, so this is 96 on a scaled display and the scale comes out at 100:
  // the thumbnail sizes below stay in logical pixels and Qt applies the device
  // pixel ratio itself, which is the same apparent size Qt 5 produced.
  float fScale = QGuiApplication::primaryScreen()->logicalDotsPerInchX() / 96.0 * 100.0;
  CMainWindow *pMainWin = new CMainWindow(sAppPath, fScale, &RenderService);

  int iRetCode = 0;
  const int iBatchArg = app.arguments().indexOf("--batch-export-test");
  if (iBatchArg >= 0) {
    // Non-interactive acceptance hook. The menu remains the user-facing path;
    // this lets real retail assets exercise that exact coordinator without
    // automating native folder dialogs in CI or during release packaging.
    const QStringList Arguments = app.arguments();
    if (iBatchArg + 3 >= Arguments.size()
        || (Arguments[iBatchArg + 1].compare("obj", Qt::CaseInsensitive) != 0
            && Arguments[iBatchArg + 1].compare(
                   "gltf", Qt::CaseInsensitive) != 0)) {
      qWarning().noquote()
          << "usage: --batch-export-test <obj|gltf> <FATDATA> <output>";
      iRetCode = 2;
    } else {
      const eExportType exportType =
          Arguments[iBatchArg + 1].compare("obj", Qt::CaseInsensitive) == 0
          ? eExportType::EXPORT_OBJ : eExportType::EXPORT_GLTF;
      CEditorBatchExporter Exporter(pMainWin, &RenderService);
      const tEdBatchExportResult Result = Exporter.Export(
          Arguments[iBatchArg + 2], Arguments[iBatchArg + 3], exportType);
      qInfo().noquote() << QString("batch export: %1 succeeded, %2 failed")
          .arg(Result.iSucceeded).arg(Result.iFailed);
      for (const QString &Failure : Result.Failures)
        qWarning().noquote() << Failure;
      iRetCode = Result.iFailed == 0 && !Result.bCancelled ? 0 : 1;
    }
  } else {
    iRetCode = app.exec();
  }

  delete pMainWin;
  pMainWin = NULL;

  RenderService.Stop();
  const eRollerEdResult eTeardownResult = RollerEd_Teardown();
  if (iRetCode == 0 && eTeardownResult != ROLLER_ED_RESULT_OK)
    iRetCode = 1;

  return iRetCode;
}

//-------------------------------------------------------------------------------------------------
