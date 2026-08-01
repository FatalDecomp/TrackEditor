#ifndef _WHIPLIB_INDEXBUFFER_H
#define _WHIPLIB_INDEXBUFFER_H
//-------------------------------------------------------------------------------------------------
#include <GL/glew.h>
#include "Types.h"
//-------------------------------------------------------------------------------------------------

class CIndexBuffer
{
public:
  CIndexBuffer(const uint32 *pData, uint32 uiCount, GLenum usage = GL_STATIC_DRAW);
  ~CIndexBuffer();

  void Bind() const;
  void Unbind() const;
  inline uint32 GetCount() const { return m_uiCount; };
  void Update(const uint32 *pData, uint32 uiCount);

private:
  uint32 m_uiId;
  uint32 m_uiCount;
  GLenum m_usage;
};

//-------------------------------------------------------------------------------------------------
#endif
