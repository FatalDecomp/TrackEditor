#include "TrackEditor.h"
#include "EditSurfaceDialog.h"
#include "Texture.h"
#include "Palette.h"
#include "TilePicker.h"
#include "Track.h"
#include "QtHelpers.h"
#include "MainWindow.h"
//-------------------------------------------------------------------------------------------------
#if defined(_DEBUG) && defined(IS_WINDOWS)
  #define new new(_CLIENT_BLOCK, __FILE__, __LINE__)
#endif
//-------------------------------------------------------------------------------------------------

CEditSurfaceDialog::CEditSurfaceDialog(QWidget *pParent, eSurfaceField field, 
                                       bool bShowDisable, 
                                       const QString &sDisableEffects, 
                                       bool bShowDisableAttach,
                                       bool bApplyCancel,
                                       int iValue)
  : QDialog(pParent)
  , m_field(field)
  , m_bShowDisable(bShowDisable)
  , m_bShowDisableAttach(bShowDisableAttach)
  , m_bApplyCancel(bApplyCancel)
{
  m_uiSignedBitValue = CTrack::GetSignedBitValueFromInt(iValue);
  setupUi(this);

  if (!bApplyCancel) {
    pbApply->hide();
    pbCancel->setText("Close");
  }

  ckDisable->setVisible(bShowDisable);
  lblDisableEffects->setVisible(bShowDisable);
  ckDisableAttach->setVisible(bShowDisableAttach);
  lblDisableEffects->setText(sDisableEffects);
  for (int i = 0; i < g_transparencyAyCount; ++i) {
    cbTransparency->addItem(g_transparencyAy[i].c_str());
  }

  UpdateDialog();

  connect(ckDisable,            &QCheckBox::toggled, this, &CEditSurfaceDialog::OnDisableChecked);
  connect(ckDisableAttach,      &QCheckBox::toggled, this, &CEditSurfaceDialog::OnDisableAttachChecked);
  connect(ck31Wall,             &QCheckBox::toggled, this, &CEditSurfaceDialog::On31WallChecked);
  connect(ck30Bounce,           &QCheckBox::toggled, this, &CEditSurfaceDialog::On30BounceChecked);
  connect(ck29Echo,             &QCheckBox::toggled, this, &CEditSurfaceDialog::On29EchoChecked);
  connect(ck28,                 &QCheckBox::toggled, this, &CEditSurfaceDialog::On28Checked);
  connect(ck27PairNext,         &QCheckBox::toggled, this, &CEditSurfaceDialog::On27PairNextChecked);
  connect(ck26,                 &QCheckBox::toggled, this, &CEditSurfaceDialog::On26Checked);
  connect(ck25Pit,              &QCheckBox::toggled, this, &CEditSurfaceDialog::On25PitChecked);
  connect(ck24Yellow,           &QCheckBox::toggled, this, &CEditSurfaceDialog::On24YellowMapChecked);
  connect(ck23,                 &QCheckBox::toggled, this, &CEditSurfaceDialog::On23Checked);
  connect(ck22Wall,             &QCheckBox::toggled, this, &CEditSurfaceDialog::On22WallChecked);
  connect(ck21Transparent,      &QCheckBox::toggled, this, &CEditSurfaceDialog::On21TransparentChecked);
  connect(ck20Bounce,           &QCheckBox::toggled, this, &CEditSurfaceDialog::On20BounceChecked);
  connect(ck19NonMagnetic,      &QCheckBox::toggled, this, &CEditSurfaceDialog::On19NonMagneticChecked);
  connect(ck18FlipVertically,   &QCheckBox::toggled, this, &CEditSurfaceDialog::On18FlipVertChecked);
  connect(ck17NonSolid,         &QCheckBox::toggled, this, &CEditSurfaceDialog::On17NonSolidChecked);
  connect(ck16TexturePair,      &QCheckBox::toggled, this, &CEditSurfaceDialog::On16TexturePairChecked);
  connect(ck15Livery,           &QCheckBox::toggled, this, &CEditSurfaceDialog::On15LiveryChecked);
  connect(ck14,                 &QCheckBox::toggled, this, &CEditSurfaceDialog::On14Checked);
  connect(ck13Motion,           &QCheckBox::toggled, this, &CEditSurfaceDialog::On13MotionChecked);
  connect(ck12FlipHorizontally, &QCheckBox::toggled, this, &CEditSurfaceDialog::On12FlipHorizChecked);
  connect(ck11Back,             &QCheckBox::toggled, this, &CEditSurfaceDialog::On11BackChecked);
  connect(ck10PartialTrans,     &QCheckBox::toggled, this, &CEditSurfaceDialog::On10PartialTransChecked);
  connect(ck9AnmsLookup,        &QCheckBox::toggled, this, &CEditSurfaceDialog::On9AnmsLookupChecked);
  connect(ck8ApplyTexture,      &QCheckBox::toggled, this, &CEditSurfaceDialog::On8ApplyTextureChecked);
  connect(pbTexture1,           &QPushButton::clicked, this, &CEditSurfaceDialog::OnTextureClicked);
  connect(cbTransparency, SIGNAL(currentIndexChanged(int)), this, SLOT(OnTransparencyTypeChanged(int)));

  connect(pbCancel, &QPushButton::clicked, this, &CEditSurfaceDialog::reject);
  connect(pbApply, &QPushButton::clicked, this, &CEditSurfaceDialog::accept);
}

