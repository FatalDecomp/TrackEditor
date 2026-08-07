#include "QtHelpers.h"
#include "qlineedit.h"
#include "qcombobox.h"
#include "qpushbutton.h"
#include "qcheckbox.h"
#include "Track.h"
#include "Texture.h"
#include "EditSurfaceDialog.h"
#include "Texture.h"
#include "Palette.h"
#include "MainWindow.h"
//-------------------------------------------------------------------------------------------------
#if defined(_DEBUG) && defined(IS_WINDOWS)
#define new new(_CLIENT_BLOCK, __FILE__, __LINE__)
#endif
//-------------------------------------------------------------------------------------------------

void QtHelpers::UpdateSignButtonDisplay(QPushButton *pPushButton, QCheckBox *pCheckbox, QLineEdit *pLineEdit, CTexture *pTex)
{
  pCheckbox->blockSignals(true);
  if (pLineEdit->text().compare("-1") == 0) {
    pPushButton->setIcon(QIcon());
    pPushButton->setText("<none>");
    pCheckbox->setEnabled(false);
    pCheckbox->setChecked(false);
  } else if (pLineEdit->text().isEmpty()) {
    pPushButton->setIcon(QIcon());
    //pPushButton->setText(MIXED_DATA);
    pCheckbox->setEnabled(false);
    pCheckbox->setChecked(false);
  } else {
    int iValue = pLineEdit->text().toInt();
    unsigned int uiSignedBitVal = CTrack::GetSignedBitValueFromInt(iValue);
    bool bChecked = uiSignedBitVal & SURFACE_FLAG_APPLY_TEXTURE;
    int iBldIndex = CTrack::GetIntValueFromSignedBit(uiSignedBitVal & SURFACE_MASK_TEXTURE_INDEX);
    if (iBldIndex < pTex->GetNumTiles()) {
      QPixmap pixmap;
      pixmap.convertFromImage(QtHelpers::GetQImageFromTile(pTex->m_pTileAy[iBldIndex]));
      pPushButton->setIcon(pixmap);
      pPushButton->setText("");
    }
    pCheckbox->setEnabled(true);
    pCheckbox->setChecked(bChecked);
  }
  pCheckbox->blockSignals(false);
}

//-------------------------------------------------------------------------------------------------

void QtHelpers::UpdateTextures(QLabel *pLblTex1, QLabel *pLblTex2, CTexture *pTex, CPalette *pPal, int iSurface)
{
  QPixmap pixmap;
  unsigned int uiSignedBitVal = CTrack::GetSignedBitValueFromInt(iSurface);
  int iIndex = (int)(uiSignedBitVal & SURFACE_MASK_TEXTURE_INDEX);
  QSize size((int)(TILE_WIDTH * g_pMainWindow->GetDesktopScale() / 200.0),
             (int)(TILE_HEIGHT * g_pMainWindow->GetDesktopScale() / 200.0));
  if (pLblTex1) {
    pLblTex1->setMinimumSize(size);
    pLblTex1->setText("");
    pLblTex1->setPixmap(QPixmap());
  }
  if (pLblTex2) {
    pLblTex2->setMinimumSize(size);
    pLblTex2->setText("");
    pLblTex2->setPixmap(QPixmap());
  }
  if (iSurface != -1) {
    if (pTex && uiSignedBitVal & SURFACE_FLAG_APPLY_TEXTURE) {
      if (iIndex < pTex->GetNumTiles()) {
        pixmap.convertFromImage(QtHelpers::GetQImageFromTile(pTex->m_pTileAy[iIndex], true));
        if (pLblTex1)
          pLblTex1->setPixmap(pixmap);

        if (uiSignedBitVal & SURFACE_FLAG_TEXTURE_PAIR && iIndex > 0) {
          pixmap.convertFromImage(QtHelpers::GetQImageFromTile(pTex->m_pTileAy[iIndex + 1], true));
          if (pLblTex2)
            pLblTex2->setPixmap(pixmap);
        }
      }
    } else if (uiSignedBitVal & SURFACE_FLAG_TRANSPARENT) {
      if (iIndex < g_transparencyAyCount)
        pLblTex1->setText(g_transparencyAy[iIndex].c_str());
    } else {
      if (pPal && iIndex < PALETTE_SIZE) {
        QPixmap colorPixmap;
        colorPixmap.convertFromImage(QtHelpers::GetQImageFromColor(pPal->m_paletteAy[iIndex]));
        if (pLblTex1)
          pLblTex1->setPixmap(colorPixmap);
      }
    }
  }
}

//-------------------------------------------------------------------------------------------------

QImage QtHelpers::GetQImageFromTile(const tTile &tile, bool bScale)
{
  QImage image(TILE_WIDTH, TILE_HEIGHT, QImage::Format_RGB32);
  for (int i = 0; i < TILE_WIDTH; ++i) {
    for (int j = 0; j < TILE_HEIGHT; ++j)
      image.setPixelColor(i, j, QColor(tile.data[i][j].r,
                                       tile.data[i][j].g,
                                       tile.data[i][j].b));
  }
  if (bScale)
    return image.scaled((int)(TILE_WIDTH * g_pMainWindow->GetDesktopScale() / 200.0),
                        (int)(TILE_HEIGHT * g_pMainWindow->GetDesktopScale() / 200.0));
  else
    return image;
}

//-------------------------------------------------------------------------------------------------

QImage QtHelpers::GetQImageFromColor(const tPaletteColor &color, bool bScale)
{
  QImage image(TILE_WIDTH, TILE_HEIGHT, QImage::Format_RGB32);
  for (int i = 0; i < TILE_WIDTH; ++i) {
    for (int j = 0; j < TILE_HEIGHT; ++j)
      image.setPixelColor(i, j, QColor(color.r,
                                       color.g,
                                       color.b));
  }
  if (bScale)
    return image.scaled((int)(TILE_WIDTH * g_pMainWindow->GetDesktopScale() / 200.0),
                        (int)(TILE_HEIGHT * g_pMainWindow->GetDesktopScale() / 200.0));
  else
    return image;
}

//-------------------------------------------------------------------------------------------------
