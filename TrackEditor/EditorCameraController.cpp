#include "EditorCameraController.h"

#include <cmath>

namespace
{
constexpr float PI = 3.14159265358979323846f;

float DegreesToRadians(float fDegrees)
{
  return fDegrees * (PI / 180.0f);
}
}

float CEditorCameraController::s_fMovementSpeed =
    CEditorCameraController::DEFAULT_MOVEMENT_SPEED;

CEditorCameraController::CEditorCameraController()
  : m_Camera{}
  , m_fPreviousMouseX(0.0f)
  , m_fPreviousMouseY(0.0f)
  , m_bHasPreviousMousePosition(false)
{
  m_Camera.uiStructSize = sizeof(m_Camera);
  m_Camera.uiVersion = ROLLER_ED_CAMERA_STATE_VERSION;
  SetPosition(-4000.0f, 0.0f, 1600.0f);
  SetOrientation(0.0f, -25.0f);
}

float CEditorCameraController::WrapDegrees(float fDegrees)
{
  float fWrapped = std::fmod(fDegrees, 360.0f);
  if (fWrapped <= -180.0f)
    fWrapped += 360.0f;
  else if (fWrapped > 180.0f)
    fWrapped -= 360.0f;
  return fWrapped;
}

bool CEditorCameraController::Update(const tEditorCameraInput &Input,
                                     float fDeltaSeconds)
{
  bool bChanged = false;

  if (std::isfinite(fDeltaSeconds) && fDeltaSeconds > 0.0f) {
    const float fYawRadians = DegreesToRadians(m_Camera.fYawDegrees);
    const float fPitchRadians = DegreesToRadians(m_Camera.fPitchDegrees);
    const float fCosPitch = std::cos(fPitchRadians);
    const float fForwardX = std::cos(fYawRadians) * fCosPitch;
    const float fForwardY = std::sin(fYawRadians) * fCosPitch;
    const float fForwardZ = std::sin(fPitchRadians);
    const float fRightX = std::sin(fYawRadians) * fCosPitch;
    const float fRightY = -std::cos(fYawRadians) * fCosPitch;
    const float fForwardAxis = static_cast<float>(Input.bMoveForward)
        - static_cast<float>(Input.bMoveBackward);
    const float fStrafeAxis = static_cast<float>(Input.bStrafeRight)
        - static_cast<float>(Input.bStrafeLeft);
    const float fVerticalAxis = static_cast<float>(Input.bMoveUp)
        - static_cast<float>(Input.bMoveDown);
    const float fDistance = s_fMovementSpeed * fDeltaSeconds;
    const float fDeltaX = (fForwardX * fForwardAxis
                           + fRightX * fStrafeAxis) * fDistance;
    const float fDeltaY = (fForwardY * fForwardAxis
                           + fRightY * fStrafeAxis) * fDistance;
    const float fDeltaZ = (fForwardZ * fForwardAxis
                           + fVerticalAxis) * fDistance;

    if (fDeltaX != 0.0f || fDeltaY != 0.0f || fDeltaZ != 0.0f) {
      m_Camera.fPosition[0] += fDeltaX;
      m_Camera.fPosition[1] += fDeltaY;
      m_Camera.fPosition[2] += fDeltaZ;
      bChanged = true;
    }
  }

  if (!std::isfinite(Input.fMouseX) || !std::isfinite(Input.fMouseY)) {
    ResetMouseTracking();
    return bChanged;
  }

  if (!m_bHasPreviousMousePosition) {
    m_fPreviousMouseX = Input.fMouseX;
    m_fPreviousMouseY = Input.fMouseY;
    m_bHasPreviousMousePosition = true;
    return bChanged;
  }

  const float fMouseDeltaX = Input.fMouseX - m_fPreviousMouseX;
  const float fMouseDeltaY = Input.fMouseY - m_fPreviousMouseY;
  m_fPreviousMouseX = Input.fMouseX;
  m_fPreviousMouseY = Input.fMouseY;

  if (Input.bMouseLook
      && std::hypot(fMouseDeltaX, fMouseDeltaY) <= MAX_MOUSE_DELTA
      && (fMouseDeltaX != 0.0f || fMouseDeltaY != 0.0f)) {
    // Positive screen X is a turn to the right. ROLLER yaw increases to the
    // left from +X, and positive pitch looks upward.
    m_Camera.fYawDegrees = WrapDegrees(
        m_Camera.fYawDegrees - fMouseDeltaX * MOUSE_SENSITIVITY);
    m_Camera.fPitchDegrees = WrapDegrees(
        m_Camera.fPitchDegrees - fMouseDeltaY * MOUSE_SENSITIVITY);
    bChanged = true;
  }

  return bChanged;
}

void CEditorCameraController::SetPosition(float fX, float fY, float fZ)
{
  m_Camera.fPosition[0] = fX;
  m_Camera.fPosition[1] = fY;
  m_Camera.fPosition[2] = fZ;
}

void CEditorCameraController::SetOrientation(float fYawDegrees,
                                             float fPitchDegrees)
{
  m_Camera.fYawDegrees = WrapDegrees(fYawDegrees);
  m_Camera.fPitchDegrees = WrapDegrees(fPitchDegrees);
}

void CEditorCameraController::ResetMouseTracking()
{
  m_bHasPreviousMousePosition = false;
}

void CEditorCameraController::SetMovementSpeed(float fMovementSpeed)
{
  if (std::isfinite(fMovementSpeed) && fMovementSpeed > 0.0f)
    s_fMovementSpeed = fMovementSpeed;
}