//-------------------------------------------------------------------------------------------------

CEditSurfaceDialog::~CEditSurfaceDialog()
{

}

//-------------------------------------------------------------------------------------------------

int CEditSurfaceDialog::GetDialogValue()
{
  return CTrack::GetIntValueFromSignedBit(m_uiSignedBitValue);
}

//-------------------------------------------------------------------------------------------------

void CEditSurfaceDialog::OnDisableChecked(bool bChecked)
{
  if (m_bApplyCancel) {
    if (bChecked)
      m_uiSignedBitValue = CTrack::GetSignedBitValueFromInt(-1);
    else
      m_uiSignedBitValue = 0;
  } else {
    int iFrom = g_pMainWindow->GetSelFrom();
    int iTo = g_pMainWindow->GetSelTo();

    if (!g_pMainWindow->GetCurrentTrack()
        || iFrom >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size()
        || iTo >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size())
      return;

    for (int i = iFrom; i <= iTo; ++i) {
      if (bChecked)
        GetValue(i) = -1;
      else
        GetValue(i) = 0;
    }

    g_pMainWindow->SaveHistory("Changed surface");
    g_pMainWindow->UpdateWindow();
  }
  UpdateDialog();
}

//-------------------------------------------------------------------------------------------------

void CEditSurfaceDialog::OnDisableAttachChecked(bool bChecked)
{
  if (m_bApplyCancel) {
    if (bChecked)
      m_uiSignedBitValue = CTrack::GetSignedBitValueFromInt(-2);
    else
      m_uiSignedBitValue = 0;
  } else {
    int iFrom = g_pMainWindow->GetSelFrom();
    int iTo = g_pMainWindow->GetSelTo();

    if (!g_pMainWindow->GetCurrentTrack()
        || iFrom >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size()
        || iTo >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size())
      return;

    for (int i = iFrom; i <= iTo; ++i) {
      if (bChecked)
        GetValue(i) = -2;
      else
        GetValue(i) = 0;
    }

    g_pMainWindow->SaveHistory("Changed surface");
    g_pMainWindow->UpdateWindow();
  }
  UpdateDialog();
}

//-------------------------------------------------------------------------------------------------

void CEditSurfaceDialog::On31WallChecked(bool bChecked)
{
  UpdateValueHelper(SURFACE_FLAG_WALL_31, bChecked);
}

//-------------------------------------------------------------------------------------------------

void CEditSurfaceDialog::On30BounceChecked(bool bChecked)
{
  UpdateValueHelper(SURFACE_FLAG_BOUNCE_30, bChecked);
}

//-------------------------------------------------------------------------------------------------

