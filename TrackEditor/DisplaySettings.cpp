#include "TrackEditor.h"
#include "DisplaySettings.h"
#include "QtHelpers.h"
#include "NoclipComponent.h"
//-------------------------------------------------------------------------------------------------
#if defined(_DEBUG) && defined(IS_WINDOWS)
#define new new(_CLIENT_BLOCK, __FILE__, __LINE__)
#endif
//-------------------------------------------------------------------------------------------------

CDisplaySettings::CDisplaySettings(QWidget *pParent)
  : QWidget(pParent)
{
  setupUi(this);

  ckAllSurface->setChecked(true);
  ckHighlightSelection->setChecked(true);
  ckAttachLast->setChecked(true);
  ckAILines->setChecked(true);
  ckSigns->setChecked(true);
  ckTestCar->setChecked(true);
  ckAudio->setChecked(true);
  ckStunts->setChecked(true);
  ckEnvironment->setChecked(true);

  cbTestCarType->addItem("F1WACK", (int)eWhipModel::CAR_F1WACK);
  cbTestCarType->addItem("XAUTO", (int)eWhipModel::CAR_XAUTO);
  cbTestCarType->addItem("XDESILVA", (int)eWhipModel::CAR_XDESILVA);
  cbTestCarType->addItem("XPULSE", (int)eWhipModel::CAR_XPULSE);
  cbTestCarType->addItem("XGLOBAL", (int)eWhipModel::CAR_XGLOBAL);
  cbTestCarType->addItem("XMILLION", (int)eWhipModel::CAR_XMILLION);
  cbTestCarType->addItem("XMISSION", (int)eWhipModel::CAR_XMISSION);
  cbTestCarType->addItem("XZIZIN", (int)eWhipModel::CAR_XZIZIN);
  cbTestCarType->addItem("XREISE", (int)eWhipModel::CAR_XREISE);
  cbTestCarType->addItem("YAUTO", (int)eWhipModel::CAR_YAUTO);
  cbTestCarType->addItem("YDESILVA", (int)eWhipModel::CAR_YDESILVA);
  cbTestCarType->addItem("YPULSE", (int)eWhipModel::CAR_YPULSE);
  cbTestCarType->addItem("YGLOBAL", (int)eWhipModel::CAR_YGLOBAL);
  cbTestCarType->addItem("YMILLION", (int)eWhipModel::CAR_YMILLION);
  cbTestCarType->addItem("YMISSION", (int)eWhipModel::CAR_YMISSION);
  cbTestCarType->addItem("YZIZIN", (int)eWhipModel::CAR_YZIZIN);
  cbTestCarType->addItem("YREISE", (int)eWhipModel::CAR_YREISE);
  cbTestCarType->addItem("DEATH", (int)eWhipModel::CAR_DEATH);
  cbTestCarType->setCurrentIndex(cbTestCarType->findData((int)eWhipModel::CAR_YZIZIN));

  cbTestCarPos->addItem("AILINE 1", (int)eShapeSection::AILINE1);
  cbTestCarPos->addItem("AILINE 2", (int)eShapeSection::AILINE2);
  cbTestCarPos->addItem("AILINE 3", (int)eShapeSection::AILINE3);
  cbTestCarPos->addItem("AILINE 4", (int)eShapeSection::AILINE4);
  cbTestCarPos->setCurrentIndex(cbTestCarPos->findData((int)eShapeSection::AILINE1));

  connect(ckAllSurface, &QCheckBox::toggled, this, &CDisplaySettings::UpdateAllSurface);
  connect(ckAllWireframe, &QCheckBox::toggled, this, &CDisplaySettings::UpdateAllWireframe);
  connect(ckCenterSurface, &QCheckBox::toggled, this, &CDisplaySettings::UpdatePreviewSelection);
  connect(ckCenterWireframe, &QCheckBox::toggled, this, &CDisplaySettings::UpdatePreviewSelection);
  connect(ckLShoulderSurface, &QCheckBox::toggled, this, &CDisplaySettings::UpdatePreviewSelection);
  connect(ckLShoulderWireframe, &QCheckBox::toggled, this, &CDisplaySettings::UpdatePreviewSelection);
  connect(ckRShoulderSurface, &QCheckBox::toggled, this, &CDisplaySettings::UpdatePreviewSelection);
  connect(ckRShoulderWireframe, &QCheckBox::toggled, this, &CDisplaySettings::UpdatePreviewSelection);
  connect(ckLWallSurface, &QCheckBox::toggled, this, &CDisplaySettings::UpdatePreviewSelection);
  connect(ckLWallWireframe, &QCheckBox::toggled, this, &CDisplaySettings::UpdatePreviewSelection);
  connect(ckRWallSurface, &QCheckBox::toggled, this, &CDisplaySettings::UpdatePreviewSelection);
  connect(ckRWallWireframe, &QCheckBox::toggled, this, &CDisplaySettings::UpdatePreviewSelection);
  connect(ckRoofSurface, &QCheckBox::toggled, this, &CDisplaySettings::UpdatePreviewSelection);
  connect(ckRoofWireframe, &QCheckBox::toggled, this, &CDisplaySettings::UpdatePreviewSelection);
  connect(ckLUOWallSurface, &QCheckBox::toggled, this, &CDisplaySettings::UpdatePreviewSelection);
  connect(ckLUOWallWireframe, &QCheckBox::toggled, this, &CDisplaySettings::UpdatePreviewSelection);
  connect(ckLLOWallSurface, &QCheckBox::toggled, this, &CDisplaySettings::UpdatePreviewSelection);
  connect(ckLLOWallWireframe, &QCheckBox::toggled, this, &CDisplaySettings::UpdatePreviewSelection);
  connect(ckRUOWallSurface, &QCheckBox::toggled, this, &CDisplaySettings::UpdatePreviewSelection);
  connect(ckRUOWallWireframe, &QCheckBox::toggled, this, &CDisplaySettings::UpdatePreviewSelection);
  connect(ckRLOWallSurface, &QCheckBox::toggled, this, &CDisplaySettings::UpdatePreviewSelection);
  connect(ckRLOWallWireframe, &QCheckBox::toggled, this, &CDisplaySettings::UpdatePreviewSelection);
  connect(ckOWallFloorSurface, &QCheckBox::toggled, this, &CDisplaySettings::UpdatePreviewSelection);
  connect(ckOWallFloorWireframe, &QCheckBox::toggled, this, &CDisplaySettings::UpdatePreviewSelection);
  connect(ckEnvironment, &QCheckBox::toggled, this, &CDisplaySettings::UpdatePreviewSelection);
  connect(ckHighlightSelection, &QCheckBox::toggled, this, &CDisplaySettings::UpdatePreviewSelection);
  connect(ckAILines, &QCheckBox::toggled, this, &CDisplaySettings::UpdatePreviewSelection);
  connect(ckSigns, &QCheckBox::toggled, this, &CDisplaySettings::UpdatePreviewSelection);
  connect(ckAudio, &QCheckBox::toggled, this, &CDisplaySettings::UpdatePreviewSelection);
  connect(ckStunts, &QCheckBox::toggled, this, &CDisplaySettings::UpdatePreviewSelection);
  connect(ckAnimateStunts, &QCheckBox::toggled, this, &CDisplaySettings::UpdatePreviewSelection);
  connect(ckTestCar, &QCheckBox::toggled, this, &CDisplaySettings::UpdatePreviewSelection);
  connect(cbTestCarType, SIGNAL(currentIndexChanged(int)), this, SLOT(UpdatePreviewSelection()));
  connect(cbTestCarPos, SIGNAL(currentIndexChanged(int)), this, SLOT(UpdatePreviewSelection()));
  connect(ckMillionPlus, &QCheckBox::toggled, this, &CDisplaySettings::UpdatePreviewSelection);
  connect(ckAttachLast, &QCheckBox::toggled, this, &CDisplaySettings::AttachLastCheckedSig);
  connect(sbCameraSpeed, SIGNAL(valueChanged(int)), this, SLOT(OnCameraSpeedChanged(int)));

  connect(ckRefModel, &QCheckBox::toggled, this, &CDisplaySettings::UpdatePreviewSelection);
  connect(pbRefBrowse, &QPushButton::pressed, this, &CDisplaySettings::OpenReferenceModelSig);

  UpdateAllSurface();
  UpdateAllWireframe();
}

