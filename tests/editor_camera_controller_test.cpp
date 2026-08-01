#include "EditorCameraController.h"
#include "TrackCoordinateConversion.h"

#include <cassert>
#include <cmath>
#include <cstdlib>
#include <iostream>

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

void TestTrackCoordinateConversion()
{
  const glm::vec3 ModelPosition(100.0f, 200.0f, 300.0f);
  const glm::vec3 RollerOrigin(1000.0f, 2000.0f, 2048.0f);
  const glm::vec3 RollerPosition =
      EditorTrackCoordinates::ToRollerWorld(ModelPosition, RollerOrigin);

  assert(NearlyEqual(RollerPosition.x, 1300.0f));
  assert(NearlyEqual(RollerPosition.y, 2100.0f));
  assert(NearlyEqual(RollerPosition.z, 2248.0f));
}
}

int main()
{
  TestFacadeStateAndWorldAxisMovement();
  TestMouseLookSensitivityAndClickGate();
  TestMovementFollowsYawAndPitch();
  TestTrackCoordinateConversion();
  CEditorCameraController::SetMovementSpeed(
      CEditorCameraController::DEFAULT_MOVEMENT_SPEED);
  std::cout << "E3-S3 editor camera input tests passed\n";
  return 0;
}