void CEditSurfaceDialog::On29EchoChecked(bool bChecked)
{
  UpdateValueHelper(SURFACE_FLAG_ECHO, bChecked);
}

//-------------------------------------------------------------------------------------------------

void CEditSurfaceDialog::On28Checked(bool bChecked)
{
  UpdateValueHelper(SURFACE_FLAG_28, bChecked);
}

//-------------------------------------------------------------------------------------------------

void CEditSurfaceDialog::On27PairNextChecked(bool bChecked)
{
  UpdateValueHelper(SURFACE_FLAG_27, bChecked);
}

//-------------------------------------------------------------------------------------------------

void CEditSurfaceDialog::On26Checked(bool bChecked)
{
  UpdateValueHelper(SURFACE_FLAG_26, bChecked);
}

//-------------------------------------------------------------------------------------------------

void CEditSurfaceDialog::On25PitChecked(bool bChecked)
{
  UpdateValueHelper(SURFACE_FLAG_PIT, bChecked);
}

//-------------------------------------------------------------------------------------------------

void CEditSurfaceDialog::On24YellowMapChecked(bool bChecked)
{
  UpdateValueHelper(SURFACE_FLAG_YELLOW_MAP, bChecked);
}

//-------------------------------------------------------------------------------------------------

void CEditSurfaceDialog::On23Checked(bool bChecked)
{
  UpdateValueHelper(SURFACE_FLAG_23, bChecked);
}

//-------------------------------------------------------------------------------------------------

void CEditSurfaceDialog::On22WallChecked(bool bChecked)
{
  UpdateValueHelper(SURFACE_FLAG_WALL_22, bChecked);
}

//-------------------------------------------------------------------------------------------------

void CEditSurfaceDialog::On21TransparentChecked(bool bChecked)
{
  if (m_bApplyCancel) {
    if (bChecked) {
      m_uiSignedBitValue |= SURFACE_FLAG_TRANSPARENT;
      m_uiSignedBitValue &= ~SURFACE_FLAG_APPLY_TEXTURE;
      int iIndex = 1;
      m_uiSignedBitValue &= ~SURFACE_MASK_TEXTURE_INDEX;
      m_uiSignedBitValue |= iIndex;
    } else {
      m_uiSignedBitValue &= ~SURFACE_FLAG_TRANSPARENT;
    }
  } else {
    int iFrom = g_pMainWindow->GetSelFrom();
    int iTo = g_pMainWindow->GetSelTo();

    if (!g_pMainWindow->GetCurrentTrack()
        || iFrom >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size()
        || iTo >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size())
      return;

    for (int i = iFrom; i <= iTo; ++i) {
      uint32 uiValue = CTrack::GetSignedBitValueFromInt(GetValue(i));

      if (bChecked) {
        uiValue |= SURFACE_FLAG_TRANSPARENT;
        uiValue &= ~SURFACE_FLAG_APPLY_TEXTURE;
        int iIndex = 1;
        uiValue &= ~SURFACE_MASK_TEXTURE_INDEX;
        uiValue |= iIndex;
      } else {
        uiValue &= ~SURFACE_FLAG_TRANSPARENT;
      }

      GetValue(i) = CTrack::GetIntValueFromSignedBit(uiValue);
    }

    g_pMainWindow->SaveHistory("Changed surface");
    g_pMainWindow->UpdateWindow();
  }
  UpdateDialog();
}

//-------------------------------------------------------------------------------------------------

void CEditSurfaceDialog::On20BounceChecked(bool bChecked)
{
  UpdateValueHelper(SURFACE_FLAG_BOUNCE_20, bChecked);
}

//-------------------------------------------------------------------------------------------------

void CEditSurfaceDialog::On19NonMagneticChecked(bool bChecked)
{
  UpdateValueHelper(SURFACE_FLAG_NON_MAGNETIC, bChecked);
}

//-------------------------------------------------------------------------------------------------

void CEditSurfaceDialog::On18FlipVertChecked(bool bChecked)
{
  UpdateValueHelper(SURFACE_FLAG_FLIP_VERT, bChecked);
}

