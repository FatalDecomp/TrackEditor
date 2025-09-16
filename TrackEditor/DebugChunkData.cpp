#include "DebugChunkData.h"
#include "Track.h"
#include "MainWindow.h"
#include "EditSurfaceDialog.h"
#include "TilePicker.h"
#include "Texture.h"
#include "Palette.h"
#include "QtHelpers.h"
//-------------------------------------------------------------------------------------------------
#if defined(_DEBUG) && defined(IS_WINDOWS)
#define new new(_CLIENT_BLOCK, __FILE__, __LINE__)
#endif
//-------------------------------------------------------------------------------------------------

CDebugChunkData::CDebugChunkData(QWidget *pParent)
  : QWidget(pParent)
{
  setupUi(this);

  connect(g_pMainWindow, &CMainWindow::UpdateGeometrySelectionSig, this, &CDebugChunkData::UpdateGeometrySelection);
}

//-------------------------------------------------------------------------------------------------

CDebugChunkData::~CDebugChunkData()
{
}

//-------------------------------------------------------------------------------------------------

void CDebugChunkData::UpdateGeometrySelection(int iFrom, int iTo)
{
  if (!g_pMainWindow->GetCurrentTrack()
      || iFrom >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size()
      || iTo >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size())
    return;

  leLShoulderWidth       ->setText(QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iLeftShoulderWidth));
  leLLaneWidth           ->setText(QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iLeftLaneWidth));
  leRLaneWidth           ->setText(QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iRightLaneWidth));
  leRShoulderWidth       ->setText(QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iRightShoulderWidth));
  leLShoulderHeight      ->setText(QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iLeftShoulderHeight));
  leRShoulderHeight      ->setText(QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iRightShoulderHeight));
  leLength               ->setText(QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iLength));
  leYaw                  ->setText(QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].dYaw));
  lePitch                ->setText(QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].dPitch));
  leRoll                 ->setText(QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].dRoll));
  leAILine1              ->setText(QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iAILine1));
  leAILine2              ->setText(QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iAILine2));
  leAILine3              ->setText(QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iAILine3));
  leAILine4              ->setText(QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iAILine4));
  leTrackGrip            ->setText(QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iTrackGrip));
  leLeftShoulderGrip     ->setText(QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iLeftShoulderGrip));
  leRShoulderGrip        ->setText(QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iRightShoulderGrip));
  leAIMaxSpeed           ->setText(QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iAIMaxSpeed));
  leGroundHeight         ->setText(QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iGroundHeight));
  leAudioAboveTrigger    ->setText(QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iAudioAboveTrigger));
  leAudioTriggerSpeed    ->setText(QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iAudioTriggerSpeed));
  leAudioBelowTrigger    ->setText(QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iAudioBelowTrigger));
  leLeftSurfaceType      ->setText(QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iLeftSurfaceType));
  leCenterSurfaceType    ->setText(QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iCenterSurfaceType));
  leRightSurfaceType     ->setText(QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iRightSurfaceType));
  leLWallType            ->setText(QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iLeftWallType));
  leRWallType            ->setText(QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iRightWallType));
  leRoofType             ->setText(QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iRoofType));
  leLUOuterWallType      ->setText(QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iLUOuterWallType));
  leLLOuterWallType      ->setText(QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iLLOuterWallType));
  leOuterFloorType       ->setText(QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iOuterFloorType));
  leRLOuterWallType      ->setText(QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iRLOuterWallType));
  leRUOuterWallType      ->setText(QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iRUOuterWallType));
  leEnvironmentFloorType ->setText(QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iEnvironmentFloorType));
  leSignType             ->setText(QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iSignType));
  leSignHorizOffset      ->setText(QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iSignHorizOffset));
  leSignVertOffset       ->setText(QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iSignVertOffset));
  leSignYaw              ->setText(QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].dSignYaw));
  leSignPitch            ->setText(QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].dSignPitch));
  leSignRoll             ->setText(QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].dSignRoll));
  leLUOuterWallHOffset   ->setText(QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iLUOuterWallHOffset));
  leLLOuterWallHOffset   ->setText(QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iLLOuterWallHOffset));
  leLOuterFloorHOffset   ->setText(QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iLOuterFloorHOffset));
  leROuterFloorHOffset   ->setText(QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iROuterFloorHOffset));
  leRLOuterWallHOffset   ->setText(QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iRLOuterWallHOffset));
  leRUOuterWallHOffset   ->setText(QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iRUOuterWallHOffset));
  leLUOuterWallHeight    ->setText(QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iLUOuterWallHeight));
  leLLOuterWallHeight    ->setText(QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iLLOuterWallHeight));
  leLOuterFloorHeight    ->setText(QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iLOuterFloorHeight));
  leROuterFloorHeight    ->setText(QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iROuterFloorHeight));
  leRLOuterWallHeight    ->setText(QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iRLOuterWallHeight));
  leRUOuterWallHeight    ->setText(QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iRUOuterWallHeight));
  leRoofHeight           ->setText(QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iRoofHeight));
  leNearForward          ->setText(QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iNearForward));
  leNearForwardExStart   ->setText(QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iNearForwardExStart));
  leNearForwardEx        ->setText(QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iNearForwardEx));
  leLeftSubdivDist       ->setText(QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iLeftSubdivDist));
  leCenterSubdivDist     ->setText(QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iCenterSubdivDist));
  leRightSubdivDist      ->setText(QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iRightSubdivDist));
  leLWallSubdivDist      ->setText(QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iLWallSubdivDist));
  leRWallSubdivDist      ->setText(QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iRWallSubdivDist));
  leRoofSubdivDist       ->setText(QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iRoofSubdivDist));
  leLUOuterWallSubdivDist->setText(QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iLUOuterWallSubdivDist));
  leLLOuterWallSubdivDist->setText(QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iLLOuterWallSubdivDist));
  leOuterFloorSubdivDist ->setText(QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iOuterFloorSubdivDist));
  leRLOuterWallSubdivDist->setText(QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iRLOuterWallSubdivDist));
  leRUOuterWallSubdivDist->setText(QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iRUOuterWallSubdivDist));
  leNearBackward         ->setText(QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iNearBackward));
  leNearBackwardExStart  ->setText(QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iNearBackwardExStart));
  leNearBackwardEx       ->setText(QString::number(g_pMainWindow->GetCurrentTrack()->m_chunkAy[iFrom].iNearBackwardEx));

  UpdateTextures(leLeftSurfaceType, lblLSurfaceTex1, lblLSurfaceTex2);
  UpdateTextures(leCenterSurfaceType, lblCSurfaceTex1, lblCSurfaceTex2);
  UpdateTextures(leRightSurfaceType, lblRSurfaceTex1, lblRSurfaceTex2);
  UpdateTextures(leLWallType, lblLWallTex1, lblLWallTex2);
  UpdateTextures(leRWallType, lblRWallTex1, lblRWallTex2);
  UpdateTextures(leRoofType, lblRoofTex1, lblRoofTex2);
  UpdateTextures(leLUOuterWallType, lblLUOuterWallTex1, lblLUOuterWallTex2);
  UpdateTextures(leLLOuterWallType, lblLLOuterWallTex1, lblLLOuterWallTex2);
  UpdateTextures(leOuterFloorType, lblOuterFloorTex1, lblOuterFloorTex2);
  UpdateTextures(leRLOuterWallType, lblRLOuterWallTex1, lblRLOuterWallTex2);
  UpdateTextures(leRUOuterWallType, lblRUOuterWallTex1, lblRUOuterWallTex2);
  UpdateTextures(leEnvironmentFloorType, lblEnvirFloorTex1, lblEnvirFloorTex2);
}

