#include "EditGeometryWidget.h"
#include "Track.h"
#include "Texture.h"
#include "Palette.h"
#include "MainWindow.h"
#include "QtHelpers.h"
#include "EditSurfaceDialog.h"
//-------------------------------------------------------------------------------------------------
#if defined(_DEBUG) && defined(IS_WINDOWS)
#define new new(_CLIENT_BLOCK, __FILE__, __LINE__)
#endif
//-------------------------------------------------------------------------------------------------

CEditGeometryWidget::CEditGeometryWidget(QWidget *pParent)
  : QWidget(pParent)
{

  setupUi(this);

  connect(g_pMainWindow, &CMainWindow::UpdateGeometrySelectionSig, this, &CEditGeometryWidget::UpdateGeometrySelection);

  lblRoofDisabled->setStyleSheet("QLabel { color : red; }");
  lblLLOWallDisabled->setStyleSheet("QLabel { color : red; }");
  lblLUOWallDisabled->setStyleSheet("QLabel { color : red; }");
  lblRLOWallDisabled->setStyleSheet("QLabel { color : red; }");
  lblRUOWallDisabled->setStyleSheet("QLabel { color : red; }");
  
  connect(dsbYaw              , SIGNAL(valueChanged(double)), this, SLOT(YawChanged(double)));
  connect(dsbPitch            , SIGNAL(valueChanged(double)), this, SLOT(PitchChanged(double)));
  connect(dsbRoll             , SIGNAL(valueChanged(double)), this, SLOT(RollChanged(double)));
  connect(sbLength            , SIGNAL(valueChanged(int)), this, SLOT(LengthChanged(int)));
  connect(sbLLaneWidth        , SIGNAL(valueChanged(int)), this, SLOT(LLaneWidthChanged (int)));
  connect(sbRLaneWidth        , SIGNAL(valueChanged(int)), this, SLOT(RLaneWidthChanged (int)));
  connect(sbLShoulderWidth    , SIGNAL(valueChanged(int)), this, SLOT(LShoulderWidthChanged(int)));
  connect(sbRShoulderWidth    , SIGNAL(valueChanged(int)), this, SLOT(RShoulderWidthChanged(int)));
  connect(sbLShoulderHeight   , SIGNAL(valueChanged(int)), this, SLOT(LShoulderHeightChanged(int)));
  connect(sbRShoulderHeight   , SIGNAL(valueChanged(int)), this, SLOT(RShoulderHeightChanged(int)));
  connect(sbWallHeight        , SIGNAL(valueChanged(int)), this, SLOT(WallHeightChanged(int)));
  connect(sbLLOWallHOffset    , SIGNAL(valueChanged(int)), this, SLOT(LLOWallHOffsetChanged(int)));
  connect(sbLLOWallHeight     , SIGNAL(valueChanged(int)), this, SLOT(LLOWallHeightChanged(int)));
  connect(sbLUOWallHOffset    , SIGNAL(valueChanged(int)), this, SLOT(LUOWallHOffsetChanged(int)));
  connect(sbLUOWallHeight     , SIGNAL(valueChanged(int)), this, SLOT(LUOWallHeightChanged(int)));
  connect(sbRLOWallHOffset    , SIGNAL(valueChanged(int)), this, SLOT(RLOWallHOffsetChanged(int)));
  connect(sbRLOWallHeight     , SIGNAL(valueChanged(int)), this, SLOT(RLOWallHeightChanged(int)));
  connect(sbRUOWallHOffset    , SIGNAL(valueChanged(int)), this, SLOT(RUOWallHOffsetChanged(int)));
  connect(sbRUOWallHeight     , SIGNAL(valueChanged(int)), this, SLOT(RUOWallHeightChanged(int)));
  connect(sbLOuterFloorHOffset, SIGNAL(valueChanged(int)), this, SLOT(LOuterFloorHOffsetChanged(int)));
  connect(sbROuterFloorHOffset, SIGNAL(valueChanged(int)), this, SLOT(ROuterFloorHOffsetChanged(int)));
  connect(sbLOuterFloorHeight , SIGNAL(valueChanged(int)), this, SLOT(LOuterFloorHeightChanged(int)));
  connect(sbROuterFloorHeight , SIGNAL(valueChanged(int)), this, SLOT(ROuterFloorHeightChanged(int)));
  connect(sbAILine1           , SIGNAL(valueChanged(int)), this, SLOT(AILine1Changed(int)));
  connect(sbAILine2           , SIGNAL(valueChanged(int)), this, SLOT(AILine2Changed(int)));
  connect(sbAILine3           , SIGNAL(valueChanged(int)), this, SLOT(AILine3Changed(int)));
  connect(sbAILine4           , SIGNAL(valueChanged(int)), this, SLOT(AILine4Changed(int)));
  connect(sbGroundHeight      , SIGNAL(valueChanged(int)), this, SLOT(GroundHeightChanged(int)));
  connect(sbNearForward       , SIGNAL(valueChanged(int)), this, SLOT(NearForwardChanged(int)));
  connect(sbNearForwardExStart, SIGNAL(valueChanged(int)), this, SLOT(NearForwardExStartChanged(int)));
  connect(sbNearForwardEx     , SIGNAL(valueChanged(int)), this, SLOT(NearForwardExChanged(int)));
  connect(sbNearBackward       , SIGNAL(valueChanged(int)), this, SLOT(NearForwardChanged(int)));
  connect(sbNearBackwardExStart, SIGNAL(valueChanged(int)), this, SLOT(NearForwardExStartChanged(int)));
  connect(sbNearBackwardEx     , SIGNAL(valueChanged(int)), this, SLOT(NearForwardExChanged(int)));

  connect(pbEditCenter,     &QPushButton::clicked, this, &CEditGeometryWidget::EditCSurface);
  connect(pbEditLShoulder,  &QPushButton::clicked, this, &CEditGeometryWidget::EditLShoulder);
  connect(pbEditRShoulder,  &QPushButton::clicked, this, &CEditGeometryWidget::EditRShoulder);
  connect(pbEditLWall,      &QPushButton::clicked, this, &CEditGeometryWidget::EditLWall);
  connect(pbEditRWall,      &QPushButton::clicked, this, &CEditGeometryWidget::EditRWall);
  connect(pbEditRoofTex,    &QPushButton::clicked, this, &CEditGeometryWidget::EditRoof);
  connect(pbEditLUOWall,    &QPushButton::clicked, this, &CEditGeometryWidget::EditLUOWall);
  connect(pbEditLLOWall,    &QPushButton::clicked, this, &CEditGeometryWidget::EditLLOWall);
  connect(pbEditRLOWall,    &QPushButton::clicked, this, &CEditGeometryWidget::EditRLOWall);
  connect(pbEditRUOWall,    &QPushButton::clicked, this, &CEditGeometryWidget::EditRUOWall);
  connect(pbEditOFloor,     &QPushButton::clicked, this, &CEditGeometryWidget::EditOFloor);
  connect(pbEditEnvirFloor, &QPushButton::clicked, this, &CEditGeometryWidget::EditEnvirFloor);

  connect(sldCGrip        , SIGNAL(valueChanged(int)), this, SLOT(CGripChanged(int)));
  connect(sldLShoulderGrip, SIGNAL(valueChanged(int)), this, SLOT(LGripChanged(int)));
  connect(sldRShoulderGrip, SIGNAL(valueChanged(int)), this, SLOT(RGripChanged(int)));
  connect(sldAISpeed,       SIGNAL(valueChanged(int)), this, SLOT(AISpeedChanged(int)));
}

