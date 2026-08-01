#ifndef _TRACKEDITOR_QTUSERKEYMAPPER_H
#define _TRACKEDITOR_QTUSERKEYMAPPER_H
//-------------------------------------------------------------------------------------------------
#include "EditorCameraController.h"
#include <QSet>
//-------------------------------------------------------------------------------------------------
class QKeyEvent;
class QMouseEvent;
//-------------------------------------------------------------------------------------------------

class CQtUserKeyMapper
{
public:
  CQtUserKeyMapper();

  void QtMousePressEvent(QMouseEvent *pEvent);
  void QtMouseReleaseEvent(QMouseEvent *pEvent);
  void QtKeyPressEvent(QKeyEvent *pEvent);
  void QtKeyReleaseEvent(QKeyEvent *pEvent);
  tEditorCameraInput GetCameraInput() const;

private:
  QSet<int> m_PressedKeys;
  bool m_bMouseLook;
};

//-------------------------------------------------------------------------------------------------
#endif
