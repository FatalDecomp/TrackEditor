#ifndef _TRACKEDITOR_TRACKPREVIEW_H
#define _TRACKEDITOR_TRACKPREVIEW_H
//-------------------------------------------------------------------------------------------------
#include <QGL>
#include "Types.h"
//-------------------------------------------------------------------------------------------------
#define DEFAULT_HISTORY_MAX_SIZE 256 //approx 200KB per saved track
struct tTrackHistory
{
  std::string sDescription;
  std::vector<uint8> byteAy;
};
typedef std::vector<tTrackHistory> CHistoryAy;
//-------------------------------------------------------------------------------------------------
enum eExportType
{
  EXPORT_FBX = 0,
  EXPORT_OBJ
};
//-------------------------------------------------------------------------------------------------
class CTrackPreviewPrivate;
class CTrack;
//-------------------------------------------------------------------------------------------------
class CTrackPreview : public QGLWidget
{
  Q_OBJECT

public:
  CTrackPreview(QWidget *pParent, const QString &sTrackFile = "");
  ~CTrackPreview();

  void UpdateCameraPos();
  bool LoadTrack(const QString &sFilename);
  void DeleteEnvirFloor();
  void UpdateTrack(bool bUpdatingStunt = false);
  void ShowModels(uint32 uiShowModels);
  void UpdateCar(eWhipModel carModel, eShapeSection aiLine, bool bMillionPlus);
  void AttachLast(bool bAttachLast);
  void OpenReferenceModel();
  CTrack *GetTrack();
  bool SaveChangesAndContinue();
  bool SaveTrack();
  bool SaveTrackAs();
  bool Export(eExportType exportType);
  QString GetTitle(bool bFullPath);
  const QString &GetFilename() { return m_sTrackFile; };
  void UpdateGeometrySelection();
  void SaveHistory(const QString &sDescription);
  void Undo();
  void Redo();

  bool m_bUnsavedChanges;
  int m_iSelFrom;
  int m_iSelTo;
  bool m_bToChecked;

protected:
  void initializeGL();
  void paintGL();
  void mousePressEvent(QMouseEvent *pEvent) override;
  void mouseReleaseEvent(QMouseEvent *pEvent) override;
  void mouseMoveEvent(QMouseEvent *pEvent) override;
  void keyPressEvent(QKeyEvent *pEvent) override;
  void keyReleaseEvent(QKeyEvent *pEvent) override;

private:
  void LoadHistory(const tTrackHistory *pHistory);
  bool SaveTrack_Internal(const QString &sFilename);

  CTrackPreviewPrivate *p;
  uint32 m_uiShowModels;
  eWhipModel m_carModel;
  eShapeSection m_carAILine;
  bool m_bMillionPlus;
  bool m_bAttachLast;
  int m_iScale;
  bool m_bAlreadySaved;
  QString m_sTrackFile;
  QString m_sLastCarTex;
  int m_iHistoryIndex;
  QString m_sReferenceModelFile;
};

//-------------------------------------------------------------------------------------------------
#endif