//-------------------------------------------------------------------------------------------------

CEditGeometryWidget::~CEditGeometryWidget()
{

}

//-------------------------------------------------------------------------------------------------

void CEditGeometryWidget::UpdateGeometrySelection(int iFrom, int iTo)
{
  (void)(iTo);
  if (!g_pMainWindow->GetCurrentTrack() || iFrom >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size())
    return;
  BLOCK_SIG_AND_DO(dsbYaw, setValue(               g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].dYaw));
  BLOCK_SIG_AND_DO(dsbPitch, setValue(             g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].dPitch));
  BLOCK_SIG_AND_DO(dsbRoll, setValue(              g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].dRoll));
  BLOCK_SIG_AND_DO(sbLength, setValue(             g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iLength));
  BLOCK_SIG_AND_DO(sbLLaneWidth, setValue(         g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iLeftLaneWidth));
  BLOCK_SIG_AND_DO(sbRLaneWidth, setValue(         g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iRightLaneWidth));
  BLOCK_SIG_AND_DO(sbLShoulderWidth, setValue(     g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iLeftShoulderWidth));
  BLOCK_SIG_AND_DO(sbRShoulderWidth, setValue(     g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iRightShoulderWidth));
  BLOCK_SIG_AND_DO(sbLShoulderHeight, setValue(    g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iLeftShoulderHeight));
  BLOCK_SIG_AND_DO(sbRShoulderHeight, setValue(    g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iRightShoulderHeight));
  BLOCK_SIG_AND_DO(sbWallHeight, setValue(         g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iRoofHeight));
  BLOCK_SIG_AND_DO(sbLLOWallHOffset, setValue(     g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iLLOuterWallHOffset));
  BLOCK_SIG_AND_DO(sbLLOWallHeight, setValue(      g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iLLOuterWallHeight));
  BLOCK_SIG_AND_DO(sbLUOWallHOffset, setValue(     g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iLUOuterWallHOffset));
  BLOCK_SIG_AND_DO(sbLUOWallHeight, setValue(      g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iLUOuterWallHeight));
  BLOCK_SIG_AND_DO(sbRLOWallHOffset, setValue(     g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iRLOuterWallHOffset));
  BLOCK_SIG_AND_DO(sbRLOWallHeight, setValue(      g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iRLOuterWallHeight));
  BLOCK_SIG_AND_DO(sbRUOWallHOffset, setValue(     g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iRUOuterWallHOffset));
  BLOCK_SIG_AND_DO(sbRUOWallHeight, setValue(      g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iRUOuterWallHeight));
  BLOCK_SIG_AND_DO(sbLOuterFloorHOffset, setValue( g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iLOuterFloorHOffset));
  BLOCK_SIG_AND_DO(sbROuterFloorHOffset, setValue( g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iROuterFloorHOffset));
  BLOCK_SIG_AND_DO(sbLOuterFloorHeight, setValue(  g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iLOuterFloorHeight));
  BLOCK_SIG_AND_DO(sbROuterFloorHeight, setValue(  g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iROuterFloorHeight));
  BLOCK_SIG_AND_DO(sbAILine1, setValue(            g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iAILine1));
  BLOCK_SIG_AND_DO(sbAILine2, setValue(            g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iAILine2));
  BLOCK_SIG_AND_DO(sbAILine3, setValue(            g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iAILine3));
  BLOCK_SIG_AND_DO(sbAILine4, setValue(            g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iAILine4));
  BLOCK_SIG_AND_DO(sldCGrip, setValue(             g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iTrackGrip));
  BLOCK_SIG_AND_DO(sldLShoulderGrip, setValue(     g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iLeftShoulderGrip));
  BLOCK_SIG_AND_DO(sldRShoulderGrip, setValue(     g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iRightShoulderGrip);)
  BLOCK_SIG_AND_DO(sldAISpeed, setValue(           g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iAIMaxSpeed / 10));
  BLOCK_SIG_AND_DO(sbGroundHeight, setValue(       g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iGroundHeight));
  BLOCK_SIG_AND_DO(sbNearForward, setValue(        g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iNearForward));
  BLOCK_SIG_AND_DO(sbNearForwardExStart, setValue( g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iNearForwardExStart));
  BLOCK_SIG_AND_DO(sbNearForwardEx, setValue(      g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iNearForwardEx));
  BLOCK_SIG_AND_DO(sbNearBackward, setValue(       g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iNearBackward));
  BLOCK_SIG_AND_DO(sbNearBackwardExStart, setValue(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iNearBackwardExStart));
  BLOCK_SIG_AND_DO(sbNearBackwardEx, setValue(     g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iNearBackwardEx));

  lblCGrip->setText(        "(" + QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iTrackGrip) + ")");
  lblLShoulderGrip->setText("(" + QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iLeftShoulderGrip) + ")");
  lblRShoulderGrip->setText("(" + QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iRightShoulderGrip) + ")");
  lblAISpeed->setText(      "(" + QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iAIMaxSpeed / 10) + "%)");

  QtHelpers::UpdateTextures(lblCTex1, lblCTex2, g_pMainWindow->GetCurrentTrack()->m_pTex, g_pMainWindow->GetCurrentTrack()->m_pPal, g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iCenterSurfaceType);
  QtHelpers::UpdateTextures(lblLShoulderTex1, lblLShoulderTex2, g_pMainWindow->GetCurrentTrack()->m_pTex, g_pMainWindow->GetCurrentTrack()->m_pPal, g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iLeftSurfaceType);
  QtHelpers::UpdateTextures(lblRShoulderTex1, lblRShoulderTex2, g_pMainWindow->GetCurrentTrack()->m_pTex, g_pMainWindow->GetCurrentTrack()->m_pPal, g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iRightSurfaceType);
  QtHelpers::UpdateTextures(lblLWallTex1, lblLWallTex2, g_pMainWindow->GetCurrentTrack()->m_pTex, g_pMainWindow->GetCurrentTrack()->m_pPal, g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iLeftWallType);
  QtHelpers::UpdateTextures(lblRWallTex1, lblRWallTex2, g_pMainWindow->GetCurrentTrack()->m_pTex, g_pMainWindow->GetCurrentTrack()->m_pPal, g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iRightWallType);
  QtHelpers::UpdateTextures(lblRoofTex1, lblRoofTex2, g_pMainWindow->GetCurrentTrack()->m_pTex, g_pMainWindow->GetCurrentTrack()->m_pPal, g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iRoofType);
  QtHelpers::UpdateTextures(lblLLOWallTex1, lblLLOWallTex2, g_pMainWindow->GetCurrentTrack()->m_pTex, g_pMainWindow->GetCurrentTrack()->m_pPal, g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iLLOuterWallType);
  QtHelpers::UpdateTextures(lblLUOWallTex1, lblLUOWallTex2, g_pMainWindow->GetCurrentTrack()->m_pTex, g_pMainWindow->GetCurrentTrack()->m_pPal, g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iLUOuterWallType);
  QtHelpers::UpdateTextures(lblRLOWallTex1, lblRLOWallTex2, g_pMainWindow->GetCurrentTrack()->m_pTex, g_pMainWindow->GetCurrentTrack()->m_pPal, g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iRLOuterWallType);
  QtHelpers::UpdateTextures(lblRUOWallTex1, lblRUOWallTex2, g_pMainWindow->GetCurrentTrack()->m_pTex, g_pMainWindow->GetCurrentTrack()->m_pPal, g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iRUOuterWallType);
  QtHelpers::UpdateTextures(lblOFloorTex1, lblOFloorTex2, g_pMainWindow->GetCurrentTrack()->m_pTex, g_pMainWindow->GetCurrentTrack()->m_pPal, g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iOuterFloorType);
  QtHelpers::UpdateTextures(lblEnvirFloorTex, NULL, NULL, g_pMainWindow->GetCurrentTrack()->m_pPal, g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iEnvironmentFloorType);

  //update roof disabled text
  lblRoofDisabled->setVisible(!g_pMainWindow->GetCurrentTrack()->ShouldShowChunkSection(iFrom, eShapeSection::ROOF) && CTrack::ShouldDrawSurfaceType(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iRoofType));

  //update llowall disabled text
  lblLLOWallDisabled->setVisible(!g_pMainWindow->GetCurrentTrack()->ShouldShowChunkSection(iFrom, eShapeSection::LLOWALL) && CTrack::ShouldDrawSurfaceType(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iLLOuterWallType));
  if (g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iOuterFloorType == -1)
    lblLLOWallDisabled->setText("(disabled by floor)");
  else if (g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iOuterFloorType == -2)
    lblLLOWallDisabled->setText("(disabled by center)");
  else
    lblLLOWallDisabled->setText("(disabled by other)");

  //update rlowall disabled text
  lblRLOWallDisabled->setVisible(!g_pMainWindow->GetCurrentTrack()->ShouldShowChunkSection(iFrom, eShapeSection::RLOWALL) && CTrack::ShouldDrawSurfaceType(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iRLOuterWallType));
  if (g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iOuterFloorType == -1)
    lblRLOWallDisabled->setText("(disabled by floor)");
  else if (g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iOuterFloorType == -2)
    lblRLOWallDisabled->setText("(disabled by center)");
  else
    lblRLOWallDisabled->setText("(disabled by other)");

  //update luowall disabled text
  lblLUOWallDisabled->setVisible(!g_pMainWindow->GetCurrentTrack()->ShouldShowChunkSection(iFrom, eShapeSection::LUOWALL) && CTrack::ShouldDrawSurfaceType(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iLUOuterWallType));
  if (g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iLLOuterWallType == -1)
    lblLUOWallDisabled->setText("(disabled by lower wall)");
  else if (g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iOuterFloorType == -1)
    lblLUOWallDisabled->setText("(disabled by floor)");
  else if (g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iOuterFloorType == -2)
    lblLUOWallDisabled->setText("(disabled by center)");
  else
    lblLUOWallDisabled->setText("(disabled by other)");

  //update ruowall disabled text
  lblRUOWallDisabled->setVisible(!g_pMainWindow->GetCurrentTrack()->ShouldShowChunkSection(iFrom, eShapeSection::RUOWALL) && CTrack::ShouldDrawSurfaceType(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iRUOuterWallType));
  if (g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iRLOuterWallType == -1)
    lblRUOWallDisabled->setText("(disabled by lower wall)");
  else if (g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iOuterFloorType == -1)
    lblRUOWallDisabled->setText("(disabled by floor)");
  else if (g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iOuterFloorType == -2)
    lblRUOWallDisabled->setText("(disabled by center)");
  else
    lblRUOWallDisabled->setText("(disabled by other)");
}

