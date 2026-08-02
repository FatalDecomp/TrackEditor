#ifndef _TRACKEDITOR_QTHELPERS_H
#define _TRACKEDITOR_QTHELPERS_H
//-------------------------------------------------------------------------------------------------
#include "Palette.h"
#include "qimage.h"
//-------------------------------------------------------------------------------------------------
class QLineEdit;
class QString;
class QComboBox;
class QPushButton;
class QCheckBox;
class CTexture;
class CPalette;
class QWidget;
class QLabel;
struct tTile;
//-------------------------------------------------------------------------------------------------
#define BLOCK_SIG_AND_DO(obj, func){\
  obj->blockSignals(true);\
  obj->func;\
  obj->blockSignals(false);}
//-------------------------------------------------------------------------------------------------

namespace QtHelpers
{
  extern void UpdateSignButtonDisplay(QPushButton *pPushButton, QCheckBox *pCheckbox, QLineEdit *pLineEdit, CTexture *pTex);
  extern void UpdateTextures(QLabel *pLblTex1, QLabel *pLblTex2, CTexture *pTex, CPalette *pPal, int iSurface);
  extern QImage GetQImageFromTile(const tTile &tile, bool bScale = false);
  extern QImage GetQImageFromColor(const tPaletteColor &color, bool bScale = false);
};

//-------------------------------------------------------------------------------------------------
#endif