//-------------------------------------------------------------------------------------------------

CDisplaySettings::~CDisplaySettings()
{
}

//-------------------------------------------------------------------------------------------------

uint32 CDisplaySettings::GetDisplaySettings(eWhipModel &carModel, eShapeSection &aiLine, bool &bMillionPlus)
{
  uint32 uiShowModels = 0;
  if (ckCenterSurface->isChecked())       uiShowModels |= SHOW_CENTER_SURF_MODEL;
  if (ckCenterWireframe->isChecked())     uiShowModels |= SHOW_CENTER_WIRE_MODEL;
  if (ckLShoulderSurface->isChecked())    uiShowModels |= SHOW_LSHOULDER_SURF_MODEL;
  if (ckLShoulderWireframe->isChecked())  uiShowModels |= SHOW_LSHOULDER_WIRE_MODEL;
  if (ckRShoulderSurface->isChecked())    uiShowModels |= SHOW_RSHOULDER_SURF_MODEL;
  if (ckRShoulderWireframe->isChecked())  uiShowModels |= SHOW_RSHOULDER_WIRE_MODEL;
  if (ckLWallSurface->isChecked())        uiShowModels |= SHOW_LWALL_SURF_MODEL;
  if (ckLWallWireframe->isChecked())      uiShowModels |= SHOW_LWALL_WIRE_MODEL;
  if (ckRWallSurface->isChecked())        uiShowModels |= SHOW_RWALL_SURF_MODEL;
  if (ckRWallWireframe->isChecked())      uiShowModels |= SHOW_RWALL_WIRE_MODEL;
  if (ckRoofSurface->isChecked())         uiShowModels |= SHOW_ROOF_SURF_MODEL;
  if (ckRoofWireframe->isChecked())       uiShowModels |= SHOW_ROOF_WIRE_MODEL;
  if (ckLLOWallSurface->isChecked())      uiShowModels |= SHOW_LLOWALL_SURF_MODEL;
  if (ckLLOWallWireframe->isChecked())    uiShowModels |= SHOW_LLOWALL_WIRE_MODEL;
  if (ckLUOWallSurface->isChecked())      uiShowModels |= SHOW_LUOWALL_SURF_MODEL;
  if (ckLUOWallWireframe->isChecked())    uiShowModels |= SHOW_LUOWALL_WIRE_MODEL;
  if (ckRUOWallSurface->isChecked())      uiShowModels |= SHOW_RUOWALL_SURF_MODEL;
  if (ckRUOWallWireframe->isChecked())    uiShowModels |= SHOW_RUOWALL_WIRE_MODEL;
  if (ckRLOWallSurface->isChecked())      uiShowModels |= SHOW_RLOWALL_SURF_MODEL;
  if (ckRLOWallWireframe->isChecked())    uiShowModels |= SHOW_RLOWALL_WIRE_MODEL;
  if (ckOWallFloorSurface->isChecked())   uiShowModels |= SHOW_OWALLFLOOR_SURF_MODEL;
  if (ckOWallFloorWireframe->isChecked()) uiShowModels |= SHOW_OWALLFLOOR_WIRE_MODEL;
  if (ckEnvironment->isChecked())         uiShowModels |= SHOW_ENVIRONMENT;
  if (ckHighlightSelection->isChecked())  uiShowModels |= SHOW_SELECTION_HIGHLIGHT;
  if (ckAILines->isChecked())             uiShowModels |= SHOW_AILINE_MODELS;
  if (ckTestCar->isChecked())             uiShowModels |= SHOW_TEST_CAR;
  if (ckSigns->isChecked())               uiShowModels |= SHOW_SIGNS;
  if (ckAudio->isChecked())               uiShowModels |= SHOW_AUDIO;
  if (ckStunts->isChecked())              uiShowModels |= SHOW_STUNTS;
  if (ckAnimateStunts->isChecked())       uiShowModels |= ANIMATE_STUNTS;
  if (ckRefModel->isChecked())            uiShowModels |= SHOW_REF_MODEL;

  carModel = (eWhipModel)cbTestCarType->currentData().toInt();
  aiLine = (eShapeSection)cbTestCarPos->currentData().toInt();
  bMillionPlus = ckMillionPlus->isChecked();

  return uiShowModels;
}

