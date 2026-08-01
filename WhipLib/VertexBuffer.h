#ifndef _WHIPLIB_VERTEXBUFFER_H
#define _WHIPLIB_VERTEXBUFFER_H
//-------------------------------------------------------------------------------------------------
#include <GL/glew.h>
#include "Types.h"
#include "Vertex.h"
//-------------------------------------------------------------------------------------------------

class CVertexBuffer
{
public:
  CVertexBuffer(const tVertex *pData, uint32 uiCount, GLenum usage = GL_STATIC_DRAW);
  ~CVertexBuffer();

  void Bind() const;
  void Unbind() const;
  void Update(const tVertex *pData, uint32 uiCount);

private:
  uint32 m_uiId;
  uint32 m_uiCount;
  GLenum m_usage;
};

//-------------------------------------------------------------------------------------------------
#endif
