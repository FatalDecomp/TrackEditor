#ifndef _TRACKEDITOR_EDITSERIESDLG_H
#define _TRACKEDITOR_EDITSERIESDLG_H
//-------------------------------------------------------------------------------------------------
#include "ui_EditSeriesDialog.h"
#include <vector>
//-------------------------------------------------------------------------------------------------
class CEditSeriesDialog : public QWidget, private Ui::EditSeriesDialog
{
  Q_OBJECT

public:
  CEditSeriesDialog(QWidget *pParent);
  ~CEditSeriesDialog();

  int GetStartChunk();
  int GetEndChunk();
  int GetInterval();
  int GetField();
  QString GetStartValue();
  QString GetEndValue();
  QString GetIncrement();

protected slots:
  void OnUpdateWindow();
  void OnRateChanged(const QString &sText);

private:
  void Validate();
  bool IsDirectionValid(double dStart, double dEnd, double dIncrement);
  int ToInt(QString sText);
  template <typename T> void ApplySeriesToGeometry(int iStartChunk, 
                                                   int iEndChunk, 
                                                   int iInterval, 
                                                   int iField, 
                                                   T tStartValue, 
                                                   T tEndValue, 
                                                   T tIncrement);

  int m_iStartChunk;
  int m_iEndChunk;
  int m_iInterval;
  int m_iField;
  QString m_sStartValue;
  QString m_sEndValue;
  QString m_sIncrement;
  QString chunkFields[81] = {
    "Left shoulder width",
    "Left lane width",
    "Right lane width",
    "Right shoulder width",
    "Left shoulder height",
    "Right shoulder height",
    "Length",
    "Yaw",
    "Pitch",
    "Roll",
    "AI Line 1",
    "AI Line 2",
    "AI Line 3",
    "AI Line 4",
    "Center grip",
    "Left shoulder grip",
    "Right shoulder grip",
    "AI max speed",
    "Ground height",
    "Audio above trigger",
    "Audio trigger speed",
    "Audio below trigger",
    "Left surface type",
    "Center surface type",
    "Right surface type",
    "Left wall type",
    "Right wall type",
    "Roof type",
    "Left upper outer wall type",
    "Left lower outer wall type",
    "Outer floor type",
    "Right lower outer wall type",
    "Right upper outer wall type",
    "Environment floor type",
    "Sign type",
    "Sign horizontal offset",
    "Sign vertical offset",
    "Sign yaw",
    "Sign pitch",
    "Sign roll",
    "Left upper outer wall horizontal offset",
    "Left lower outer wall horizontal offset",
    "Left outer floor horizontal offset",
    "Right outer floor horizontal offset",
    "Right lower outer wall horizontal offset",
    "Right upper outer wall horizontal offset",
    "Left upper outer wall height",
    "Left lower outer wall height",
    "Left outer floor height",
    "Right outer floor height",
    "Right lower outer wall height",
    "Right upper outer wall height",
    "Wall/roof height",
    "Near chunks (forward)",
    "Near chunks (forward extra start)",
    "Near chunks (forward extra)",
    "Subdivide distance (left shoulder)",
    "Subdivide distance (center)",
    "Subdivide distance (right shoulder)",
    "Subdivide distance (left wall)",
    "Subdivide distance (right wall)",
    "Subdivide distance (roof)",
    "Subdivide distance (left upper outer wall)",
    "Subdivide distance (left lower outer wall)",
    "Subdivide distance (outer floor)",
    "Subdivide distance (right lower outer wall)",
    "Subdivide distance (right upper outer wall)",
    "Near chunks (backward)",
    "Near chunks (backward extra start)",
    "Near chunks (backward extra)",
    "Sign texture",
    "Back texture",
    "Stunt ramp chunk count",
    "Stunt ticks",
    "Stunt tick start index",
    "Stunt timing group",
    "Stunt height delta/tick",
    "Stunt time at peak",
    "Stunt time flat",
    "Stunt ramp chunk length",
    "Stunt section flags"
  };
  size_t chunkFieldsSize = sizeof(CEditSeriesDialog::chunkFields) / sizeof(CEditSeriesDialog::chunkFields[0]);
};

//-------------------------------------------------------------------------------------------------
#endif