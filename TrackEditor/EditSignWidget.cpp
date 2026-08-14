#include "EditSignWidget.h"
#include "Track.h"
#include "Texture.h"
#include "Palette.h"
#include "MainWindow.h"
#include "QtHelpers.h"
#include "EditSurfaceDialog.h"
#include "EditorSignModel.h"
#include "SignType.h"
//-------------------------------------------------------------------------------------------------
#if defined(_DEBUG) && defined(IS_WINDOWS)
#define new new(_CLIENT_BLOCK, __FILE__, __LINE__)
#endif
//-------------------------------------------------------------------------------------------------

CEditSignWidget::CEditSignWidget(QWidget *pParent)
  : QWidget(pParent)
{
  setupUi(this);

  for (int i = 0; i < g_signAyCount; ++i) {
    cbType->addItem(g_signAy[i].sDescription.c_str(), i);
  }
  lblUnk->hide();
  leUnk->hide();
  lblTowerDisabled->setStyleSheet("QLabel { color : red; }");
  lblTowerDisabled->hide();

  connect(g_pMainWindow, &CMainWindow::UpdateGeometrySelectionSig, this, &CEditSignWidget::UpdateGeometrySelection);

  connect(dsbYaw    , SIGNAL(valueChanged(double)),     this, SLOT(YawChanged(double)));
  connect(dsbPitch  , SIGNAL(valueChanged(double)),     this, SLOT(PitchChanged(double)));
  connect(dsbRoll   , SIGNAL(valueChanged(double)),     this, SLOT(RollChanged(double)));
  connect(sbHOffset , SIGNAL(valueChanged(int)),        this, SLOT(HOffsetChanged(int)));
  connect(sbVOffset , SIGNAL(valueChanged(int)),        this, SLOT(VOffsetChanged(int)));
  connect(cbType    , SIGNAL(currentIndexChanged(int)), this, SLOT(TypeChanged(int)));
  connect(pbEdit    , SIGNAL(clicked()),                this, SLOT(EditClicked()));
  connect(pbSign    , SIGNAL(clicked()),                this, SLOT(SignClicked()));
  connect(leUnk, &QLineEdit::textChanged, this, &CEditSignWidget::UnkChanged);
}

//-------------------------------------------------------------------------------------------------

CEditSignWidget::~CEditSignWidget()
{
}

//-------------------------------------------------------------------------------------------------

bool CEditSignWidget::GetSelection(CTrack *&pTrackOut,
                                   int &iFromOut, int &iToOut) const
{
  pTrackOut = g_pMainWindow->GetCurrentTrack();
  iFromOut = g_pMainWindow->GetSelFrom();
  iToOut = g_pMainWindow->GetSelTo();
  return pTrackOut && iFromOut >= 0 && iToOut >= iFromOut
      && iToOut < static_cast<int>(pTrackOut->m_chunkAy.size());
}

//-------------------------------------------------------------------------------------------------

void CEditSignWidget::CommitEdit(int iEdited, const QString &sDescription)
{
  if (iEdited == 0)
    return;
  g_pMainWindow->SaveHistory(sDescription);
  g_pMainWindow->UpdateWindow();
}

//-------------------------------------------------------------------------------------------------

