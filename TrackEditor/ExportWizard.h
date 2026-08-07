#ifndef _TRACKEDITOR_EXPORTWIZARD_H
#define _TRACKEDITOR_EXPORTWIZARD_H
//-------------------------------------------------------------------------------------------------
#include "ui_ExportWizard.h"
#include "TrackPreview.h"
//-------------------------------------------------------------------------------------------------

class CExportWizard : public QDialog, private Ui::ExportWizard
{
  Q_OBJECT

public:
  explicit CExportWizard(QWidget *pParent);
  ~CExportWizard();

  bool m_bExportSeparate;
  bool m_bExportBacks;
  bool m_bExportSigns;

protected slots:
  void OnSeparateChecked(bool bChecked);
  void OnBacksChecked(bool bChecked);
  void OnSignsChecked(bool bChecked);
};

//-------------------------------------------------------------------------------------------------
#endif