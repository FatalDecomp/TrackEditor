#include "EditorCameraController.h"

#include <cassert>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

#ifdef assert
#undef assert
#endif
#define assert(condition)                                                      \
  do {                                                                         \
    if (!(condition)) {                                                        \
      std::cerr << "assertion failed: " #condition << " (" << __FILE__       \
                << ':' << __LINE__ << ")\n";                                  \
      std::abort();                                                            \
    }                                                                          \
  } while (false)

namespace
{
bool NearlyEqual(float fA, float fB, float fTolerance = 0.01f)
{
  return std::fabs(fA - fB) <= fTolerance;
}

void TestFacadeStateAndWorldAxisMovement()
{
  CEditorCameraController::SetMovementSpeed(
      CEditorCameraController::DEFAULT_MOVEMENT_SPEED);
  CEditorCameraController Camera;
  Camera.SetPosition(0.0f, 0.0f, 0.0f);
  Camera.SetOrientation(0.0f, 0.0f);

  const tEdCameraState &Initial = Camera.GetCameraState();
  assert(Initial.uiStructSize == sizeof(tEdCameraState));
  assert(Initial.uiVersion == ROLLER_ED_CAMERA_STATE_VERSION);

  tEditorCameraInput Input;
  Input.bMoveForward = true;
  assert(Camera.Update(Input, 0.5f));
  assert(NearlyEqual(Camera.GetCameraState().fPosition[0], 15000.0f));
  assert(NearlyEqual(Camera.GetCameraState().fPosition[1], 0.0f));
  assert(NearlyEqual(Camera.GetCameraState().fPosition[2], 0.0f));

  Camera.SetPosition(0.0f, 0.0f, 0.0f);
  CEditorCameraController::SetMovementSpeed(4000.0f);
  Input = {};
  Input.bMoveForward = true;
  assert(Camera.Update(Input, 0.5f));
  assert(NearlyEqual(Camera.GetCameraState().fPosition[0], 2000.0f));
  CEditorCameraController::SetMovementSpeed(
      CEditorCameraController::DEFAULT_MOVEMENT_SPEED);

  Input = {};
  Input.bStrafeRight = true;
  assert(Camera.Update(Input, 0.25f));
  assert(NearlyEqual(Camera.GetCameraState().fPosition[1], -7500.0f));

  Input = {};
  Input.bMoveUp = true;
  assert(Camera.Update(Input, 0.1f));
  assert(NearlyEqual(Camera.GetCameraState().fPosition[2], 3000.0f));

  Input = {};
  Input.bMoveDown = true;
  assert(Camera.Update(Input, 0.1f));
  assert(NearlyEqual(Camera.GetCameraState().fPosition[2], 0.0f));
}

void TestMouseLookSensitivityAndClickGate()
{
  CEditorCameraController Camera;
  Camera.SetOrientation(0.0f, 0.0f);

  tEditorCameraInput Input;
  Input.fMouseX = 100.0f;
  Input.fMouseY = 100.0f;
  Input.bMouseLook = true;
  assert(!Camera.Update(Input, 0.0f));

  Input.fMouseX = 110.0f;
  Input.fMouseY = 105.0f;
  Input.bMouseLook = false;
  assert(!Camera.Update(Input, 0.0f));
  assert(NearlyEqual(Camera.GetCameraState().fYawDegrees, 0.0f));
  assert(NearlyEqual(Camera.GetCameraState().fPitchDegrees, 0.0f));

  Input.fMouseX = 120.0f;
  Input.fMouseY = 100.0f;
  Input.bMouseLook = true;
  assert(Camera.Update(Input, 0.0f));
  assert(NearlyEqual(Camera.GetCameraState().fYawDegrees, -3.0f));
  assert(NearlyEqual(Camera.GetCameraState().fPitchDegrees, 1.5f));

  Input.fMouseX = 220.0f;
  Input.fMouseY = 100.0f;
  assert(!Camera.Update(Input, 0.0f));
  assert(NearlyEqual(Camera.GetCameraState().fYawDegrees, -3.0f));
}

void TestMovementFollowsYawAndPitch()
{
  CEditorCameraController Camera;
  Camera.SetPosition(0.0f, 0.0f, 0.0f);
  Camera.SetOrientation(90.0f, 30.0f);

  tEditorCameraInput Input;
  Input.bMoveForward = true;
  assert(Camera.Update(Input, 1.0f));
  const tEdCameraState &State = Camera.GetCameraState();
  assert(NearlyEqual(State.fPosition[0], 0.0f));
  assert(NearlyEqual(State.fPosition[1], 25980.76f, 0.1f));
  assert(NearlyEqual(State.fPosition[2], 15000.0f, 0.1f));

  Camera.SetOrientation(725.0f, -725.0f);
  assert(NearlyEqual(Camera.GetCameraState().fYawDegrees, 5.0f));
  assert(NearlyEqual(Camera.GetCameraState().fPitchDegrees, -5.0f));
}

void TestLookAtOrientationUsesEditorDegrees()
{
  const float Position[3] = { 1.0f, 2.0f, 3.0f };
  float fYaw = -999.0f;
  float fPitch = -999.0f;

  const float AlongX[3] = { 5.0f, 2.0f, 3.0f };
  assert(CEditorCameraController::CalculateLookAtOrientation(
      Position, AlongX, fYaw, fPitch));
  assert(NearlyEqual(fYaw, 0.0f));
  assert(NearlyEqual(fPitch, 0.0f));

  const float AlongY[3] = { 1.0f, 7.0f, 3.0f };
  assert(CEditorCameraController::CalculateLookAtOrientation(
      Position, AlongY, fYaw, fPitch));
  assert(NearlyEqual(fYaw, 90.0f));
  assert(NearlyEqual(fPitch, 0.0f));

  const float DiagonalUp[3] = { 4.0f, 6.0f, 8.0f };
  assert(CEditorCameraController::CalculateLookAtOrientation(
      Position, DiagonalUp, fYaw, fPitch));
  assert(NearlyEqual(fYaw, 53.1301f));
  assert(NearlyEqual(fPitch, 45.0f));

  const float Vertical[3] = { 1.0f, 2.0f, 13.0f };
  assert(CEditorCameraController::CalculateLookAtOrientation(
      Position, Vertical, fYaw, fPitch));
  assert(NearlyEqual(fYaw, 0.0f));
  assert(NearlyEqual(fPitch, 90.0f));

  fYaw = 17.0f;
  fPitch = 23.0f;
  assert(!CEditorCameraController::CalculateLookAtOrientation(
      Position, Position, fYaw, fPitch));
  assert(NearlyEqual(fYaw, 17.0f));
  assert(NearlyEqual(fPitch, 23.0f));

  const float Invalid[3] = {
    std::numeric_limits<float>::quiet_NaN(), 2.0f, 3.0f
  };
  assert(!CEditorCameraController::CalculateLookAtOrientation(
      Position, Invalid, fYaw, fPitch));
}

}

int main()
{
  TestFacadeStateAndWorldAxisMovement();
  TestMouseLookSensitivityAndClickGate();
  TestMovementFollowsYawAndPitch();
  TestLookAtOrientationUsesEditorDegrees();
  CEditorCameraController::SetMovementSpeed(
      CEditorCameraController::DEFAULT_MOVEMENT_SPEED);
  std::cout << "E3-S3 editor camera input tests passed\n";
  return 0;
}