void CEditSignWidget::UpdateGeometrySelection(int iFrom, int iTo)
{
  (void)(iTo);
  CTrack *pTrack = g_pMainWindow->GetCurrentTrack();
  if (!pTrack || iFrom < 0
      || iFrom >= static_cast<int>(pTrack->m_chunkAy.size()))
    return;

  const tGeometryChunk &Chunk = pTrack->m_chunkAy[iFrom];
  const int iSignType = Chunk.iSignType;
  const bool bHasTower = CEditorSignModel::IsTower(iSignType);
  // Preserve the legacy treatment of non-tower raw values; E7-S7 only splits
  // the >= 256 tower namespace away from the sign controls.
  const bool bChunkHasSign = iSignType != -1 && !bHasTower;
  const bool bKnownSign = CEditorSignModel::IsKnownSignIndex(
      iSignType, g_signAyCount);
  const bool bCanHaveTexture = bKnownSign
      && g_signAy[iSignType].bCanHaveTexture;
  // E7-S7 regression: the table may only be indexed after both bounds have
  // been established. Tower values and the empty -1 value never reach it.
  const bool bBillboarded = iSignType >= 0
      && iSignType < g_signAyCount
      && g_signAy[iSignType].bBillboarded;

  BLOCK_SIG_AND_DO(dsbYaw, setValue(Chunk.dSignYaw));
  BLOCK_SIG_AND_DO(dsbPitch, setValue(Chunk.dSignPitch));
  BLOCK_SIG_AND_DO(dsbRoll, setValue(Chunk.dSignRoll));
  BLOCK_SIG_AND_DO(sbHOffset, setValue(Chunk.iSignHorizOffset));
  BLOCK_SIG_AND_DO(sbVOffset, setValue(Chunk.iSignVertOffset));
  // Keep the last actual sign selected while a tower owns this shared field.
  // In particular, findData(>= 256) must not replace cbType with index -1.
  if (!bHasTower) {
    BLOCK_SIG_AND_DO(cbType, setCurrentIndex(cbType->findData(iSignType)));
    BLOCK_SIG_AND_DO(leUnk, setText(QString::number(iSignType)));
  }

  dsbYaw->setEnabled(bChunkHasSign && !bBillboarded);
  dsbPitch->setEnabled(bChunkHasSign);
  dsbRoll->setEnabled(bChunkHasSign);
  sbHOffset->setEnabled(bChunkHasSign);
  sbVOffset->setEnabled(bChunkHasSign);
  cbType->setEnabled(bChunkHasSign);
  leUnk->setEnabled(bChunkHasSign);
  pbEdit->setEnabled(bChunkHasSign && bCanHaveTexture);
  pbSign->setEnabled(!bHasTower);
  lblTex->setEnabled(bChunkHasSign);
  lblYaw->setEnabled(bChunkHasSign);
  lblPitch->setEnabled(bChunkHasSign);
  lblRoll->setEnabled(bChunkHasSign);
  lblHOffset->setEnabled(bChunkHasSign);
  lblVOffset->setEnabled(bChunkHasSign);
  lblType->setEnabled(bChunkHasSign);
  pbSign->setText(bChunkHasSign ? "Delete Sign" : "Add Sign");

  lblTowerDisabled->setVisible(bHasTower);
  lblTowerDisabled->setEnabled(true);
  const bool bUnk = bChunkHasSign && iSignType > 255;
  leUnk->setVisible(bUnk);
  lblUnk->setVisible(bUnk);

  QtHelpers::UpdateTextures(lblTex, NULL, pTrack->m_assets.GetSignTexture(),
                            pTrack->m_assets.GetPalette(), Chunk.iSignTexture);
}

//-------------------------------------------------------------------------------------------------

void CEditSignWidget::YawChanged(double dVal)
{
  CTrack *pTrack = nullptr;
  int iFrom = 0;
  int iTo = 0;
  if (!GetSelection(pTrack, iFrom, iTo))
    return;
  CommitEdit(CEditorSignModel::ApplyToRange(
      pTrack->m_chunkAy, iFrom, iTo,
      [dVal](tGeometryChunk &Chunk) { Chunk.dSignYaw = dVal; }),
      "Changed sign yaw");
}

//-------------------------------------------------------------------------------------------------

void CEditSignWidget::PitchChanged(double dVal)
{
  CTrack *pTrack = nullptr;
  int iFrom = 0;
  int iTo = 0;
  if (!GetSelection(pTrack, iFrom, iTo))
    return;
  CommitEdit(CEditorSignModel::ApplyToRange(
      pTrack->m_chunkAy, iFrom, iTo,
      [dVal](tGeometryChunk &Chunk) { Chunk.dSignPitch = dVal; }),
      "Changed sign pitch");
}

//-------------------------------------------------------------------------------------------------

void CEditSignWidget::RollChanged(double dVal)
{
  CTrack *pTrack = nullptr;
  int iFrom = 0;
  int iTo = 0;
  if (!GetSelection(pTrack, iFrom, iTo))
    return;
  CommitEdit(CEditorSignModel::ApplyToRange(
      pTrack->m_chunkAy, iFrom, iTo,
      [dVal](tGeometryChunk &Chunk) { Chunk.dSignRoll = dVal; }),
      "Changed sign roll");
}

//-------------------------------------------------------------------------------------------------

void CEditSignWidget::HOffsetChanged(int iVal)
{
  CTrack *pTrack = nullptr;
  int iFrom = 0;
  int iTo = 0;
  if (!GetSelection(pTrack, iFrom, iTo))
    return;
  CommitEdit(CEditorSignModel::ApplyToRange(
      pTrack->m_chunkAy, iFrom, iTo,
      [iVal](tGeometryChunk &Chunk) { Chunk.iSignHorizOffset = iVal; }),
      "Changed sign horiz offset");
}

