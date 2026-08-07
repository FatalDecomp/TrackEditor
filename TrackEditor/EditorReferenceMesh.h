#ifndef TRACKEDITOR_EDITORREFERENCEMESH_H
#define TRACKEDITOR_EDITORREFERENCEMESH_H

#include "editor_api.h"

#include <cstdint>
#include <vector>

//-------------------------------------------------------------------------------------------------
// Owns the editor's reference mesh in the facade's own AD-13 shape (E3A-S7).
//
// The core copies vertex and texture data during RollerEd_SetReferenceMesh and
// retains nothing, so the arrays here have to outlive only the call. They do
// not outlive the UI, though: the render worker runs on another thread, so the
// request that carries a mesh deep-copies it at enqueue like every other
// pointer payload (AD-16). GetMesh() therefore points into this object and is
// only valid while it is.
//
// This class owns no Qt type and calls no RollerEd_* function, so the
// conversion is unit-testable without a worker or an event loop -- the same
// boundary CEditorOverlaySettings keeps.
//
// Coordinates: the dialog's X/Y/Z and yaw/pitch/roll are ROLLER world values.
// The Y-up WhipLib renderer those numbers were once typed against is deleted,
// and the user now lines the model up against what the ROLLER preview draws,
// so the axes are ADR 0003's: world +Z is up, yaw turns about Z, pitch tilts
// about Y, roll banks about X.
//-------------------------------------------------------------------------------------------------

class CEditorReferenceMesh
{
public:
  CEditorReferenceMesh();

  // Replaces the geometry. An empty vertex list clears the mesh, which is how
  // AD-13 says "draw nothing". puiIndices may be null for a plain triangle
  // list, in which case the core synthesizes the indices.
  void SetGeometry(const tEdReferenceVertex *pVertices, size_t uiVertexCount,
                   const uint32_t *puiIndices, size_t uiIndexCount);
  void Clear();

  // The UpdateReferenceModelPos(yaw, pitch, roll, x, y, z, scale) septuple,
  // unchanged. Scale is uniform, as the dialog's single spin box implies.
  void SetTransform(double dYaw, double dPitch, double dRoll,
                    int iX, int iY, int iZ, double dScale);
  void SetWireframe(bool bWireframe);

  bool HasMesh() const { return !m_Vertices.empty(); }
  // E4-S6. Whether the imported file actually supplied normals. An OBJ with
  // no vn lines leaves them zero, and claiming HAS_NORMALS over zeros is worse
  // than not claiming it: AD-13 says the core generates them when the flag is
  // clear, so a model without normals shades correctly only if we say so.
  bool HasNormals() const { return m_bHasNormals; }
  size_t VertexCount() const { return m_Vertices.size(); }
  size_t IndexCount() const { return m_Indices.size(); }

  // A tEdReferenceMesh pointing at this object's storage, ready to hand to
  // RollerEd_SetReferenceMesh. Valid while this object is unmodified.
  tEdReferenceMesh GetMesh() const;

  const std::vector<tEdReferenceVertex> &Vertices() const { return m_Vertices; }
  const std::vector<uint32_t> &Indices() const { return m_Indices; }

private:
  std::vector<tEdReferenceVertex> m_Vertices;
  std::vector<uint32_t> m_Indices;
  double m_dYaw;
  double m_dPitch;
  double m_dRoll;
  int m_iX;
  int m_iY;
  int m_iZ;
  double m_dScale;
  bool m_bWireframe;
  bool m_bHasNormals;
};

#endif
