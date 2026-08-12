#include "TrackEditor.h"
#include "BackWidget.h"
#include "Texture.h"
#include "Palette.h"
#include "QtHelpers.h"
#include "Track.h"
#include "EditSurfaceDialog.h"
//-------------------------------------------------------------------------------------------------
#if defined(_DEBUG) && defined(IS_WINDOWS)
  #define new new(_CLIENT_BLOCK, __FILE__, __LINE__)
#endif
//-------------------------------------------------------------------------------------------------

CBackWidget::CBackWidget(QWidget *pParent, int iIndex, CTexture *pTexture, CPalette *pPal, int iBack)
  // Declaration order, which is the order these actually initialize in.
  : QWidget(pParent)
  , m_iIndex(iIndex)
  , m_iBack(iBack)
  , m_pTexture(pTexture)
  , m_pPalette(pPal)
{
  setupUi(this);
  lblIndex->setText(QString::number(iIndex));
  
  QPixmap pixmap;
  pixmap.convertFromImage(QtHelpers::GetQImageFromTile(pTexture->m_pTileAy[iIndex]));
  lblTex->setPixmap(pixmap);

  connect(pbBack, &QPushButton::clicked, this, &CBackWidget::OnBackClicked);

  UpdateWidget();
}

//-------------------------------------------------------------------------------------------------

CBackWidget::~CBackWidget()
{

}

//-------------------------------------------------------------------------------------------------

int CBackWidget::GetIndex()
{
  return m_iIndex;
}

//-------------------------------------------------------------------------------------------------

int CBackWidget::GetBack()
{
  return m_iBack;
}

//-------------------------------------------------------------------------------------------------

void CBackWidget::OnBackClicked()
{
  CEditSurfaceDialog dlg(this, eSurfaceField::SURFACE_BACK, true, "", false, true, m_iBack);
  if (dlg.exec())
    m_iBack = dlg.GetDialogValue();

  UpdateWidget();
}

//-------------------------------------------------------------------------------------------------

void CBackWidget::UpdateWidget()
{
  if (m_iBack < 0)
    return;

  uint32 uiBackIndex = CTrack::GetSignedBitValueFromInt(m_iBack);
  uiBackIndex &= SURFACE_MASK_TEXTURE_INDEX;
  if ((int)uiBackIndex < m_pTexture->GetNumTiles()) {
    QPixmap backPixmap;
    backPixmap.convertFromImage(QtHelpers::GetQImageFromTile(m_pTexture->m_pTileAy[uiBackIndex]));
    pbBack->setIcon(backPixmap);
  }
}

//-------------------------------------------------------------------------------------------------