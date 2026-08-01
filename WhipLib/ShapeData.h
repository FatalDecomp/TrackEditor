#ifndef _WHIPLIB_SHAPEDATA_H
#define _WHIPLIB_SHAPEDATA_H
//-------------------------------------------------------------------------------------------------
#include <GL/glew.h>
#include "glm.hpp"
#include "Vertex.h"
#include "Types.h"
//-------------------------------------------------------------------------------------------------
class CShader;
class CVertexBuffer;
class CIndexBuffer;
class CVertexArray;
class CTexture;
//-------------------------------------------------------------------------------------------------
class CShapeData
{
public:
  CShapeData(CVertexBuffer *pVertexBuf,
             CIndexBuffer *pIndexBuf,
             CVertexArray *pVertexArray,
             CShader *pShader,
             CTexture *pTexture = NULL,
             GLenum drawType = GL_TRIANGLES);
  ~CShapeData();

  void Draw(const glm::mat4 &worldToProjectionMatrix, const glm::vec3 cameraPosition);
  void TransformVertsForExport();
  void FlipTexCoordsForExport();

  glm::mat4 m_modelToWorldMatrix;
  CIndexBuffer *m_pIndexBuf;
  CVertexBuffer *m_pVertexBuf;
  CTexture *m_pTexture; //owned by track or renderer
  GLenum m_drawType;

  uint32 m_uiNumVerts;
  tVertex *m_vertices;
  uint32 m_uiNumIndices;
  uint32 *m_indices;

private:
  CVertexArray *m_pVertexArray;
  CShader *m_pShader; //owned by renderer
};
//-------------------------------------------------------------------------------------------------
#endif
