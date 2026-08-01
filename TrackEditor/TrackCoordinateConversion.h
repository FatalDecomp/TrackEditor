#ifndef TRACKEDITOR_TRACKCOORDINATECONVERSION_H
#define TRACKEDITOR_TRACKCOORDINATECONVERSION_H

#include <glm.hpp>

namespace EditorTrackCoordinates
{
/* WhipLib models tracks as X-right, Y-up, Z-forward from a local origin.
 * ROLLER renders the same track as Y-right, Z-up, X-forward and retains the
 * three initial world coordinates stored in the track header. */
inline glm::vec3 ToRollerWorld(const glm::vec3 &ModelPosition,
                               const glm::vec3 &RollerOrigin)
{
  return glm::vec3(RollerOrigin.x + ModelPosition.z,
                   RollerOrigin.y + ModelPosition.x,
                   RollerOrigin.z + ModelPosition.y);
}
}

#endif