//-------------------------------------------------------------------------------------------------

void CDisplaySettings::SetDisplaySettings(uint32 uiShowModels, eWhipModel carModel, eShapeSection aiLine, bool bMillionPlus)
{
  BLOCK_SIG_AND_DO(ckCenterSurface, setChecked(      uiShowModels & SHOW_CENTER_SURF_MODEL));
  BLOCK_SIG_AND_DO(ckCenterWireframe, setChecked(    uiShowModels & SHOW_CENTER_WIRE_MODEL));
  BLOCK_SIG_AND_DO(ckLShoulderSurface, setChecked(   uiShowModels & SHOW_LSHOULDER_SURF_MODEL));
  BLOCK_SIG_AND_DO(ckLShoulderWireframe, setChecked( uiShowModels & SHOW_LSHOULDER_WIRE_MODEL));
  BLOCK_SIG_AND_DO(ckRShoulderSurface, setChecked(   uiShowModels & SHOW_RSHOULDER_SURF_MODEL));
  BLOCK_SIG_AND_DO(ckRShoulderWireframe, setChecked( uiShowModels & SHOW_RSHOULDER_WIRE_MODEL));
  BLOCK_SIG_AND_DO(ckLWallSurface, setChecked(       uiShowModels & SHOW_LWALL_SURF_MODEL));
  BLOCK_SIG_AND_DO(ckLWallWireframe, setChecked(     uiShowModels & SHOW_LWALL_WIRE_MODEL));
  BLOCK_SIG_AND_DO(ckRWallSurface, setChecked(       uiShowModels & SHOW_RWALL_SURF_MODEL));
  BLOCK_SIG_AND_DO(ckRWallWireframe, setChecked(     uiShowModels & SHOW_RWALL_WIRE_MODEL));
  BLOCK_SIG_AND_DO(ckRoofSurface, setChecked(        uiShowModels & SHOW_ROOF_SURF_MODEL));
  BLOCK_SIG_AND_DO(ckRoofWireframe, setChecked(      uiShowModels & SHOW_ROOF_WIRE_MODEL));
  BLOCK_SIG_AND_DO(ckLLOWallSurface, setChecked(     uiShowModels & SHOW_LLOWALL_SURF_MODEL));
  BLOCK_SIG_AND_DO(ckLLOWallWireframe, setChecked(   uiShowModels & SHOW_LLOWALL_WIRE_MODEL));
  BLOCK_SIG_AND_DO(ckLUOWallSurface, setChecked(     uiShowModels & SHOW_LUOWALL_SURF_MODEL));
  BLOCK_SIG_AND_DO(ckLUOWallWireframe, setChecked(   uiShowModels & SHOW_LUOWALL_WIRE_MODEL));
  BLOCK_SIG_AND_DO(ckRUOWallSurface, setChecked(     uiShowModels & SHOW_RUOWALL_SURF_MODEL));
  BLOCK_SIG_AND_DO(ckRUOWallWireframe, setChecked(   uiShowModels & SHOW_RUOWALL_WIRE_MODEL));
  BLOCK_SIG_AND_DO(ckRLOWallSurface, setChecked(     uiShowModels & SHOW_RLOWALL_SURF_MODEL));
  BLOCK_SIG_AND_DO(ckRLOWallWireframe, setChecked(   uiShowModels & SHOW_RLOWALL_WIRE_MODEL));
  BLOCK_SIG_AND_DO(ckOWallFloorSurface, setChecked(  uiShowModels & SHOW_OWALLFLOOR_SURF_MODEL));
  BLOCK_SIG_AND_DO(ckOWallFloorWireframe, setChecked(uiShowModels & SHOW_OWALLFLOOR_WIRE_MODEL));
  BLOCK_SIG_AND_DO(ckEnvironment, setChecked(        uiShowModels & SHOW_ENVIRONMENT));
  BLOCK_SIG_AND_DO(ckHighlightSelection, setChecked( uiShowModels & SHOW_SELECTION_HIGHLIGHT));
  BLOCK_SIG_AND_DO(ckAILines, setChecked(            uiShowModels & SHOW_AILINE_MODELS));
  BLOCK_SIG_AND_DO(ckTestCar, setChecked(            uiShowModels & SHOW_TEST_CAR));
  BLOCK_SIG_AND_DO(ckSigns, setChecked(              uiShowModels & SHOW_SIGNS));
  BLOCK_SIG_AND_DO(ckAudio, setChecked(              uiShowModels & SHOW_AUDIO));
  BLOCK_SIG_AND_DO(ckStunts, setChecked(             uiShowModels & SHOW_STUNTS));
  BLOCK_SIG_AND_DO(ckAnimateStunts, setChecked(      uiShowModels & ANIMATE_STUNTS));
  BLOCK_SIG_AND_DO(ckRefModel, setChecked(           uiShowModels & SHOW_REF_MODEL));

  BLOCK_SIG_AND_DO(cbTestCarType, setCurrentIndex(cbTestCarType->findData((int)carModel)));
  BLOCK_SIG_AND_DO(cbTestCarPos, setCurrentIndex(cbTestCarPos->findData((int)aiLine)));
  BLOCK_SIG_AND_DO(ckMillionPlus, setChecked(bMillionPlus));

  UpdatePreviewSelection();
}

