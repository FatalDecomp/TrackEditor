#ifndef TRACKEDITOR_EDITORBATCHEXPORTER_H
#define TRACKEDITOR_EDITORBATCHEXPORTER_H

#include "EditorExportFormat.h"

#include <QString>
#include <QStringList>

class CEditorRenderService;
class QWidget;

struct tEdBatchExportResult
{
  int iSucceeded = 0;
  int iFailed = 0;
  bool bCancelled = false;
  QStringList Failures;
};

class CEditorBatchExporter
{
public:
  CEditorBatchExporter(QWidget *pParent,
                       CEditorRenderService *pRenderService);

  static bool IsFatdataFolder(const QString &sFolder);
  tEdBatchExportResult Export(const QString &sFatdataFolder,
                              const QString &sOutputFolder,
                              eExportType exportType);

private:
  QWidget *m_pParent;
  CEditorRenderService *m_pRenderService;
};

#endif
