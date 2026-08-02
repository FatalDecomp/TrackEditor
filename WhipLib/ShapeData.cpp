#include "ShapeData.h"
//-------------------------------------------------------------------------------------------------
#if defined(_DEBUG) && defined(IS_WINDOWS)
#define new new(_CLIENT_BLOCK, __FILE__, __LINE__)
#endif
//-------------------------------------------------------------------------------------------------

CShapeData::CShapeData(tVertex *pVertices,
                       uint32 uiNumVerts,
                       uint32 *pIndices,
                       uint32 uiNumIndices,
                       CTexture *pTexture,
                       eShapePrimitive drawType)
  : m_modelToWorldMatrix(glm::mat4(1))
  , m_pTexture(pTexture)
  , m_drawType(drawType)
  , m_uiNumVerts(uiNumVerts)
  , m_vertices(pVertices)
  , m_uiNumIndices(uiNumIndices)
  , m_indices(pIndices)
{
}

//-------------------------------------------------------------------------------------------------

CShapeData::~CShapeData()
{
  if (m_vertices) {
    delete[] m_vertices;
    m_vertices = NULL;
  }
  if (m_indices) {
    delete[] m_indices;
    m_indices = NULL;
  }
}

//-------------------------------------------------------------------------------------------------

void CShapeData::ReplaceVertices(tVertex *pVertices, uint32 uiNumVerts)
{
  if (m_vertices != pVertices) {
    delete[] m_vertices;
    m_vertices = pVertices;
  }
  m_uiNumVerts = uiNumVerts;
}

//-------------------------------------------------------------------------------------------------

void CShapeData::ReplaceIndices(uint32 *pIndices, uint32 uiNumIndices)
{
  if (m_indices != pIndices) {
    delete[] m_indices;
    m_indices = pIndices;
  }
  m_uiNumIndices = uiNumIndices;
}

//-------------------------------------------------------------------------------------------------

void CShapeData::ReplaceGeometry(tVertex *pVertices, uint32 uiNumVerts,
                                 uint32 *pIndices, uint32 uiNumIndices)
{
  ReplaceVertices(pVertices, uiNumVerts);
  ReplaceIndices(pIndices, uiNumIndices);
}

//-------------------------------------------------------------------------------------------------

void CShapeData::TransformVertsForExport()
{
  for (uint32 i = 0; i < m_uiNumVerts; ++i) {
    m_vertices[i].position = glm::vec3(m_modelToWorldMatrix * glm::vec4(m_vertices[i].position, 1.0f));
  }
}

//-------------------------------------------------------------------------------------------------

void CShapeData::FlipTexCoordsForExport()
{
  //The exported image rows use the opposite vertical origin from model UVs.
  for (uint32 i = 0; i < m_uiNumVerts; ++i) {
    m_vertices[i].texCoords.y = 1.0f - m_vertices[i].texCoords.y;
  }
}

//-------------------------------------------------------------------------------------------------
