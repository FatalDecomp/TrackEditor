#ifndef _TRACKEDITOR_EDITGEOMETRYWIDGET_H
#define _TRACKEDITOR_EDITGEOMETRYWIDGET_H
//-------------------------------------------------------------------------------------------------
#include "ui_EditGeometryWidget.h"
//-------------------------------------------------------------------------------------------------
class CTrack;
class CTexture;
class CPalette;
class CEditGeometryWidgetPrivate;
//-------------------------------------------------------------------------------------------------

class CEditGeometryWidget : public QWidget, private Ui::EditGeometryWidget
{
  Q_OBJECT

public:
  CEditGeometryWidget(QWidget *pParent, CTrack *pTrack, CTexture *pTex, CPalette *pPal);
  ~CEditGeometryWidget();

protected slots:
  void UpdateGeometrySelection(int iFrom, int iTo);
  void YawChanged(double dValue);
  void PitchChanged(double dValue);
  void RollChanged(double dValue);
  void LengthChanged(int iValue);
  void LLaneWidthChanged(int iValue);
  void RLaneWidthChanged(int iValue);
  void LShoulderWidthChanged(int iValue);
  void RShoulderWidthChanged(int iValue);
  void LShoulderHeightChanged(int iValue);
  void RShoulderHeightChanged(int iValue);
  void WallHeightChanged(int iValue);
  void LLOWallHOffsetChanged(int iValue);
  void LLOWallHeightChanged(int iValue);
  void LUOWallHOffsetChanged(int iValue);
  void LUOWallHeightChanged(int iValue);
  void RLOWallHOffsetChanged(int iValue);
  void RLOWallHeightChanged(int iValue);
  void RUOWallHOffsetChanged(int iValue);
  void RUOWallHeightChanged(int iValue);
  void LOuterFloorHOffsetChanged(int iValue);
  void ROuterFloorHOffsetChanged(int iValue);
  void LOuterFloorHeightChanged(int iValue);
  void ROuterFloorHeightChanged(int iValue);
  void AILine1Changed(int iValue);
  void AILine2Changed(int iValue);
  void AILine3Changed(int iValue);
  void AILine4Changed(int iValue);
  void EditCSurface();
  void EditLShoulder();
  void EditRShoulder();
  void EditLWall();
  void EditRWall();
  void EditBack();
  void EditRoof();
  void EditLUOWall();
  void EditLLOWall();
  void EditRLOWall();
  void EditRUOWall();
  void EditOFloor();
  void CGripChanged(int iValue);
  void LGripChanged(int iValue);
  void RGripChanged(int iValue);
  void AISpeedChanged(int iValue);
  void AIAccuracyChanged(int iValue);

private:
  CEditGeometryWidgetPrivate *p;
};

//-------------------------------------------------------------------------------------------------
#endif