//-------------------------------------------------------------------------------------------------

void CEditSurfaceDialog::On17NonSolidChecked(bool bChecked)
{
  UpdateValueHelper(SURFACE_FLAG_SKIP_RENDER, bChecked);
}

//-------------------------------------------------------------------------------------------------

void CEditSurfaceDialog::On16TexturePairChecked(bool bChecked)
{
  UpdateValueHelper(SURFACE_FLAG_TEXTURE_PAIR, bChecked);
}

//-------------------------------------------------------------------------------------------------

void CEditSurfaceDialog::On15LiveryChecked(bool bChecked)
{
  UpdateValueHelper(SURFACE_FLAG_ANMS_LIVERY, bChecked);
}

//-------------------------------------------------------------------------------------------------

void CEditSurfaceDialog::On14Checked(bool bChecked)
{
  UpdateValueHelper(SURFACE_FLAG_CONCAVE, bChecked);
}

//-------------------------------------------------------------------------------------------------

void CEditSurfaceDialog::On13MotionChecked(bool bChecked)
{
  UpdateValueHelper(SURFACE_FLAG_FLIP_BACKFACE, bChecked);
}

//-------------------------------------------------------------------------------------------------

void CEditSurfaceDialog::On12FlipHorizChecked(bool bChecked)
{
  UpdateValueHelper(SURFACE_FLAG_FLIP_HORIZ, bChecked);
}

//-------------------------------------------------------------------------------------------------

void CEditSurfaceDialog::On11BackChecked(bool bChecked)
{
  UpdateValueHelper(SURFACE_FLAG_BACK, bChecked);
}

//-------------------------------------------------------------------------------------------------

void CEditSurfaceDialog::On10PartialTransChecked(bool bChecked)
{
  UpdateValueHelper(SURFACE_FLAG_PARTIAL_TRANS, bChecked);
}

//-------------------------------------------------------------------------------------------------

void CEditSurfaceDialog::On9AnmsLookupChecked(bool bChecked)
{
  UpdateValueHelper(SURFACE_FLAG_ANMS_LOOKUP, bChecked);
}

//-------------------------------------------------------------------------------------------------

void CEditSurfaceDialog::On8ApplyTextureChecked(bool bChecked)
{
  if (!GetTexture())
    return;

  if (m_bApplyCancel) {
    int iIndex = m_uiSignedBitValue & SURFACE_MASK_TEXTURE_INDEX;

    if (bChecked) {
      if (iIndex >= GetTexture()->GetNumTiles())
        m_uiSignedBitValue = 0;
      m_uiSignedBitValue |= SURFACE_FLAG_APPLY_TEXTURE;
      m_uiSignedBitValue &= ~SURFACE_FLAG_TRANSPARENT;
    } else {
      if (iIndex >= PALETTE_SIZE)
        m_uiSignedBitValue = 0;
      m_uiSignedBitValue &= ~SURFACE_FLAG_APPLY_TEXTURE;
    }
  } else {
    int iFrom = g_pMainWindow->GetSelFrom();
    int iTo = g_pMainWindow->GetSelTo();

    if (!g_pMainWindow->GetCurrentTrack()
        || iFrom >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size()
        || iTo >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size())
      return;

    for (int i = iFrom; i <= iTo; ++i) {
      uint32 uiValue = CTrack::GetSignedBitValueFromInt(GetValue(i));

      int iIndex = uiValue & SURFACE_MASK_TEXTURE_INDEX;

      if (bChecked) {
        if (iIndex >= GetTexture()->GetNumTiles())
          uiValue = 0;
        uiValue |= SURFACE_FLAG_APPLY_TEXTURE;
        uiValue &= ~SURFACE_FLAG_TRANSPARENT;
      } else {
        if (iIndex >= PALETTE_SIZE)
          uiValue = 0;
        uiValue &= ~SURFACE_FLAG_APPLY_TEXTURE;
      }

      GetValue(i) = CTrack::GetIntValueFromSignedBit(uiValue);
    }

    g_pMainWindow->SaveHistory("Changed surface");
    g_pMainWindow->UpdateWindow();
  }
  UpdateDialog();
}