//-------------------------------------------------------------------------------------------------

void CEditGeometryWidget::YawChanged(double dValue)
{
  int iFrom = g_pMainWindow->GetSelFrom();
  int iTo = g_pMainWindow->GetSelTo();

  if (!g_pMainWindow->GetCurrentTrack()
      || iFrom >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size()
      || iTo >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size())
    return;

  for (int i = iFrom; i <= iTo; ++i) {
    g_pMainWindow->GetCurrentTrack()->m_chunkAy[i].dYaw = dValue;
  }

  g_pMainWindow->SaveHistory("Changed yaw");
  g_pMainWindow->UpdateWindow();
}

//-------------------------------------------------------------------------------------------------

void CEditGeometryWidget::PitchChanged(double dValue)
{
  int iFrom = g_pMainWindow->GetSelFrom();
  int iTo = g_pMainWindow->GetSelTo();

  if (!g_pMainWindow->GetCurrentTrack()
      || iFrom >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size()
      || iTo >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size())
    return;

  for (int i = iFrom; i <= iTo; ++i) {
    g_pMainWindow->GetCurrentTrack()->m_chunkAy[i].dPitch = dValue;
  }

  g_pMainWindow->SaveHistory("Changed pitch");
  g_pMainWindow->UpdateWindow();
}