//-------------------------------------------------------------------------------------------------

void CDebugChunkData::UpdateTextures(QLineEdit *pLineEdit, QLabel *pTex1, QLabel *pTex2)
{
  if (!g_pMainWindow->GetCurrentTrack() || !g_pMainWindow->GetCurrentTrack()->m_pTex)
    return;

  //textures
  QPixmap pixmap;
  int iIndex;
  if (pLineEdit->text().compare("-1") == 0) {
    pTex1->setPixmap(QPixmap());
    pTex2->setPixmap(QPixmap());
  } else if (!pLineEdit->text().isEmpty()) {
    int iValue = pLineEdit->text().toInt();
    unsigned int uiSignedBitVal = CTrack::GetSignedBitValueFromInt(iValue);
    iIndex = CTrack::GetIntValueFromSignedBit(uiSignedBitVal & SURFACE_MASK_TEXTURE_INDEX);
    if (iIndex < g_pMainWindow->GetCurrentTrack()->m_pTex->GetNumTiles()) {
      pixmap.convertFromImage(QtHelpers::GetQImageFromTile(g_pMainWindow->GetCurrentTrack()->m_pTex->m_pTileAy[iIndex]));
      pTex1->setPixmap(pixmap);

      if (uiSignedBitVal & SURFACE_FLAG_TEXTURE_PAIR && iIndex > 0) {
        pixmap.convertFromImage(QtHelpers::GetQImageFromTile(g_pMainWindow->GetCurrentTrack()->m_pTex->m_pTileAy[iIndex + 1]));
        pTex2->setPixmap(pixmap);
      } else {
        pTex2->setPixmap(QPixmap());
      }
    }
  } else {
    pTex1->setPixmap(QPixmap());
    pTex2->setPixmap(QPixmap());
  }
}

//-------------------------------------------------------------------------------------------------