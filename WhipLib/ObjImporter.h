#ifndef _WHIPLIB_OBJIMPORTER_H
#define _WHIPLIB_OBJIMPORTER_H
//-------------------------------------------------------------------------------------------------
#include <string>
//-------------------------------------------------------------------------------------------------
class CShapeData;
class CShader;
class CTexture;
//-------------------------------------------------------------------------------------------------

class CObjImporter
{
public:
  static CObjImporter &GetObjImporter();
  ~CObjImporter();
  CObjImporter(CObjImporter const &) = delete;
  void operator=(CObjImporter const &) = delete;

  //shader and texture come from track
  bool ImportObj(const std::string &sFile, CShapeData **pShapeData, CShader *pShader, CTexture *pTexture);

private:
  CObjImporter();
};

//-------------------------------------------------------------------------------------------------
#endif