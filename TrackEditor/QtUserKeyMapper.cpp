#include "QtUserKeyMapper.h"
#include <Qt>
#include "qevent.h"
#include "qcursor.h"
//-------------------------------------------------------------------------------------------------
#if defined(_DEBUG) && defined(IS_WINDOWS)
#define new new(_CLIENT_BLOCK, __FILE__, __LINE__)
#endif
//-------------------------------------------------------------------------------------------------

CQtUserKeyMapper::CQtUserKeyMapper()
  : m_bMouseLook(false)
{ }

//-------------------------------------------------------------------------------------------------

void CQtUserKeyMapper::QtMousePressEvent(QMouseEvent *pEvent)
{
  m_bMouseLook = pEvent->buttons() != Qt::NoButton;
}

//-------------------------------------------------------------------------------------------------

void CQtUserKeyMapper::QtMouseReleaseEvent(QMouseEvent *pEvent)
{
  m_bMouseLook = pEvent->buttons() != Qt::NoButton;
}

//-------------------------------------------------------------------------------------------------

void CQtUserKeyMapper::QtKeyPressEvent(QKeyEvent *pEvent)
{
  if (!pEvent->isAutoRepeat())
    m_PressedKeys.insert(pEvent->key());
}

//-------------------------------------------------------------------------------------------------

void CQtUserKeyMapper::QtKeyReleaseEvent(QKeyEvent *pEvent)
{
  if (!pEvent->isAutoRepeat())
    m_PressedKeys.remove(pEvent->key());
}

//-------------------------------------------------------------------------------------------------

tEditorCameraInput CQtUserKeyMapper::GetCameraInput() const
{
  tEditorCameraInput Input;
  Input.bMoveForward = m_PressedKeys.contains(Qt::Key_W);
  Input.bMoveBackward = m_PressedKeys.contains(Qt::Key_S);
  Input.bStrafeLeft = m_PressedKeys.contains(Qt::Key_A);
  Input.bStrafeRight = m_PressedKeys.contains(Qt::Key_D);
  Input.bMoveUp = m_PressedKeys.contains(Qt::Key_R) ||
                  m_PressedKeys.contains(Qt::Key_E);
  Input.bMoveDown = m_PressedKeys.contains(Qt::Key_F) ||
                    m_PressedKeys.contains(Qt::Key_Q);
  Input.bMouseLook = m_bMouseLook;

  const QPoint Pos = QCursor::pos();
  Input.fMouseX = static_cast<float>(Pos.x());
  Input.fMouseY = static_cast<float>(Pos.y());
  return Input;
}

//-------------------------------------------------------------------------------------------------
