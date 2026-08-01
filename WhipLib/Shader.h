#ifndef _WHIPLIB_SHADER_H
#define _WHIPLIB_SHADER_H
//-------------------------------------------------------------------------------------------------
#include "Types.h"
#include <GL/glew.h>
#include "glm.hpp"
#include <string>
#include <unordered_map>
//-------------------------------------------------------------------------------------------------

class CShader
{
public:
  CShader(const std::string &sVertexFile, const std::string &sFragmentFile);
  ~CShader();

  void Bind() const;
  void Unbind() const;
  bool SetUniform1i(const std::string &sName, int iValue);
  bool SetUniformVec3(const std::string &sName, const glm::vec3 &vec);
  bool SetUniformVec4(const std::string &sName, const glm::vec4 &vec);
  bool SetUniformMat4(const std::string &sName, const glm::mat4 &matrix);

private:
  int GetUniformLocation(const std::string &sName);
  bool CheckStatus(GLuint objectId,
                   PFNGLGETSHADERIVPROC objectPropertyGetter,
                   PFNGLGETSHADERINFOLOGPROC getInfoLogFunc,
                   GLenum statusType);
  bool CheckShaderStatus(GLuint shaderId);
  bool CheckProgramStatus(GLuint programId);
  std::string ReadShaderCode(const char *szFile);
  void InstallShaders(GLuint &programId, const char *szVertexShader, const char *szFragmentShader);

  uint32 m_uiId;
  std::unordered_map<std::string, int> m_locationCache;
};

//-------------------------------------------------------------------------------------------------
#endif
