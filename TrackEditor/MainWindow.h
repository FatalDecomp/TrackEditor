#ifndef _TRACKEDITOR_MAINWINDOW_H
#define _TRACKEDITOR_MAINWINDOW_H
//-------------------------------------------------------------------------------------------------
#include "ui_MainWindow.h"
#include "QtUserKeyMapper.h"
//-------------------------------------------------------------------------------------------------
class CMainWindowPrivate;
class CTrack;
class CTrackPreview;
//-------------------------------------------------------------------------------------------------
struct tPreferences
{
  tPreferences();
  int iHistoryMaxSize;
  bool bPasteNewChunks;
  bool bCopyRelativeYaw;
  bool bCopyRelativePitch;
  bool bCopyRelativeRoll;
  bool bPasteDirection;
  bool bPasteGeometry;
  bool bPasteTextures;
  bool bPasteSurfaceData;
  bool bPasteAIBehavior;
  bool bPasteDrawOrder;
  bool bPasteSigns;
  bool bPasteAudio;
};
//-------------------------------------------------------------------------------------------------

class CMainWindow : public QMainWindow, private Ui::MainWindow
{
  Q_OBJECT

public:
  CMainWindow(const QString &sAppPath, float fDesktopScale);
  ~CMainWindow();

  const QString &GetAppPath() { return m_sAppPath; };
  void LogMessage(const QString &sMsg);
  void SaveHistory(const QString &sDescription);
  void UpdateWindow(bool bUpdatingTextures = false);
  void InsertUIUpdate(int iInsertVal);
  int GetSelFrom();
  int GetSelTo();
  float GetDesktopScale() { return m_fDesktopScale; };
  CTrack *GetCurrentTrack();
  CTrackPreview *GetCurrentPreview();

  QString m_sLastTrackFilesFolder;
  CQtUserKeyMapper m_keyMapper;
  tPreferences m_preferences;

protected:
  void closeEvent(QCloseEvent *pEvent);

protected slots:
  void OnLogMsg(QString sMsg);
  void OnNewTrack();
  void OnLoadTrack();
  void OnSaveTrack();
  void OnSaveTrackAs();
  void OnExportFBX();
  void OnExportOBJ();
  void OnUndo();
  void OnRedo();
  void OnCut();
  void OnCopy();
  void OnPaste();
  void OnSelectAll();
  void OnMirror();
  void OnDeselect();
  void OnBacks();
  void OnPreferences();
  void OnDebug();
  void OnAbout();
  void OnTabCloseRequested(int iIndex);
  void OnTabChanged(int iIndex);
  void OnSelChunksFromChanged(int iValue);
  void OnSelChunksToChanged(int iValue);
  void OnToChecked(bool bChecked);
  void OnDeleteChunkClicked();
  void OnAddChunkClicked();
  void OnAttachLast(bool bChecked);
  void OnOpenReferenceModel();
  void OnUpdatePreview();
  void OnSaveHistoryTimer();
  void OnStuntTimer();
  void OnZeroTimer();
  void OnReferenceModelChanged();
  void OnRefModelPos(double dYaw, double dPitch, double dRoll,
                     int iX, int iY, int iZ,
                     double dScale);

signals:
  void LogMsgSig(QString sMsg);
  void UpdateWindowSig();
  void UpdateGeometrySelectionSig(int iFrom, int iTo);

private:
  void LoadSettings();
  void SaveSettings();
  bool SaveChangesAndContinue();
  void UpdateGeometrySelection();
  int MirrorSurfaceType(int iSurfaceType);

  CMainWindowPrivate *p;
  QString m_sAppPath;
  QString m_sSettingsFile;
  float m_fDesktopScale;
  int m_iNewTrackNum;
  QString m_sHistoryDescription;
  QTimer *m_pSaveHistoryTimer;
  QTimer *m_pStuntTimer;
  QTimer *m_pZeroTimer;
};

//-------------------------------------------------------------------------------------------------

extern CMainWindow *g_pMainWindow;

//-------------------------------------------------------------------------------------------------
#endif