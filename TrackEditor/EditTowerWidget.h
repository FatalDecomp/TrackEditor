#ifndef TRACKEDITOR_EDITTOWERWIDGET_H
#define TRACKEDITOR_EDITTOWERWIDGET_H

#include "ui_EditTowerWidget.h"

class CTrack;

class CEditTowerWidget : public QWidget, private Ui::EditTowerWidget
{
  Q_OBJECT

public:
  explicit CEditTowerWidget(QWidget *pParent);
  ~CEditTowerWidget();

protected slots:
  void UpdateGeometrySelection(int iFrom, int iTo);
  void TowerClicked();
  void ModeChanged(int iIndex);
  void ZoomChanged(int iIndex);
  void HOffsetChanged(int iValue);
  void VOffsetChanged(int iValue);
  void RawTypeChanged(const QString &sText);

private:
  bool GetSelection(CTrack *&pTrackOut, int &iFromOut, int &iToOut) const;
  void CommitEdit(int iChanged, const QString &sDescription);
};

#endif