//-------------------------------------------------------------------------------------------------

bool CDisplaySettings::GetAttachLast()
{
  return ckAttachLast->isChecked();
}

//-------------------------------------------------------------------------------------------------

void CDisplaySettings::SetAttachLast(bool bAttachLast)
{
  ckAttachLast->setChecked(bAttachLast);
}

//-------------------------------------------------------------------------------------------------

int CDisplaySettings::GetCameraSpeed()
{
  return sbCameraSpeed->value();
}

//-------------------------------------------------------------------------------------------------

void CDisplaySettings::SetCameraSpeed(int iSpeed)
{
  sbCameraSpeed->setValue(iSpeed);
}

//-------------------------------------------------------------------------------------------------

void CDisplaySettings::UpdateAllSurface()
{
  BLOCK_SIG_AND_DO(ckCenterSurface, setChecked(ckAllSurface->isChecked()));
  BLOCK_SIG_AND_DO(ckLShoulderSurface, setChecked(ckAllSurface->isChecked()));
  BLOCK_SIG_AND_DO(ckRShoulderSurface, setChecked(ckAllSurface->isChecked()));
  BLOCK_SIG_AND_DO(ckLWallSurface, setChecked(ckAllSurface->isChecked()));
  BLOCK_SIG_AND_DO(ckRWallSurface, setChecked(ckAllSurface->isChecked()));
  BLOCK_SIG_AND_DO(ckRoofSurface, setChecked(ckAllSurface->isChecked()));
  BLOCK_SIG_AND_DO(ckLUOWallSurface, setChecked(ckAllSurface->isChecked()));
  BLOCK_SIG_AND_DO(ckLLOWallSurface, setChecked(ckAllSurface->isChecked()));
  BLOCK_SIG_AND_DO(ckRUOWallSurface, setChecked(ckAllSurface->isChecked()));
  BLOCK_SIG_AND_DO(ckRLOWallSurface, setChecked(ckAllSurface->isChecked()));
  BLOCK_SIG_AND_DO(ckOWallFloorSurface, setChecked(ckAllSurface->isChecked()));

  UpdatePreviewSig();
}