//-------------------------------------------------------------------------------------------------

void CEditSignWidget::VOffsetChanged(int iVal)
{
  CTrack *pTrack = nullptr;
  int iFrom = 0;
  int iTo = 0;
  if (!GetSelection(pTrack, iFrom, iTo))
    return;
  CommitEdit(CEditorSignModel::ApplyToRange(
      pTrack->m_chunkAy, iFrom, iTo,
      [iVal](tGeometryChunk &Chunk) { Chunk.iSignVertOffset = iVal; }),
      "Changed sign vert offset");
}

//-------------------------------------------------------------------------------------------------

void CEditSignWidget::TypeChanged(int iIndex)
{
  CTrack *pTrack = nullptr;
  int iFrom = 0;
  int iTo = 0;
  if (iIndex < 0 || !GetSelection(pTrack, iFrom, iTo))
    return;
  if (CEditorSignModel::IsTower(pTrack->m_chunkAy[iFrom].iSignType))
    return;

  const int iSignType = cbType->itemData(iIndex).toInt();
  if (!CEditorSignModel::IsSign(iSignType))
    return;
  const bool bCanHaveTexture =
      CEditorSignModel::IsKnownSignIndex(iSignType, g_signAyCount)
      && g_signAy[iSignType].bCanHaveTexture;
  const int iEdited = CEditorSignModel::ApplyToRange(
      pTrack->m_chunkAy, iFrom, iTo,
      [iSignType, bCanHaveTexture](tGeometryChunk &Chunk) {
        Chunk.iSignType = iSignType;
        if (bCanHaveTexture)
          Chunk.iSignTexture = SURFACE_FLAG_APPLY_TEXTURE;
      });
  // Preserve the old non-textured-type behavior: only the displayed chunk's
  // texture field was cleared, while textured types initialized the range.
  if (iEdited != 0 && !bCanHaveTexture)
    pTrack->m_chunkAy[iFrom].iSignTexture = -1;
  CommitEdit(iEdited, "Changed sign type");
}

//-------------------------------------------------------------------------------------------------

void CEditSignWidget::EditClicked()
{
  CTrack *pTrack = nullptr;
  int iFrom = 0;
  int iTo = 0;
  if (!GetSelection(pTrack, iFrom, iTo)
      || CEditorSignModel::IsTower(pTrack->m_chunkAy[iFrom].iSignType)) {
    return;
  }

  CEditSurfaceDialog dlg(this, eSurfaceField::SURFACE_SIGN);
  dlg.exec();

  g_pMainWindow->SaveHistory("Changed sign texture");
  g_pMainWindow->UpdateWindow();
}

//-------------------------------------------------------------------------------------------------

void CEditSignWidget::SignClicked()
{
  CTrack *pTrack = nullptr;
  int iFrom = 0;
  int iTo = 0;
  if (!GetSelection(pTrack, iFrom, iTo)
      || CEditorSignModel::IsTower(pTrack->m_chunkAy[iFrom].iSignType)) {
    return;
  }

  const bool bHasSign = pTrack->m_chunkAy[iFrom].iSignType != -1;
  CommitEdit(CEditorSignModel::ApplyToRange(
      pTrack->m_chunkAy, iFrom, iTo,
      [bHasSign](tGeometryChunk &Chunk) {
        Chunk.iSignType = bHasSign ? -1 : 9; // Balloon is the default sign.
        if (!bHasSign)
          Chunk.iSignTexture = SURFACE_FLAG_APPLY_TEXTURE;
      }), bHasSign ? "Removed sign" : "Added sign");
}

//-------------------------------------------------------------------------------------------------

void CEditSignWidget::UnkChanged(const QString &sText)
{
  bool bOk = false;
  const int iSignType = sText.toInt(&bOk);
  if (!bOk || !CEditorSignModel::IsSign(iSignType))
    return;

  CTrack *pTrack = nullptr;
  int iFrom = 0;
  int iTo = 0;
  if (!GetSelection(pTrack, iFrom, iTo))
    return;
  CommitEdit(CEditorSignModel::ApplyToRange(
      pTrack->m_chunkAy, iFrom, iTo,
      [iSignType](tGeometryChunk &Chunk) {
        Chunk.iSignType = iSignType;
      }), "Changed unk sign value");
}

//-------------------------------------------------------------------------------------------------