//-------------------------------------------------------------------------------------------------

void CEditGeometryWidget::RollChanged(double dValue)
{
  int iFrom = g_pMainWindow->GetSelFrom();
  int iTo = g_pMainWindow->GetSelTo();

  if (!g_pMainWindow->GetCurrentTrack()
      || iFrom >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size()
      || iTo >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size())
    return;

  for (int i = iFrom; i <= iTo; ++i) {
    g_pMainWindow->GetCurrentTrack()->m_chunkAy[i].dRoll = dValue;
  }

  g_pMainWindow->SaveHistory("Changed roll");
  g_pMainWindow->UpdateWindow();
}

//-------------------------------------------------------------------------------------------------

void CEditGeometryWidget::LengthChanged(int iValue)
{
  int iFrom = g_pMainWindow->GetSelFrom();
  int iTo = g_pMainWindow->GetSelTo();

  if (!g_pMainWindow->GetCurrentTrack()
      || iFrom >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size()
      || iTo >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size())
    return;

  for (int i = iFrom; i <= iTo; ++i) {
    g_pMainWindow->GetCurrentTrack()->m_chunkAy[i].iLength = iValue;
  }

  g_pMainWindow->SaveHistory("Changed length");
  g_pMainWindow->UpdateWindow();
}

//-------------------------------------------------------------------------------------------------

void CEditGeometryWidget::LLaneWidthChanged(int iValue)
{
  int iFrom = g_pMainWindow->GetSelFrom();
  int iTo = g_pMainWindow->GetSelTo();

  if (!g_pMainWindow->GetCurrentTrack()
      || iFrom >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size()
      || iTo >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size())
    return;

  for (int i = iFrom; i <= iTo; ++i) {
    g_pMainWindow->GetCurrentTrack()->m_chunkAy[i].iLeftLaneWidth = iValue;
  }

  g_pMainWindow->SaveHistory("Changed left lane width");
  g_pMainWindow->UpdateWindow();
}

//-------------------------------------------------------------------------------------------------

void CEditGeometryWidget::RLaneWidthChanged(int iValue)
{
  int iFrom = g_pMainWindow->GetSelFrom();
  int iTo = g_pMainWindow->GetSelTo();

  if (!g_pMainWindow->GetCurrentTrack()
      || iFrom >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size()
      || iTo >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size())
    return;

  for (int i = iFrom; i <= iTo; ++i) {
    g_pMainWindow->GetCurrentTrack()->m_chunkAy[i].iRightLaneWidth = iValue;
  }

  g_pMainWindow->SaveHistory("Changed right lane width");
  g_pMainWindow->UpdateWindow();
}

//-------------------------------------------------------------------------------------------------

void CEditGeometryWidget::LShoulderWidthChanged(int iValue)
{
  int iFrom = g_pMainWindow->GetSelFrom();
  int iTo = g_pMainWindow->GetSelTo();

  if (!g_pMainWindow->GetCurrentTrack()
      || iFrom >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size()
      || iTo >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size())
    return;

  for (int i = iFrom; i <= iTo; ++i) {
    g_pMainWindow->GetCurrentTrack()->m_chunkAy[i].iLeftShoulderWidth = iValue;
  }

  g_pMainWindow->SaveHistory("Changed left shoulder width");
  g_pMainWindow->UpdateWindow();
}

//-------------------------------------------------------------------------------------------------

void CEditGeometryWidget::RShoulderWidthChanged(int iValue)
{
  int iFrom = g_pMainWindow->GetSelFrom();
  int iTo = g_pMainWindow->GetSelTo();

  if (!g_pMainWindow->GetCurrentTrack()
      || iFrom >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size()
      || iTo >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size())
    return;

  for (int i = iFrom; i <= iTo; ++i) {
    g_pMainWindow->GetCurrentTrack()->m_chunkAy[i].iRightShoulderWidth = iValue;
  }

  g_pMainWindow->SaveHistory("Changed right shoulder width");
  g_pMainWindow->UpdateWindow();
}