//-------------------------------------------------------------------------------------------------

void CEditSurfaceDialog::OnTextureClicked()
{
  if (!GetTexture())
    return;

  if (m_bApplyCancel) {
    int iIndex = m_uiSignedBitValue & SURFACE_MASK_TEXTURE_INDEX;
    if (m_uiSignedBitValue & SURFACE_FLAG_APPLY_TEXTURE) {
      CTilePicker dlg(this, iIndex, GetTexture());
      if (dlg.exec()) {
        iIndex = dlg.GetSelected();
        m_uiSignedBitValue &= ~SURFACE_MASK_TEXTURE_INDEX;
        m_uiSignedBitValue |= iIndex;
      }
    } else {
      CTilePicker dlg(this, iIndex, g_pMainWindow->GetCurrentTrack()->m_pPal);
      if (dlg.exec()) {
        iIndex = dlg.GetSelected();
        m_uiSignedBitValue &= ~SURFACE_MASK_TEXTURE_INDEX;
        m_uiSignedBitValue |= iIndex;
      }
    }
  } else {
    int iFrom = g_pMainWindow->GetSelFrom();
    int iTo = g_pMainWindow->GetSelTo();

    if (!g_pMainWindow->GetCurrentTrack()
        || iFrom >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size()
        || iTo >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size())
      return;

    uint32 uiFromValue = CTrack::GetSignedBitValueFromInt(GetValue(iFrom));
    int iIndex = uiFromValue & SURFACE_MASK_TEXTURE_INDEX;
    if (uiFromValue & SURFACE_FLAG_APPLY_TEXTURE) {
      CTilePicker dlg(this, iIndex, GetTexture());
      if (dlg.exec()) {
        iIndex = dlg.GetSelected();
      }
    } else {
      CTilePicker dlg(this, iIndex, g_pMainWindow->GetCurrentTrack()->m_pPal);
      if (dlg.exec()) {
        iIndex = dlg.GetSelected();
      }
    }

    for (int i = iFrom; i <= iTo; ++i) {
      uint32 uiValue = CTrack::GetSignedBitValueFromInt(GetValue(i));
      uiValue &= ~SURFACE_MASK_TEXTURE_INDEX;
      uiValue |= iIndex;

      if (uiFromValue & SURFACE_FLAG_APPLY_TEXTURE)
        uiValue |= SURFACE_FLAG_APPLY_TEXTURE;
      else
        uiValue &= ~SURFACE_FLAG_APPLY_TEXTURE;

      GetValue(i) = CTrack::GetIntValueFromSignedBit(uiValue);
    }

    g_pMainWindow->SaveHistory("Changed surface");
    g_pMainWindow->UpdateWindow();
  }
  UpdateDialog();
}

//-------------------------------------------------------------------------------------------------

void CEditSurfaceDialog::OnTransparencyTypeChanged(int iIndex)
{
  if (m_bApplyCancel) {
    m_uiSignedBitValue &= ~SURFACE_MASK_TEXTURE_INDEX;
    m_uiSignedBitValue |= iIndex;
  } else {
    int iFrom = g_pMainWindow->GetSelFrom();
    int iTo = g_pMainWindow->GetSelTo();

    if (!g_pMainWindow->GetCurrentTrack()
        || iFrom >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size()
        || iTo >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size())
      return;

    for (int i = iFrom; i <= iTo; ++i) {
      uint32 uiValue = CTrack::GetSignedBitValueFromInt(GetValue(i));

      uiValue &= ~SURFACE_MASK_TEXTURE_INDEX;
      uiValue |= iIndex;

      GetValue(i) = CTrack::GetIntValueFromSignedBit(uiValue);
    }

    g_pMainWindow->SaveHistory("Changed surface");
    g_pMainWindow->UpdateWindow();
  }
  UpdateDialog();
}

//-------------------------------------------------------------------------------------------------

