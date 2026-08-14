#include "EditTowerWidget.h"

#include "EditorTowerModel.h"
#include "MainWindow.h"
#include "QtHelpers.h"
#include "Track.h"
#include "TrackPreview.h"

#include <QIntValidator>
#include <limits>

//-------------------------------------------------------------------------------------------------

CEditTowerWidget::CEditTowerWidget(QWidget *pParent)
  : QWidget(pParent)
{
  setupUi(this);

  cbMode->addItem("Static", static_cast<int>(eEditorTowerMode::STATIC));
  cbMode->addItem("Follow near (25%)",
                  static_cast<int>(eEditorTowerMode::FOLLOW_NEAR));
  cbMode->addItem("Follow at distance",
                  static_cast<int>(eEditorTowerMode::FOLLOW_AT_DISTANCE));
  cbMode->addItem("Track surface, 2 back",
                  static_cast<int>(eEditorTowerMode::TRACK_SURFACE_TWO_BACK));
  cbMode->addItem("Overhead follow",
                  static_cast<int>(eEditorTowerMode::OVERHEAD_FOLLOW));

  cbZoom->addItem("Unchanged", 0);
  cbZoom->addItem("1 - VIEWDIST 120", 1);
  cbZoom->addItem("2 - VIEWDIST 75", 2);
  cbZoom->addItem("3 - VIEWDIST 500", 3);
  cbZoom->addItem("4 - VIEWDIST 750", 4);

  leRawType->setValidator(new QIntValidator(
      CEditorTowerModel::TOWER_TYPE_BASE,
      std::numeric_limits<int>::max(), leRawType));
  lblSignDisabled->setStyleSheet("QLabel { color : red; }");
  lblBudget->setStyleSheet("");
  lblSignDisabled->hide();
  lblRawPreserved->hide();
  lblOffsetScale->hide();

  connect(g_pMainWindow, &CMainWindow::UpdateGeometrySelectionSig,
          this, &CEditTowerWidget::UpdateGeometrySelection);
  connect(pbTower, &QPushButton::clicked,
          this, &CEditTowerWidget::TowerClicked);
  connect(pbViewFromTower, &QPushButton::clicked,
          this, &CEditTowerWidget::ViewFromTowerClicked);
  connect(cbMode, SIGNAL(currentIndexChanged(int)),
          this, SLOT(ModeChanged(int)));
  connect(cbZoom, SIGNAL(currentIndexChanged(int)),
          this, SLOT(ZoomChanged(int)));
  connect(sbHOffset, SIGNAL(valueChanged(int)),
          this, SLOT(HOffsetChanged(int)));
  connect(sbVOffset, SIGNAL(valueChanged(int)),
          this, SLOT(VOffsetChanged(int)));
  connect(leRawType, &QLineEdit::textChanged,
          this, &CEditTowerWidget::RawTypeChanged);
}

//-------------------------------------------------------------------------------------------------

CEditTowerWidget::~CEditTowerWidget()
{
}

//-------------------------------------------------------------------------------------------------

bool CEditTowerWidget::GetSelection(CTrack *&pTrackOut,
                                    int &iFromOut, int &iToOut) const
{
  pTrackOut = g_pMainWindow->GetCurrentTrack();
  iFromOut = g_pMainWindow->GetSelFrom();
  iToOut = g_pMainWindow->GetSelTo();
  return pTrackOut && iFromOut >= 0 && iToOut >= iFromOut
      && iToOut < static_cast<int>(pTrackOut->m_chunkAy.size());
}

//-------------------------------------------------------------------------------------------------