//-------------------------------------------------------------------------------------------------

void CEditGeometryWidget::LShoulderHeightChanged(int iValue)
{
  int iFrom = g_pMainWindow->GetSelFrom();
  int iTo = g_pMainWindow->GetSelTo();

  if (!g_pMainWindow->GetCurrentTrack()
      || iFrom >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size()
      || iTo >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size())
    return;

  for (int i = iFrom; i <= iTo; ++i) {
    g_pMainWindow->GetCurrentTrack()->m_chunkAy[i].iLeftShoulderHeight = iValue;
  }

  g_pMainWindow->SaveHistory("Changed left shoulder height");
  g_pMainWindow->UpdateWindow();
}

//-------------------------------------------------------------------------------------------------

void CEditGeometryWidget::RShoulderHeightChanged(int iValue)
{
  int iFrom = g_pMainWindow->GetSelFrom();
  int iTo = g_pMainWindow->GetSelTo();

  if (!g_pMainWindow->GetCurrentTrack()
      || iFrom >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size()
      || iTo >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size())
    return;

  for (int i = iFrom; i <= iTo; ++i) {
    g_pMainWindow->GetCurrentTrack()->m_chunkAy[i].iRightShoulderHeight = iValue;
  }

  g_pMainWindow->SaveHistory("Changed right shoulder height");
  g_pMainWindow->UpdateWindow();
}

//-------------------------------------------------------------------------------------------------

void CEditGeometryWidget::WallHeightChanged(int iValue)
{
  int iFrom = g_pMainWindow->GetSelFrom();
  int iTo = g_pMainWindow->GetSelTo();

  if (!g_pMainWindow->GetCurrentTrack()
      || iFrom >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size()
      || iTo >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size())
    return;

  for (int i = iFrom; i <= iTo; ++i) {
    g_pMainWindow->GetCurrentTrack()->m_chunkAy[i].iRoofHeight = iValue;
  }

  g_pMainWindow->SaveHistory("Changed wall height");
  g_pMainWindow->UpdateWindow();
}

//-------------------------------------------------------------------------------------------------

void CEditGeometryWidget::LLOWallHOffsetChanged(int iValue)
{
  int iFrom = g_pMainWindow->GetSelFrom();
  int iTo = g_pMainWindow->GetSelTo();

  if (!g_pMainWindow->GetCurrentTrack()
      || iFrom >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size()
      || iTo >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size())
    return;

  for (int i = iFrom; i <= iTo; ++i) {
    g_pMainWindow->GetCurrentTrack()->m_chunkAy[i].iLLOuterWallHOffset = iValue;
  }

  g_pMainWindow->SaveHistory("Changed llowall width");
  g_pMainWindow->UpdateWindow();
}

//-------------------------------------------------------------------------------------------------

void CEditGeometryWidget::LLOWallHeightChanged(int iValue)
{
  int iFrom = g_pMainWindow->GetSelFrom();
  int iTo = g_pMainWindow->GetSelTo();

  if (!g_pMainWindow->GetCurrentTrack()
      || iFrom >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size()
      || iTo >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size())
    return;

  for (int i = iFrom; i <= iTo; ++i) {
    g_pMainWindow->GetCurrentTrack()->m_chunkAy[i].iLLOuterWallHeight = iValue;
  }

  g_pMainWindow->SaveHistory("Changed llowall height");
  g_pMainWindow->UpdateWindow();
}

//-------------------------------------------------------------------------------------------------

void CEditGeometryWidget::LUOWallHOffsetChanged(int iValue)
{
  int iFrom = g_pMainWindow->GetSelFrom();
  int iTo = g_pMainWindow->GetSelTo();

  if (!g_pMainWindow->GetCurrentTrack()
      || iFrom >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size()
      || iTo >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size())
    return;

  for (int i = iFrom; i <= iTo; ++i) {
    g_pMainWindow->GetCurrentTrack()->m_chunkAy[i].iLUOuterWallHOffset = iValue;
  }

  g_pMainWindow->SaveHistory("Changed luowall width");
  g_pMainWindow->UpdateWindow();
}

//-------------------------------------------------------------------------------------------------

void CEditGeometryWidget::LUOWallHeightChanged(int iValue)
{
  int iFrom = g_pMainWindow->GetSelFrom();
  int iTo = g_pMainWindow->GetSelTo();

  if (!g_pMainWindow->GetCurrentTrack()
      || iFrom >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size()
      || iTo >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size())
    return;

  for (int i = iFrom; i <= iTo; ++i) {
    g_pMainWindow->GetCurrentTrack()->m_chunkAy[i].iLUOuterWallHeight = iValue;
  }

  g_pMainWindow->SaveHistory("Changed luowall height");
  g_pMainWindow->UpdateWindow();
}

//-------------------------------------------------------------------------------------------------

void CEditGeometryWidget::RLOWallHOffsetChanged(int iValue)
{
  int iFrom = g_pMainWindow->GetSelFrom();
  int iTo = g_pMainWindow->GetSelTo();

  if (!g_pMainWindow->GetCurrentTrack()
      || iFrom >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size()
      || iTo >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size())
    return;

  for (int i = iFrom; i <= iTo; ++i) {
    g_pMainWindow->GetCurrentTrack()->m_chunkAy[i].iRLOuterWallHOffset = iValue;
  }

  g_pMainWindow->SaveHistory("Changed rlowall width");
  g_pMainWindow->UpdateWindow();
}

//-------------------------------------------------------------------------------------------------