//-------------------------------------------------------------------------------------------------

void CDisplaySettings::UpdateAllWireframe()
{
  BLOCK_SIG_AND_DO(ckCenterWireframe, setChecked(ckAllWireframe->isChecked()));
  BLOCK_SIG_AND_DO(ckLShoulderWireframe, setChecked(ckAllWireframe->isChecked()));
  BLOCK_SIG_AND_DO(ckRShoulderWireframe, setChecked(ckAllWireframe->isChecked()));
  BLOCK_SIG_AND_DO(ckLWallWireframe, setChecked(ckAllWireframe->isChecked()));
  BLOCK_SIG_AND_DO(ckRWallWireframe, setChecked(ckAllWireframe->isChecked()));
  BLOCK_SIG_AND_DO(ckRoofWireframe, setChecked(ckAllWireframe->isChecked()));
  BLOCK_SIG_AND_DO(ckLUOWallWireframe, setChecked(ckAllWireframe->isChecked()));
  BLOCK_SIG_AND_DO(ckLLOWallWireframe, setChecked(ckAllWireframe->isChecked()));
  BLOCK_SIG_AND_DO(ckRUOWallWireframe, setChecked(ckAllWireframe->isChecked()));
  BLOCK_SIG_AND_DO(ckRLOWallWireframe, setChecked(ckAllWireframe->isChecked()));
  BLOCK_SIG_AND_DO(ckOWallFloorWireframe, setChecked(ckAllWireframe->isChecked()));

  UpdatePreviewSig();
}