void CEditTowerWidget::UpdateGeometrySelection(int iFrom, int iTo)
{
  (void)iTo;
  CTrack *pTrack = g_pMainWindow->GetCurrentTrack();
  if (!pTrack || iFrom < 0 || iFrom >= static_cast<int>(pTrack->m_chunkAy.size()))
    return;

  const tGeometryChunk &Chunk = pTrack->m_chunkAy[iFrom];
  const bool bHasTower = CEditorTowerModel::IsTower(Chunk.iSignType);
  const bool bHasSign = CEditorTowerModel::IsSign(Chunk.iSignType);
  const int iTowerCount = CEditorTowerModel::CountTowers(pTrack->m_chunkAy);
  const eEditorTowerMode mode = bHasTower
      ? CEditorTowerModel::DecodeMode(Chunk.iSignType)
      : eEditorTowerMode::STATIC;
  const int iZoom = bHasTower
      ? CEditorTowerModel::DecodeZoom(Chunk.iSignType) : 0;

  BLOCK_SIG_AND_DO(cbMode, setCurrentIndex(
      cbMode->findData(static_cast<int>(mode))));
  BLOCK_SIG_AND_DO(cbZoom, setCurrentIndex(cbZoom->findData(iZoom)));
  BLOCK_SIG_AND_DO(sbHOffset, setValue(Chunk.iSignHorizOffset));
  BLOCK_SIG_AND_DO(sbVOffset, setValue(Chunk.iSignVertOffset));
  BLOCK_SIG_AND_DO(leRawType, setText(
      bHasTower ? QString::number(Chunk.iSignType) : QString()));

  const bool bCanAdd = !bHasSign && !bHasTower
      && iTowerCount < CEditorTowerModel::TOWER_LIMIT;
  pbTower->setText(bHasTower ? "Delete Tower" : "Add Tower");
  pbTower->setEnabled(bHasTower || bCanAdd);
  pbViewFromTower->setEnabled(bHasTower);

  cbMode->setEnabled(bHasTower);
  cbZoom->setEnabled(bHasTower);
  leRawType->setEnabled(bHasTower);
  sbHOffset->setEnabled(bHasTower
      && CEditorTowerModel::UsesHorizontalOffset(mode));
  sbVOffset->setEnabled(bHasTower
      && CEditorTowerModel::UsesVerticalOffset(mode));
  lblMode->setEnabled(bHasTower);
  lblZoom->setEnabled(bHasTower);
  lblRawType->setEnabled(bHasTower);
  lblHOffset->setEnabled(bHasTower);
  lblVOffset->setEnabled(bHasTower);

  lblSignDisabled->setVisible(bHasSign);
  // A disabled parent label is gray, so explicitly keep the red reason live.
  lblSignDisabled->setEnabled(true);

  QString sBudget = QString("%1 of %2")
      .arg(iTowerCount).arg(CEditorTowerModel::TOWER_LIMIT);
  if (iTowerCount >= CEditorTowerModel::TOWER_LIMIT)
    sBudget += " - tower limit reached";
  lblBudget->setText(sBudget);
  lblBudget->setStyleSheet(iTowerCount >= CEditorTowerModel::TOWER_LIMIT
      ? "QLabel { color : red; }" : "");

  const bool bOverhead = bHasTower
      && mode == eEditorTowerMode::OVERHEAD_FOLLOW;
  const bool bTrackSurface = bHasTower
      && mode == eEditorTowerMode::TRACK_SURFACE_TWO_BACK;
  lblOffsetScale->setText(bTrackSurface
      ? "Track-surface mode ignores both offsets."
      : (bOverhead
          ? "Overhead mode uses vertical offset x128 at runtime."
          : "Offsets use x32 track units at runtime."));
  lblOffsetScale->setVisible(bHasTower);
  lblRawPreserved->setVisible(bHasTower
      && !CEditorTowerModel::IsCanonical(Chunk.iSignType));
}

//-------------------------------------------------------------------------------------------------

void CEditTowerWidget::CommitEdit(int iChanged, const QString &sDescription)
{
  if (iChanged == 0)
    return;
  g_pMainWindow->SaveHistory(sDescription);
  g_pMainWindow->UpdateWindow();
}

//-------------------------------------------------------------------------------------------------