void CEditSurfaceDialog::UpdateValueHelper(uint32 uiFlag, bool bChecked)
{
  if (m_bApplyCancel) {
  } else {
    int iFrom = g_pMainWindow->GetSelFrom();
    int iTo = g_pMainWindow->GetSelTo();

    if (!g_pMainWindow->GetCurrentTrack()
        || iFrom >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size()
        || iTo >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size())
      return;

    for (int i = iFrom; i <= iTo; ++i) {
      uint32 uiValue = CTrack::GetSignedBitValueFromInt(GetValue(i));

      if (bChecked)
        uiValue |= uiFlag;
      else
        uiValue &= ~uiFlag;

      GetValue(i) = CTrack::GetIntValueFromSignedBit(uiValue);
    }

    g_pMainWindow->SaveHistory("Changed surface");
    g_pMainWindow->UpdateWindow();
  }
  UpdateDialog();
}

//-------------------------------------------------------------------------------------------------

int &CEditSurfaceDialog::GetValue(int i)
{
  if (!g_pMainWindow->GetCurrentTrack()
      || i >= (int)g_pMainWindow->GetCurrentTrack()->m_chunkAy.size())
    return (int&)m_uiSignedBitValue;

  switch (m_field) {
    case eSurfaceField::SURFACE_CENTER:    return g_pMainWindow->GetCurrentTrack()->m_chunkAy[i].iCenterSurfaceType;     break;
    case eSurfaceField::SURFACE_LSHOULDER: return g_pMainWindow->GetCurrentTrack()->m_chunkAy[i].iLeftSurfaceType;       break;
    case eSurfaceField::SURFACE_RSHOULDER: return g_pMainWindow->GetCurrentTrack()->m_chunkAy[i].iRightSurfaceType;      break;
    case eSurfaceField::SURFACE_LWALL:     return g_pMainWindow->GetCurrentTrack()->m_chunkAy[i].iLeftWallType;          break;
    case eSurfaceField::SURFACE_RWALL:     return g_pMainWindow->GetCurrentTrack()->m_chunkAy[i].iRightWallType;         break;
    case eSurfaceField::SURFACE_ROOF:      return g_pMainWindow->GetCurrentTrack()->m_chunkAy[i].iRoofType;              break;
    case eSurfaceField::SURFACE_LUOWALL:   return g_pMainWindow->GetCurrentTrack()->m_chunkAy[i].iLUOuterWallType;       break;
    case eSurfaceField::SURFACE_LLOWALL:   return g_pMainWindow->GetCurrentTrack()->m_chunkAy[i].iLLOuterWallType;       break;
    case eSurfaceField::SURFACE_OFLOOR:    return g_pMainWindow->GetCurrentTrack()->m_chunkAy[i].iOuterFloorType;        break;
    case eSurfaceField::SURFACE_RLOWALL:   return g_pMainWindow->GetCurrentTrack()->m_chunkAy[i].iRLOuterWallType;       break;
    case eSurfaceField::SURFACE_RUOWALL:   return g_pMainWindow->GetCurrentTrack()->m_chunkAy[i].iRUOuterWallType;       break;
    case eSurfaceField::SURFACE_ENVFLOOR:  return g_pMainWindow->GetCurrentTrack()->m_chunkAy[i].iEnvironmentFloorType;  break;
    case eSurfaceField::SURFACE_SIGN:      return g_pMainWindow->GetCurrentTrack()->m_chunkAy[i].iSignTexture;           break;
    default: return g_pMainWindow->GetCurrentTrack()->m_chunkAy[i].iEnvironmentFloorType;
  }
}

//-------------------------------------------------------------------------------------------------w

CTexture *CEditSurfaceDialog::GetTexture()
{
  if (g_pMainWindow->GetCurrentTrack()) {
    if (m_field == eSurfaceField::SURFACE_SIGN)
      return g_pMainWindow->GetCurrentTrack()->m_pBld;
    else
      return g_pMainWindow->GetCurrentTrack()->m_pTex;
  }
  return NULL;
}

//-------------------------------------------------------------------------------------------------

