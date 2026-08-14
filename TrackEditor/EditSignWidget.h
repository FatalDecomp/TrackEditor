#ifndef _TRACKEDITOR_EDITSIGNWIDGET_H
#define _TRACKEDITOR_EDITSIGNWIDGET_H
//-------------------------------------------------------------------------------------------------
#include "ui_EditSignWidget.h"
//-------------------------------------------------------------------------------------------------
class CEditSignWidgetPrivate;
class CTrack;
//-------------------------------------------------------------------------------------------------

class CEditSignWidget : public QWidget, private Ui::EditSignWidget
{
  Q_OBJECT

public:
  CEditSignWidget(QWidget *pParent);
  ~CEditSignWidget();

protected slots:
  void UpdateGeometrySelection(int iFrom, int iTo);
  void YawChanged(double dVal);
  void PitchChanged(double dVal);
  void RollChanged(double dVal);
  void HOffsetChanged(int iVal);
  void VOffsetChanged(int iVal);
  void TypeChanged(int iIndex);
  void EditClicked();
  void SignClicked();
  void UnkChanged(const QString &sText);

private:
  bool GetSelection(CTrack *&pTrackOut, int &iFromOut, int &iToOut) const;
  void CommitEdit(int iEdited, const QString &sDescription);
};

//-------------------------------------------------------------------------------------------------
#endif
