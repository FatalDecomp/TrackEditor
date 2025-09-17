#ifndef _TRACKEDITOR_DEBUGCHUNKDATA_H
#define _TRACKEDITOR_DEBUGCHUNKDATA_H
//-------------------------------------------------------------------------------------------------
#include "ui_DebugChunkData.h"
//-------------------------------------------------------------------------------------------------
class CTrack;
//-------------------------------------------------------------------------------------------------
class CDebugChunkData : public QWidget, private Ui::DebugChunkData
{
  Q_OBJECT

public:
  CDebugChunkData(QWidget *pParent);
  ~CDebugChunkData();

protected slots:
  void UpdateGeometrySelection(int iFrom, int iTo);

private:
  void UpdateTextures(QLineEdit *pLineEdit, QLabel *pTex1, QLabel *pTex2);
  void UpdateSignTexture(QLineEdit *pLineEdit, QLabel *pTex1);
};

//-------------------------------------------------------------------------------------------------
#endif