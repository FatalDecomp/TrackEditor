#include <GL/glew.h>
#include "OpenGLDebug.h"
#include "ShapeData.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "VertexArray.h"
#include "Shader.h"
#include "Texture.h"
//-------------------------------------------------------------------------------------------------
#if defined(_DEBUG) && defined(IS_WINDOWS)
#define new new(_CLIENT_BLOCK, __FILE__, __LINE__)
#endif
//-------------------------------------------------------------------------------------------------

CShapeData::CShapeData(CVertexBuffer *pVertexBuf,
                       CIndexBuffer *pIndexBuf,
                       CVertexArray *pVertexArray,
                       CShader *pShader,
                       CTexture *pTexture,
                       GLenum drawType)
  : m_pVertexBuf(pVertexBuf)
  , m_pIndexBuf(pIndexBuf)
  , m_pVertexArray(pVertexArray)
  , m_pShader(pShader)
  , m_pTexture(pTexture)
  , m_drawType(drawType)
  , m_modelToWorldMatrix(glm::mat4(1))
  , m_vertices(NULL)
  , m_uiNumVerts(0)
  , m_indices(NULL)
  , m_uiNumIndices(0)
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
  if (m_pVertexBuf) {
    delete m_pVertexBuf;
    m_pVertexBuf = NULL;
  }
  if (m_pIndexBuf) {
    delete m_pIndexBuf;
    m_pIndexBuf = NULL;
  }
  if (m_pVertexArray) {
    delete m_pVertexArray;
    m_pVertexArray = NULL;
  }
}

//-------------------------------------------------------------------------------------------------

void CShapeData::Draw(const glm::mat4 &worldToProjectionMatrix, const glm::vec3 cameraPosition)
{
  if (!m_pShader || !m_pVertexArray || !m_pIndexBuf)
    return;

  m_pShader->Bind();
  m_pVertexArray->Bind();
  m_pIndexBuf->Bind();
  if (m_pTexture) {
    m_pTexture->Bind();
    m_pShader->SetUniform1i("textureSlot", 0);
  }

  glm::mat4 fullTransformMatrix = worldToProjectionMatrix * m_modelToWorldMatrix;
  m_pShader->SetUniformMat4("modelToProjectionMatrix", fullTransformMatrix);
  m_pShader->SetUniformMat4("modelToWorldMatrix", m_modelToWorldMatrix);
  m_pShader->SetUniformVec3("eyePositionWorld", cameraPosition);
  GLCALL(glDrawElements(m_drawType, m_pIndexBuf->GetCount(), GL_UNSIGNED_INT, 0));
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
  //OpenGL expects texture data in reverse order
  for (uint32 i = 0; i < m_uiNumVerts; ++i) {
    m_vertices[i].texCoords.y = 1.0f - m_vertices[i].texCoords.y;
  }
}

//-------------------------------------------------------------------------------------------------
