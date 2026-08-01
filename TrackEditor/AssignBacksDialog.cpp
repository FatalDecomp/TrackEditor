#include "TrackEditor.h"
#include "AssignBacksDialog.h"
#include "qdesktopwidget.h"
#include "Track.h"
#include "Texture.h"
#include "BackWidget.h"
//-------------------------------------------------------------------------------------------------
#if defined(_DEBUG) && defined(IS_WINDOWS)
  #define new new(_CLIENT_BLOCK, __FILE__, __LINE__)
#endif
//-------------------------------------------------------------------------------------------------

CAssignBacksDialog::CAssignBacksDialog(QWidget *pParent, CTrack *pTrack)
  : QDialog(pParent)
  , m_pTrack(pTrack)
{
  setupUi(this);
  resize(QDesktopWidget().availableGeometry(this).size().width() * 0.1, QDesktopWidget().availableGeometry(this).size().height() * 0.8);
  connect(pbCancel, &QPushButton::clicked, this, &CAssignBacksDialog::reject);
  connect(pbApply, &QPushButton::clicked, this, &CAssignBacksDialog::OnApplyClicked);
  
  if (pTrack && pTrack->m_assets.GetMainTexture()) {
    lblNotLoaded->setVisible(pTrack->m_assets.GetMainTexture()->GetNumTiles() == 0);
    int iButtonSize = 90;
    int iTilesPerLine = (width() - 256) / (iButtonSize + 6);
    int i = 0;
    for (; i < pTrack->m_assets.GetMainTexture()->GetNumTiles(); ++i) {
      int iBack = -1;
      CSignMap::iterator it = pTrack->m_backsMap.find(i);
      if (it != pTrack->m_backsMap.end())
        iBack = it->second;

      CBackWidget *pBackWidget = new CBackWidget(this, i, pTrack->m_assets.GetMainTexture(), pTrack->m_assets.GetPalette(), iBack);
      backsLayout->addWidget(pBackWidget);
      m_backWidgetAy.push_back(pBackWidget); //don't need to delete these, they will be cleaned up by the layout
    }
  }
}

//-------------------------------------------------------------------------------------------------

CAssignBacksDialog::~CAssignBacksDialog()
{

}

//-------------------------------------------------------------------------------------------------

void CAssignBacksDialog::OnApplyClicked()
{
  m_pTrack->m_backsMap.clear();
  for (CBackWidgetAy::iterator it = m_backWidgetAy.begin(); it != m_backWidgetAy.end(); ++it) {
    m_pTrack->m_backsMap[(*it)->GetIndex()] = (*it)->GetBack();
  }

  accept();
}

//-------------------------------------------------------------------------------------------------