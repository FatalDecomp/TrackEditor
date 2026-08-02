#ifndef _WHIPLIB_OBJIMPORTER_H
#define _WHIPLIB_OBJIMPORTER_H
//-------------------------------------------------------------------------------------------------
#include <string>
//-------------------------------------------------------------------------------------------------
class CShapeData;
class CTexture;
//-------------------------------------------------------------------------------------------------

class CObjImporter
{
public:
  static CObjImporter &GetObjImporter();
  ~CObjImporter();
  CObjImporter(CObjImporter const &) = delete;
  void operator=(CObjImporter const &) = delete;

  //Texture coordinates come from the track texture atlas.
  bool ImportObj(const std::string &sFile, CShapeData **pShape, CTexture *pTexture);

private:
  CObjImporter();
};

//-------------------------------------------------------------------------------------------------
#endif
