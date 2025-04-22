#include "ObjImporter.h"
//-------------------------------------------------------------------------------------------------
#if defined(_DEBUG) && defined(IS_WINDOWS)
#define new new(_CLIENT_BLOCK, __FILE__, __LINE__)
#endif
//-------------------------------------------------------------------------------------------------

CObjImporter &CObjImporter::GetObjImporter()
{
  static CObjImporter s_objImporter;

  return s_objImporter;
}

//-------------------------------------------------------------------------------------------------

CObjImporter::CObjImporter()
{
}

//-------------------------------------------------------------------------------------------------

CObjImporter::~CObjImporter()
{
}

//-------------------------------------------------------------------------------------------------

bool CObjImporter::ImportObj(const std::string &sFile, CShapeData **pShapeData, CShader *pShader, CTexture *pTexture)
{
  (void)(sFile); (void)(pShapeData); (void)(pShader); (void)(pTexture);
  return false;
}

//-------------------------------------------------------------------------------------------------