void CEditGeometryWidget::RLOWallHeightChanged(int iValue)
{
  int iFrom = g_pMainWindow->GetSelFrom();
  int iTo = g_pMainWindow->GetSelTo();

  if (!g_pMainWindow->GetCurrentTrack()
      || iFrom >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size()
      || iTo >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size())
    return;

  for (int i = iFrom; i <= iTo; ++i) {
    g_pMainWindow->GetCurrentTrack()->m_chunkAy[i].iRLOuterWallHeight = iValue;
  }

  g_pMainWindow->SaveHistory("Changed rlowall height");
  g_pMainWindow->UpdateWindow();
}

//-------------------------------------------------------------------------------------------------

void CEditGeometryWidget::RUOWallHOffsetChanged(int iValue)
{
  int iFrom = g_pMainWindow->GetSelFrom();
  int iTo = g_pMainWindow->GetSelTo();

  if (!g_pMainWindow->GetCurrentTrack()
      || iFrom >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size()
      || iTo >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size())
    return;

  for (int i = iFrom; i <= iTo; ++i) {
    g_pMainWindow->GetCurrentTrack()->m_chunkAy[i].iRUOuterWallHOffset = iValue;
  }

  g_pMainWindow->SaveHistory("Changed ruowall width");
  g_pMainWindow->UpdateWindow();
}

//-------------------------------------------------------------------------------------------------

void CEditGeometryWidget::RUOWallHeightChanged(int iValue)
{
  int iFrom = g_pMainWindow->GetSelFrom();
  int iTo = g_pMainWindow->GetSelTo();

  if (!g_pMainWindow->GetCurrentTrack()
      || iFrom >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size()
      || iTo >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size())
    return;

  for (int i = iFrom; i <= iTo; ++i) {
    g_pMainWindow->GetCurrentTrack()->m_chunkAy[i].iRUOuterWallHeight = iValue;
  }

  g_pMainWindow->SaveHistory("Changed ruowall height");
  g_pMainWindow->UpdateWindow();
}

//-------------------------------------------------------------------------------------------------

void CEditGeometryWidget::LOuterFloorHOffsetChanged(int iValue)
{
  int iFrom = g_pMainWindow->GetSelFrom();
  int iTo = g_pMainWindow->GetSelTo();

  if (!g_pMainWindow->GetCurrentTrack()
      || iFrom >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size()
      || iTo >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size())
    return;

  for (int i = iFrom; i <= iTo; ++i) {
    g_pMainWindow->GetCurrentTrack()->m_chunkAy[i].iLOuterFloorHOffset = iValue;
  }

  g_pMainWindow->SaveHistory("Changed lofloor horiz offset");
  g_pMainWindow->UpdateWindow();
}

//-------------------------------------------------------------------------------------------------

void CEditGeometryWidget::ROuterFloorHOffsetChanged(int iValue)
{
  int iFrom = g_pMainWindow->GetSelFrom();
  int iTo = g_pMainWindow->GetSelTo();

  if (!g_pMainWindow->GetCurrentTrack()
      || iFrom >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size()
      || iTo >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size())
    return;

  for (int i = iFrom; i <= iTo; ++i) {
    g_pMainWindow->GetCurrentTrack()->m_chunkAy[i].iROuterFloorHOffset = iValue;
  }

  g_pMainWindow->SaveHistory("Changed rofloor horiz offset");
  g_pMainWindow->UpdateWindow();
}

//-------------------------------------------------------------------------------------------------

void CEditGeometryWidget::LOuterFloorHeightChanged(int iValue)
{
  int iFrom = g_pMainWindow->GetSelFrom();
  int iTo = g_pMainWindow->GetSelTo();

  if (!g_pMainWindow->GetCurrentTrack()
      || iFrom >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size()
      || iTo >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size())
    return;

  for (int i = iFrom; i <= iTo; ++i) {
    g_pMainWindow->GetCurrentTrack()->m_chunkAy[i].iLOuterFloorHeight = iValue;
  }

  g_pMainWindow->SaveHistory("Changed lofloor height");
  g_pMainWindow->UpdateWindow();
}

//-------------------------------------------------------------------------------------------------

void CEditGeometryWidget::ROuterFloorHeightChanged(int iValue)
{
  int iFrom = g_pMainWindow->GetSelFrom();
  int iTo = g_pMainWindow->GetSelTo();

  if (!g_pMainWindow->GetCurrentTrack()
      || iFrom >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size()
      || iTo >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size())
    return;

  for (int i = iFrom; i <= iTo; ++i) {
    g_pMainWindow->GetCurrentTrack()->m_chunkAy[i].iROuterFloorHeight = iValue;
  }

  g_pMainWindow->SaveHistory("Changed rofloor height");
  g_pMainWindow->UpdateWindow();
}

//-------------------------------------------------------------------------------------------------

void CEditGeometryWidget::AILine1Changed(int iValue)
{
  int iFrom = g_pMainWindow->GetSelFrom();
  int iTo = g_pMainWindow->GetSelTo();

  if (!g_pMainWindow->GetCurrentTrack()
      || iFrom >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size()
      || iTo >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size())
    return;

  for (int i = iFrom; i <= iTo; ++i) {
    g_pMainWindow->GetCurrentTrack()->m_chunkAy[i].iAILine1 = iValue;
  }

  g_pMainWindow->SaveHistory("Changed ai line 1");
  g_pMainWindow->UpdateWindow();
}

//-------------------------------------------------------------------------------------------------

void CEditGeometryWidget::AILine2Changed(int iValue)
{
  int iFrom = g_pMainWindow->GetSelFrom();
  int iTo = g_pMainWindow->GetSelTo();

  if (!g_pMainWindow->GetCurrentTrack()
      || iFrom >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size()
      || iTo >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size())
    return;

  for (int i = iFrom; i <= iTo; ++i) {
    g_pMainWindow->GetCurrentTrack()->m_chunkAy[i].iAILine2 = iValue;
  }

  g_pMainWindow->SaveHistory("Changed ai line 2");
  g_pMainWindow->UpdateWindow();
}

//-------------------------------------------------------------------------------------------------

