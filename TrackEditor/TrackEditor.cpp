#include "TrackEditor.h"
#include "MainWindow.h"
#include "qapplication.h"
#include "qdesktopwidget.h"
#include <qstring.h>
#include "qdir.h"
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
  float fScale = app.desktop()->logicalDpiX() / 96.0 * 100.0;
  CMainWindow *pMainWin = new CMainWindow(sAppPath, fScale);
  
  int iRetCode = app.exec();

  delete pMainWin;
  pMainWin = NULL;

  return iRetCode;
}

//-------------------------------------------------------------------------------------------------
