#ifndef _TRACKEDITOR_GRAPHICSDIALOG_H
#define _TRACKEDITOR_GRAPHICSDIALOG_H

#include "ui_GraphicsDialog.h"

class CGraphicsDialog : public QDialog, private Ui::GraphicsDialog
{
  Q_OBJECT

public:
  explicit CGraphicsDialog(QWidget *pParent);
  ~CGraphicsDialog() override;

private slots:
  void HardwareRenderingToggled(bool bChecked);
  void DialogEdited();

private:
  void UpdateHardwareControls();
};

#endif