void CEditTowerWidget::TowerClicked()
{
  CTrack *pTrack = nullptr;
  int iFrom = 0;
  int iTo = 0;
  if (!GetSelection(pTrack, iFrom, iTo))
    return;

  // The displayed chunk owns the button's lifecycle. A sign is a defensive
  // no-op even if this slot is invoked directly while the button is disabled.
  if (CEditorTowerModel::IsSign(pTrack->m_chunkAy[iFrom].iSignType))
    return;

  const bool bDelete = CEditorTowerModel::IsTower(
      pTrack->m_chunkAy[iFrom].iSignType);
  const int iChanged = bDelete
      ? CEditorTowerModel::DeleteTowers(pTrack->m_chunkAy, iFrom, iTo)
      : CEditorTowerModel::AddTowers(pTrack->m_chunkAy, iFrom, iTo);
  CommitEdit(iChanged, bDelete ? "Removed tower" : "Added tower");
}

//-------------------------------------------------------------------------------------------------

void CEditTowerWidget::ViewFromTowerClicked()
{
  CTrack *pTrack = nullptr;
  int iFrom = 0;
  int iTo = 0;
  if (!GetSelection(pTrack, iFrom, iTo)
      || !CEditorTowerModel::IsTower(pTrack->m_chunkAy[iFrom].iSignType)) {
    return;
  }

  CTrackPreview *pPreview = g_pMainWindow->GetCurrentPreview();
  if (pPreview)
    pPreview->ViewFromTower(iFrom);
}

//-------------------------------------------------------------------------------------------------

void CEditTowerWidget::ModeChanged(int iIndex)
{
  CTrack *pTrack = nullptr;
  int iFrom = 0;
  int iTo = 0;
  if (iIndex < 0 || !GetSelection(pTrack, iFrom, iTo))
    return;
  const eEditorTowerMode mode = static_cast<eEditorTowerMode>(
      cbMode->itemData(iIndex).toInt());
  CommitEdit(CEditorTowerModel::SetMode(
      pTrack->m_chunkAy, iFrom, iTo, mode), "Changed tower camera mode");
}

//-------------------------------------------------------------------------------------------------

void CEditTowerWidget::ZoomChanged(int iIndex)
{
  CTrack *pTrack = nullptr;
  int iFrom = 0;
  int iTo = 0;
  if (iIndex < 0 || !GetSelection(pTrack, iFrom, iTo))
    return;
  CommitEdit(CEditorTowerModel::SetZoom(
      pTrack->m_chunkAy, iFrom, iTo, cbZoom->itemData(iIndex).toInt()),
      "Changed tower zoom");
}

//-------------------------------------------------------------------------------------------------

void CEditTowerWidget::HOffsetChanged(int iValue)
{
  CTrack *pTrack = nullptr;
  int iFrom = 0;
  int iTo = 0;
  if (!GetSelection(pTrack, iFrom, iTo))
    return;
  CommitEdit(CEditorTowerModel::SetHorizontalOffset(
      pTrack->m_chunkAy, iFrom, iTo, iValue),
      "Changed tower horizontal offset");
}

//-------------------------------------------------------------------------------------------------

void CEditTowerWidget::VOffsetChanged(int iValue)
{
  CTrack *pTrack = nullptr;
  int iFrom = 0;
  int iTo = 0;
  if (!GetSelection(pTrack, iFrom, iTo))
    return;
  CommitEdit(CEditorTowerModel::SetVerticalOffset(
      pTrack->m_chunkAy, iFrom, iTo, iValue),
      "Changed tower vertical offset");
}

//-------------------------------------------------------------------------------------------------

void CEditTowerWidget::RawTypeChanged(const QString &sText)
{
  bool bOk = false;
  const int iSignType = sText.toInt(&bOk);
  if (!bOk || !CEditorTowerModel::IsTower(iSignType))
    return;

  CTrack *pTrack = nullptr;
  int iFrom = 0;
  int iTo = 0;
  if (!GetSelection(pTrack, iFrom, iTo))
    return;
  CommitEdit(CEditorTowerModel::SetRawType(
      pTrack->m_chunkAy, iFrom, iTo, iSignType),
      "Changed raw tower type");
}
