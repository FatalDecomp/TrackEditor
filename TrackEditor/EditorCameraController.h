#ifndef TRACKEDITOR_EDITORCAMERACONTROLLER_H
#define TRACKEDITOR_EDITORCAMERACONTROLLER_H

#include "editor_api.h"

struct tEditorCameraInput
{
  bool bMoveForward = false;
  bool bMoveBackward = false;
  bool bStrafeLeft = false;
  bool bStrafeRight = false;
  bool bMoveUp = false;
  bool bMoveDown = false;
  bool bMouseLook = false;
  float fMouseX = 0.0f;
  float fMouseY = 0.0f;
};

class CEditorCameraController
{
public:
  static constexpr float DEFAULT_MOVEMENT_SPEED = 30000.0f;
  static constexpr float MOUSE_SENSITIVITY = 0.3f;
  static constexpr float MAX_MOUSE_DELTA = 50.0f;

  CEditorCameraController();

  bool Update(const tEditorCameraInput &Input, float fDeltaSeconds);
  void SetPosition(float fX, float fY, float fZ);
  void SetOrientation(float fYawDegrees, float fPitchDegrees);
  void ResetMouseTracking();
  static void SetMovementSpeed(float fMovementSpeed);
  static float GetMovementSpeed() { return s_fMovementSpeed; }

  const tEdCameraState &GetCameraState() const { return m_Camera; }

private:
  static float WrapDegrees(float fDegrees);

  tEdCameraState m_Camera;
  float m_fPreviousMouseX;
  float m_fPreviousMouseY;
  bool m_bHasPreviousMousePosition;
  static float s_fMovementSpeed;
};

#endif
