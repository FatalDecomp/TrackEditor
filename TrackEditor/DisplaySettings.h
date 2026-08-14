#ifndef _TRACKEDITOR_DISPLAYSETTINGS_H
#define _TRACKEDITOR_DISPLAYSETTINGS_H
//-------------------------------------------------------------------------------------------------
#include "ui_DisplaySettings.h"
#include "DisplaySettingsFlags.h"
#include "Types.h"
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

class CDisplaySettings : public QWidget, private Ui::DisplaySettings
{
  Q_OBJECT

public:
  CDisplaySettings(QWidget *pParent);
  ~CDisplaySettings();

  uint32 GetDisplaySettings(eWhipModel &carModel, eShapeSection &aiLine, bool &bMillionPlus);
  void SetDisplaySettings(uint32 uiShowModels, eWhipModel carModel, eShapeSection aiLine, bool bMillionPlus);
  uint32 GetFeatureSettings() const;
  void SetFeatureSettings(uint32 uiShowFeatures);
  bool GetAnimateStunts() const;
  void SetAnimateStunts(bool bAnimate);
  bool GetAttachLast();
  void SetAttachLast(bool bAttachLast);
  int GetCameraSpeed();
  void SetCameraSpeed(int iSpeed);
  void SetReferenceModel(const QString &sFile,
                         double dYaw, double dPitch, double dRoll,
                         int iX, int iY, int iZ,
                         double dScale);

protected slots:
  void UpdateAllSurface();
  void UpdateAllWireframe();
  void UpdatePreviewSelection();
  void OnCameraSpeedChanged(int iSpeed);
  void OnReferenceModelPosChanged();

signals:
  void AttachLastCheckedSig(bool bChecked);
  void UpdatePreviewSig();
  void OpenReferenceModelSig();
  void RefModelPosSig(double dYaw, double dPitch, double dRoll,
                      int iX, int iY, int iZ,
                      double dScale);
};

//-------------------------------------------------------------------------------------------------
#endif