void CEditSurfaceDialog::UpdateDialog()
{
  int iFrom = g_pMainWindow->GetSelFrom();

  int iValue = CTrack::GetIntValueFromSignedBit(m_uiSignedBitValue);
  if (!m_bApplyCancel)
    iValue = GetValue(iFrom);
  uint32 uiValue = CTrack::GetSignedBitValueFromInt(iValue);

  BLOCK_SIG_AND_DO(ckDisable            , setChecked(iValue == -1));
  BLOCK_SIG_AND_DO(ckDisableAttach      , setChecked(iValue == -2));
  BLOCK_SIG_AND_DO(ck31Wall             , setChecked(uiValue & SURFACE_FLAG_WALL_31));
  BLOCK_SIG_AND_DO(ck30Bounce           , setChecked(uiValue & SURFACE_FLAG_BOUNCE_30));
  BLOCK_SIG_AND_DO(ck29Echo             , setChecked(uiValue & SURFACE_FLAG_ECHO));
  BLOCK_SIG_AND_DO(ck28                 , setChecked(uiValue & SURFACE_FLAG_28));
  BLOCK_SIG_AND_DO(ck27PairNext         , setChecked(uiValue & SURFACE_FLAG_27));
  BLOCK_SIG_AND_DO(ck26                 , setChecked(uiValue & SURFACE_FLAG_26));
  BLOCK_SIG_AND_DO(ck25Pit              , setChecked(uiValue & SURFACE_FLAG_PIT));
  BLOCK_SIG_AND_DO(ck24Yellow           , setChecked(uiValue & SURFACE_FLAG_YELLOW_MAP));
  BLOCK_SIG_AND_DO(ck23                 , setChecked(uiValue & SURFACE_FLAG_23));
  BLOCK_SIG_AND_DO(ck22Wall             , setChecked(uiValue & SURFACE_FLAG_WALL_22));
  BLOCK_SIG_AND_DO(ck21Transparent      , setChecked(uiValue & SURFACE_FLAG_TRANSPARENT));
  BLOCK_SIG_AND_DO(ck20Bounce           , setChecked(uiValue & SURFACE_FLAG_BOUNCE_20));
  BLOCK_SIG_AND_DO(ck19NonMagnetic      , setChecked(uiValue & SURFACE_FLAG_NON_MAGNETIC));
  BLOCK_SIG_AND_DO(ck18FlipVertically   , setChecked(uiValue & SURFACE_FLAG_FLIP_VERT));
  BLOCK_SIG_AND_DO(ck17NonSolid         , setChecked(uiValue & SURFACE_FLAG_SKIP_RENDER));
  BLOCK_SIG_AND_DO(ck16TexturePair      , setChecked(uiValue & SURFACE_FLAG_TEXTURE_PAIR));
  BLOCK_SIG_AND_DO(ck15Livery           , setChecked(uiValue & SURFACE_FLAG_ANMS_LIVERY));
  BLOCK_SIG_AND_DO(ck14                 , setChecked(uiValue & SURFACE_FLAG_CONCAVE));
  BLOCK_SIG_AND_DO(ck13Motion           , setChecked(uiValue & SURFACE_FLAG_FLIP_BACKFACE));
  BLOCK_SIG_AND_DO(ck12FlipHorizontally , setChecked(uiValue & SURFACE_FLAG_FLIP_HORIZ));
  BLOCK_SIG_AND_DO(ck11Back             , setChecked(uiValue & SURFACE_FLAG_BACK));
  BLOCK_SIG_AND_DO(ck10PartialTrans     , setChecked(uiValue & SURFACE_FLAG_PARTIAL_TRANS));
  BLOCK_SIG_AND_DO(ck9AnmsLookup        , setChecked(uiValue & SURFACE_FLAG_ANMS_LOOKUP));
  BLOCK_SIG_AND_DO(ck8ApplyTexture      , setChecked(uiValue & SURFACE_FLAG_APPLY_TEXTURE));

  bool bEnableSurface = true;
  if (m_bShowDisable)
    bEnableSurface &= (iValue != -1);
  if (m_bShowDisableAttach)
    bEnableSurface &= (iValue != -2);

  ck31Wall             ->setEnabled(bEnableSurface);
  ck30Bounce           ->setEnabled(bEnableSurface);
  ck29Echo             ->setEnabled(bEnableSurface);
  ck28                 ->setEnabled(bEnableSurface);
  ck27PairNext         ->setEnabled(bEnableSurface);
  ck26                 ->setEnabled(bEnableSurface);
  ck25Pit              ->setEnabled(bEnableSurface);
  ck24Yellow           ->setEnabled(bEnableSurface);
  ck23                 ->setEnabled(bEnableSurface);
  ck22Wall             ->setEnabled(bEnableSurface);
  ck21Transparent      ->setEnabled(bEnableSurface);
  ck20Bounce           ->setEnabled(bEnableSurface);
  ck19NonMagnetic      ->setEnabled(bEnableSurface);
  ck18FlipVertically   ->setEnabled(bEnableSurface);
  ck17NonSolid         ->setEnabled(bEnableSurface);
  ck16TexturePair      ->setEnabled(bEnableSurface);
  ck15Livery           ->setEnabled(bEnableSurface);
  ck14                 ->setEnabled(bEnableSurface);
  ck13Motion           ->setEnabled(bEnableSurface);
  ck12FlipHorizontally ->setEnabled(bEnableSurface);
  ck11Back             ->setEnabled(bEnableSurface);
  ck10PartialTrans     ->setEnabled(bEnableSurface);
  ck9AnmsLookup        ->setEnabled(bEnableSurface);
  ck8ApplyTexture      ->setEnabled(bEnableSurface);
  pbTexture1           ->setEnabled(bEnableSurface);

  //qstring::number makes negative values 64-bit for some reason
  char szBuf[128];
  snprintf(szBuf, sizeof(szBuf), " (%#010x)", uiValue);

  leValue->setFont(QFont("Courier", 8));
  leValue->setText(QString::number(iValue).leftJustified(10, ' ') + szBuf);

  //textures
  if (uiValue & SURFACE_FLAG_APPLY_TEXTURE) {
    pbTexture1->show();
    lblTexture2->show();
    lblTransparency->hide();
    cbTransparency->hide();
    int iIndex = uiValue & SURFACE_MASK_TEXTURE_INDEX;
    if (GetTexture() && iIndex < GetTexture()->GetNumTiles()) {
      QPixmap pixmap;
      pixmap.convertFromImage(QtHelpers::GetQImageFromTile(GetTexture()->m_pTileAy[iIndex]));
      pbTexture1->setIcon(pixmap);

      if (uiValue & SURFACE_FLAG_TEXTURE_PAIR && iIndex > 0) {
        QPixmap pixmap2;
        pixmap2.convertFromImage(QtHelpers::GetQImageFromTile(GetTexture()->m_pTileAy[iIndex + 1]));
        lblTexture2->setPixmap(pixmap2);
      } else {
        lblTexture2->setPixmap(QPixmap());
      }
    }
  } else if (uiValue & SURFACE_FLAG_TRANSPARENT) {
    pbTexture1->hide();
    lblTexture2->hide();
    lblTransparency->show();
    cbTransparency->show();
    int iIndex = uiValue & SURFACE_MASK_TEXTURE_INDEX;
    BLOCK_SIG_AND_DO(cbTransparency, setCurrentIndex(iIndex));
  } else {
    pbTexture1->show();
    lblTexture2->show();
    lblTransparency->hide();
    cbTransparency->hide();
    int iIndex = uiValue & SURFACE_MASK_TEXTURE_INDEX;
    if (iIndex < PALETTE_SIZE) {
      QPixmap pixmap;
      pixmap.convertFromImage(QtHelpers::GetQImageFromColor(g_pMainWindow->GetCurrentTrack()->m_pPal->m_paletteAy[iIndex]));
      pbTexture1->setIcon(pixmap);
    } else {
      pbTexture1->setIcon(QPixmap());
    }

    lblTexture2->setPixmap(QPixmap());
  }
}

//-------------------------------------------------------------------------------------------------