void CEditGeometryWidget::AILine3Changed(int iValue)
{
  int iFrom = g_pMainWindow->GetSelFrom();
  int iTo = g_pMainWindow->GetSelTo();

  if (!g_pMainWindow->GetCurrentTrack()
      || iFrom >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size()
      || iTo >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size())
    return;

  for (int i = iFrom; i <= iTo; ++i) {
    g_pMainWindow->GetCurrentTrack()->m_chunkAy[i].iAILine3 = iValue;
  }

  g_pMainWindow->SaveHistory("Changed ai line 3");
  g_pMainWindow->UpdateWindow();
}

//-------------------------------------------------------------------------------------------------

void CEditGeometryWidget::AILine4Changed(int iValue)
{
  int iFrom = g_pMainWindow->GetSelFrom();
  int iTo = g_pMainWindow->GetSelTo();

  if (!g_pMainWindow->GetCurrentTrack()
      || iFrom >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size()
      || iTo >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size())
    return;

  for (int i = iFrom; i <= iTo; ++i) {
    g_pMainWindow->GetCurrentTrack()->m_chunkAy[i].iAILine4 = iValue;
  }

  g_pMainWindow->SaveHistory("Changed ai line 4");
  g_pMainWindow->UpdateWindow();
}

//-------------------------------------------------------------------------------------------------

void CEditGeometryWidget::EditCSurface()
{
  CEditSurfaceDialog dlg(this, eSurfaceField::SURFACE_CENTER, true, "(also disables outer walls if floor is -2)");
  dlg.exec();
}

//-------------------------------------------------------------------------------------------------

void CEditGeometryWidget::EditLShoulder()
{
  CEditSurfaceDialog dlg(this, eSurfaceField::SURFACE_LSHOULDER, true, "(also disables outer walls if floor is -2)");
  dlg.exec();
}

//-------------------------------------------------------------------------------------------------

void CEditGeometryWidget::EditRShoulder()
{
  CEditSurfaceDialog dlg(this, eSurfaceField::SURFACE_RSHOULDER, true, "(also disables outer walls if floor is -2)");
  dlg.exec();
}

//-------------------------------------------------------------------------------------------------

void CEditGeometryWidget::EditLWall()
{
  CEditSurfaceDialog dlg(this, eSurfaceField::SURFACE_LWALL, true, "(also disables roof)");
  dlg.exec();
}

//-------------------------------------------------------------------------------------------------

void CEditGeometryWidget::EditRWall()
{
  CEditSurfaceDialog dlg(this, eSurfaceField::SURFACE_RWALL, true, "(also disables roof)");
  dlg.exec();
}

//-------------------------------------------------------------------------------------------------

void CEditGeometryWidget::EditRoof()
{
  CEditSurfaceDialog dlg(this, eSurfaceField::SURFACE_ROOF, true);
  dlg.exec();
}

//-------------------------------------------------------------------------------------------------

void CEditGeometryWidget::EditLUOWall()
{
  CEditSurfaceDialog dlg(this, eSurfaceField::SURFACE_LUOWALL, true);
  dlg.exec();
}

//-------------------------------------------------------------------------------------------------

void CEditGeometryWidget::EditLLOWall()
{
  CEditSurfaceDialog dlg(this, eSurfaceField::SURFACE_LLOWALL, true, "(also disables upper wall)");
  dlg.exec();
}

//-------------------------------------------------------------------------------------------------

void CEditGeometryWidget::EditRLOWall()
{
  CEditSurfaceDialog dlg(this, eSurfaceField::SURFACE_RLOWALL, true, "(also disables upper wall)");
  dlg.exec();
}

//-------------------------------------------------------------------------------------------------

void CEditGeometryWidget::EditRUOWall()
{
  CEditSurfaceDialog dlg(this, eSurfaceField::SURFACE_RUOWALL, true);
  dlg.exec();
}

//-------------------------------------------------------------------------------------------------

void CEditGeometryWidget::EditOFloor()
{
  CEditSurfaceDialog dlg(this, eSurfaceField::SURFACE_OFLOOR, true, "(also disables outer walls)", true);
  dlg.exec();
}

//-------------------------------------------------------------------------------------------------

void CEditGeometryWidget::EditEnvirFloor()
{
  CEditSurfaceDialog dlg(this, eSurfaceField::SURFACE_ENVFLOOR);
  dlg.exec();
}

//-------------------------------------------------------------------------------------------------

void CEditGeometryWidget::CGripChanged(int iValue)
{
  int iFrom = g_pMainWindow->GetSelFrom();
  int iTo = g_pMainWindow->GetSelTo();

  if (!g_pMainWindow->GetCurrentTrack()
      || iFrom >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size()
      || iTo >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size())
    return;

  for (int i = iFrom; i <= iTo; ++i) {
    g_pMainWindow->GetCurrentTrack()->m_chunkAy[i].iTrackGrip = iValue;
  }
  g_pMainWindow->SaveHistory("Changed center grip");
  g_pMainWindow->UpdateWindow();
}

//-------------------------------------------------------------------------------------------------

void CEditGeometryWidget::LGripChanged(int iValue)
{
  int iFrom = g_pMainWindow->GetSelFrom();
  int iTo = g_pMainWindow->GetSelTo();

  if (!g_pMainWindow->GetCurrentTrack()
      || iFrom >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size()
      || iTo >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size())
    return;

  for (int i = iFrom; i <= iTo; ++i) {
    g_pMainWindow->GetCurrentTrack()->m_chunkAy[i].iLeftShoulderGrip = iValue;
  }
  g_pMainWindow->SaveHistory("Changed left shoulder grip");
  g_pMainWindow->UpdateWindow();
}

//-------------------------------------------------------------------------------------------------

