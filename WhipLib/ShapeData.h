#ifndef _WHIPLIB_SHAPEDATA_H
#define _WHIPLIB_SHAPEDATA_H
//-------------------------------------------------------------------------------------------------
#include "glm.hpp"
#include "Vertex.h"
#include "Types.h"
//-------------------------------------------------------------------------------------------------
class CTexture;
//-------------------------------------------------------------------------------------------------
enum class eShapePrimitive
{
  TRIANGLES,
  LINES
};
//-------------------------------------------------------------------------------------------------
class CShapeData
{
public:
  CShapeData(tVertex *pVertices,
             uint32 uiNumVerts,
             uint32 *pIndices,
             uint32 uiNumIndices,
             CTexture *pTexture = NULL,
             eShapePrimitive drawType = eShapePrimitive::TRIANGLES);
  ~CShapeData();

  void ReplaceVertices(tVertex *pVertices, uint32 uiNumVerts);
  void ReplaceIndices(uint32 *pIndices, uint32 uiNumIndices);
  void ReplaceGeometry(tVertex *pVertices, uint32 uiNumVerts,
                       uint32 *pIndices, uint32 uiNumIndices);
  void TransformVertsForExport();
  void FlipTexCoordsForExport();

  glm::mat4 m_modelToWorldMatrix;
  CTexture *m_pTexture; //owned by track
  eShapePrimitive m_drawType;

  uint32 m_uiNumVerts;
  tVertex *m_vertices;
  uint32 m_uiNumIndices;
  uint32 *m_indices;
};
//-------------------------------------------------------------------------------------------------
#endif
