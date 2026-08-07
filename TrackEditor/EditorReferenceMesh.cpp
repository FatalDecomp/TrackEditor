#include "EditorReferenceMesh.h"

#include <cstring>

//-------------------------------------------------------------------------------------------------

CEditorReferenceMesh::CEditorReferenceMesh()
  : m_Vertices()
  , m_Indices()
  , m_dYaw(0.0)
  , m_dPitch(0.0)
  , m_dRoll(0.0)
  , m_iX(0)
  , m_iY(0)
  , m_iZ(0)
  , m_dScale(1.0)
  , m_bWireframe(false)
{
}

//-------------------------------------------------------------------------------------------------

void CEditorReferenceMesh::SetGeometry(const tEdReferenceVertex *pVertices,
                                       size_t uiVertexCount,
                                       const uint32_t *puiIndices,
                                       size_t uiIndexCount)
{
  if (!pVertices || uiVertexCount == 0) {
    Clear();
    return;
  }

  m_Vertices.assign(pVertices, pVertices + uiVertexCount);
  if (puiIndices && uiIndexCount > 0)
    m_Indices.assign(puiIndices, puiIndices + uiIndexCount);
  else
    m_Indices.clear();
}

//-------------------------------------------------------------------------------------------------

void CEditorReferenceMesh::Clear()
{
  m_Vertices.clear();
  m_Indices.clear();
}

//-------------------------------------------------------------------------------------------------

void CEditorReferenceMesh::SetTransform(double dYaw, double dPitch, double dRoll,
                                        int iX, int iY, int iZ, double dScale)
{
  m_dYaw = dYaw;
  m_dPitch = dPitch;
  m_dRoll = dRoll;
  m_iX = iX;
  m_iY = iY;
  m_iZ = iZ;
  m_dScale = dScale;
}

//-------------------------------------------------------------------------------------------------

void CEditorReferenceMesh::SetWireframe(bool bWireframe)
{
  m_bWireframe = bWireframe;
}

//-------------------------------------------------------------------------------------------------

tEdReferenceMesh CEditorReferenceMesh::GetMesh() const
{
  tEdReferenceMesh Mesh;

  std::memset(&Mesh, 0, sizeof(Mesh));
  Mesh.uiStructSize = sizeof(Mesh);
  Mesh.uiVersion = ROLLER_ED_REFERENCE_MESH_VERSION;
  if (!m_Vertices.empty()) {
    Mesh.pVertices = m_Vertices.data();
    Mesh.uiVertexCount = static_cast<uint32_t>(m_Vertices.size());
  }
  if (!m_Indices.empty()) {
    Mesh.puiIndices = m_Indices.data();
    Mesh.uiIndexCount = static_cast<uint32_t>(m_Indices.size());
  }
  Mesh.fPosition[0] = static_cast<float>(m_iX);
  Mesh.fPosition[1] = static_cast<float>(m_iY);
  Mesh.fPosition[2] = static_cast<float>(m_iZ);
  Mesh.fRotation[0] = static_cast<float>(m_dYaw);
  Mesh.fRotation[1] = static_cast<float>(m_dPitch);
  Mesh.fRotation[2] = static_cast<float>(m_dRoll);
  // The dialog offers one scale spin box, so the mesh scales uniformly. A zero
  // would collapse it to nothing and read as a bug rather than a setting.
  Mesh.fScale[0] = m_dScale != 0.0 ? static_cast<float>(m_dScale) : 1.0f;
  Mesh.fScale[1] = Mesh.fScale[0];
  Mesh.fScale[2] = Mesh.fScale[0];
  // The importer supplies per-vertex normals, so the core does not regenerate
  // them. Wireframe is the host's choice, not a property of the file.
  Mesh.uiFlags = ROLLER_ED_REFERENCE_HAS_NORMALS;
  if (m_bWireframe)
    Mesh.uiFlags |= ROLLER_ED_REFERENCE_WIREFRAME;
  return Mesh;
}