void CEditGeometryWidget::RGripChanged(int iValue)
{
  int iFrom = g_pMainWindow->GetSelFrom();
  int iTo = g_pMainWindow->GetSelTo();

  if (!g_pMainWindow->GetCurrentTrack()
      || iFrom >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size()
      || iTo >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size())
    return;

  for (int i = iFrom; i <= iTo; ++i) {
    g_pMainWindow->GetCurrentTrack()->m_chunkAy[i].iRightShoulderGrip = iValue;
  }
  g_pMainWindow->SaveHistory("Changed right shoulder grip");
  g_pMainWindow->UpdateWindow();
}

//-------------------------------------------------------------------------------------------------

void CEditGeometryWidget::AISpeedChanged(int iValue)
{
  int iFrom = g_pMainWindow->GetSelFrom();
  int iTo = g_pMainWindow->GetSelTo();

  if (!g_pMainWindow->GetCurrentTrack()
      || iFrom >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size()
      || iTo >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size())
    return;

  for (int i = iFrom; i <= iTo; ++i) {
    g_pMainWindow->GetCurrentTrack()->m_chunkAy[i].iAIMaxSpeed = iValue * 10;
  }
  g_pMainWindow->SaveHistory("Changed ai speed");
  g_pMainWindow->UpdateWindow();
}

//-------------------------------------------------------------------------------------------------

void CEditGeometryWidget::GroundHeightChanged(int iValue)
{
  int iFrom = g_pMainWindow->GetSelFrom();
  int iTo = g_pMainWindow->GetSelTo();

  if (!g_pMainWindow->GetCurrentTrack()
      || iFrom >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size()
      || iTo >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size())
    return;

  for (int i = iFrom; i <= iTo; ++i) {
    g_pMainWindow->GetCurrentTrack()->m_chunkAy[i].iGroundHeight = iValue;
  }
  g_pMainWindow->SaveHistory("Changed ground height");
  g_pMainWindow->UpdateWindow();
}

//-------------------------------------------------------------------------------------------------

void CEditGeometryWidget::NearForwardChanged(int iValue)
{
  int iFrom = g_pMainWindow->GetSelFrom();
  int iTo = g_pMainWindow->GetSelTo();

  if (!g_pMainWindow->GetCurrentTrack()
      || iFrom >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size()
      || iTo >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size())
    return;

  for (int i = iFrom; i <= iTo; ++i) {
    g_pMainWindow->GetCurrentTrack()->m_chunkAy[i].iNearForward = iValue;
  }
  g_pMainWindow->SaveHistory("Changed forward main chunks");
  g_pMainWindow->UpdateWindow();
}

//-------------------------------------------------------------------------------------------------

void CEditGeometryWidget::NearForwardExStartChanged(int iValue)
{
  int iFrom = g_pMainWindow->GetSelFrom();
  int iTo = g_pMainWindow->GetSelTo();

  if (!g_pMainWindow->GetCurrentTrack()
      || iFrom >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size()
      || iTo >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size())
    return;

  for (int i = iFrom; i <= iTo; ++i) {
    g_pMainWindow->GetCurrentTrack()->m_chunkAy[i].iNearForwardExStart = iValue;
  }
  g_pMainWindow->SaveHistory("Changed forward extra chunks start");
  g_pMainWindow->UpdateWindow();
}

//-------------------------------------------------------------------------------------------------

void CEditGeometryWidget::NearForwardExChanged(int iValue)
{
  int iFrom = g_pMainWindow->GetSelFrom();
  int iTo = g_pMainWindow->GetSelTo();

  if (!g_pMainWindow->GetCurrentTrack()
      || iFrom >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size()
      || iTo >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size())
    return;

  for (int i = iFrom; i <= iTo; ++i) {
    g_pMainWindow->GetCurrentTrack()->m_chunkAy[i].iNearForwardEx = iValue;
  }
  g_pMainWindow->SaveHistory("Changed forward extra chunks");
  g_pMainWindow->UpdateWindow();
}

//-------------------------------------------------------------------------------------------------

void CEditGeometryWidget::NearBackwardChanged(int iValue)
{
  int iFrom = g_pMainWindow->GetSelFrom();
  int iTo = g_pMainWindow->GetSelTo();

  if (!g_pMainWindow->GetCurrentTrack()
      || iFrom >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size()
      || iTo >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size())
    return;

  for (int i = iFrom; i <= iTo; ++i) {
    g_pMainWindow->GetCurrentTrack()->m_chunkAy[i].iNearBackward = iValue;
  }
  g_pMainWindow->SaveHistory("Changed backward main chunks");
  g_pMainWindow->UpdateWindow();
}

//-------------------------------------------------------------------------------------------------

void CEditGeometryWidget::NearBackwardExStartChanged(int iValue)
{
  int iFrom = g_pMainWindow->GetSelFrom();
  int iTo = g_pMainWindow->GetSelTo();

  if (!g_pMainWindow->GetCurrentTrack()
      || iFrom >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size()
      || iTo >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size())
    return;

  for (int i = iFrom; i <= iTo; ++i) {
    g_pMainWindow->GetCurrentTrack()->m_chunkAy[i].iNearBackwardExStart = iValue;
  }
  g_pMainWindow->SaveHistory("Changed backward extra chunks start");
  g_pMainWindow->UpdateWindow();
}

//-------------------------------------------------------------------------------------------------

void CEditGeometryWidget::NearBackwardExChanged(int iValue)
{
  int iFrom = g_pMainWindow->GetSelFrom();
  int iTo = g_pMainWindow->GetSelTo();

  if (!g_pMainWindow->GetCurrentTrack()
      || iFrom >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size()
      || iTo >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size())
    return;

  for (int i = iFrom; i <= iTo; ++i) {
    g_pMainWindow->GetCurrentTrack()->m_chunkAy[i].iNearBackwardEx = iValue;
  }
  g_pMainWindow->SaveHistory("Changed backward extra chunks");
  g_pMainWindow->UpdateWindow();
}

//-------------------------------------------------------------------------------------------------