//-------------------------------------------------------------------------------------------------

void CDisplaySettings::UpdatePreviewSelection()
{
  bool bAllSurfaceChecked = true;
  bool bAllWireframeChecked = true;
  bAllSurfaceChecked &= ckCenterSurface->isChecked();
  bAllWireframeChecked &= ckCenterWireframe->isChecked();
  bAllSurfaceChecked &= ckLShoulderSurface->isChecked();
  bAllWireframeChecked &= ckLShoulderWireframe->isChecked();
  bAllSurfaceChecked &= ckRShoulderSurface->isChecked();
  bAllWireframeChecked &= ckRShoulderWireframe->isChecked();
  bAllSurfaceChecked &= ckLWallSurface->isChecked();
  bAllWireframeChecked &= ckLWallWireframe->isChecked();
  bAllSurfaceChecked &= ckRWallSurface->isChecked();
  bAllWireframeChecked &= ckRWallWireframe->isChecked();
  bAllSurfaceChecked &= ckRoofSurface->isChecked();
  bAllWireframeChecked &= ckRoofWireframe->isChecked();
  bAllSurfaceChecked &= ckLUOWallSurface->isChecked();
  bAllWireframeChecked &= ckLUOWallWireframe->isChecked();
  bAllSurfaceChecked &= ckLLOWallSurface->isChecked();
  bAllWireframeChecked &= ckLLOWallWireframe->isChecked();
  bAllSurfaceChecked &= ckRUOWallSurface->isChecked();
  bAllWireframeChecked &= ckRUOWallWireframe->isChecked();
  bAllSurfaceChecked &= ckRLOWallSurface->isChecked();
  bAllWireframeChecked &= ckRLOWallWireframe->isChecked();
  bAllSurfaceChecked &= ckOWallFloorSurface->isChecked();
  bAllWireframeChecked &= ckOWallFloorWireframe->isChecked();

  BLOCK_SIG_AND_DO(ckAllSurface, setChecked(bAllSurfaceChecked));
  BLOCK_SIG_AND_DO(ckAllWireframe, setChecked(bAllWireframeChecked));

  UpdatePreviewSig();
}

//-------------------------------------------------------------------------------------------------

void CDisplaySettings::OnCameraSpeedChanged(int iSpeed)
{
  CNoclipComponent::s_fMovementSpeed = (float)iSpeed;
}

//-------------------------------------------------------------------------------------------------