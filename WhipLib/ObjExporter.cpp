#include "ObjExporter.h"
#include "ShapeData.h"
#include "Logging.h"
#include <fstream>
#include <sstream>
//-------------------------------------------------------------------------------------------------
#if defined(_DEBUG) && defined(IS_WINDOWS)
#define new new(_CLIENT_BLOCK, __FILE__, __LINE__)
#endif
//-------------------------------------------------------------------------------------------------

CObjExporter &CObjExporter::GetObjExporter()
{
  static CObjExporter s_objExporter;

  return s_objExporter;
}

//-------------------------------------------------------------------------------------------------

CObjExporter::CObjExporter()
{
}

//-------------------------------------------------------------------------------------------------

CObjExporter::~CObjExporter()
{
}

//-------------------------------------------------------------------------------------------------

bool CObjExporter::ExportMaterial(const std::string &sFile, const std::vector<std::string> &texAy)
{
  //open output file
  std::ofstream out(sFile);
  if (!out.is_open()) {
    Logging::LogMessage("Error: Could not open output file %s", sFile.c_str());
    return false;
  }

  //write materials
  for (int i = 0; i < (int)texAy.size(); ++i) {
    out << "newmtl " << texAy[i] << "\n";
    out << "map_Kd " << texAy[i] << ".png\n";
    out << "map_d " << texAy[i] << ".png\n";
  }

  out.close();
  return true;
}

//-------------------------------------------------------------------------------------------------

bool CObjExporter::ExportShapes(std::vector<std::pair<std::string, CShapeData *>> shapeAy, const std::string &sFile, const std::string &sMtlFile, const std::string &sMtlName)
{
  //open output file
  std::ofstream out(sFile);
  if (!out.is_open()) {
    Logging::LogMessage("Error: Could not open output file %s", sFile.c_str());
    return false;
  }

  out << "mtllib " << sMtlFile << "\n";

  int iOffset = 0;
  for (std::vector<std::pair<std::string, CShapeData *>>::iterator it = shapeAy.begin(); it != shapeAy.end(); ++it) {
    ExportShape_Internal(out, iOffset, it->second, it->first, sMtlName);
  }

  out.close();
  return true;
}

//-------------------------------------------------------------------------------------------------

bool CObjExporter::ExportTrack(std::vector<std::pair<std::string, CShapeData *>> trackSectionAy,
                               std::vector<CShapeData *> signAy,
                               std::vector<CShapeData *> signBackAy,
                               const std::string &sFolder,
                               const std::string &sName,
                               const std::string &sFile)
{
  //create material file
  std::string sBld = sName + std::string("_BLD");
  std::vector<std::string> texAy;
  texAy.push_back(sName); //TEX
  texAy.push_back(sBld); //BLD
  std::string sMtlFile = sFolder + std::string("\\") + sName + std::string(".mtl");
  ExportMaterial(sMtlFile.c_str(), texAy);

  //open output file
  std::ofstream out(sFile);
  if (!out.is_open()) {
    Logging::LogMessage("Error: Could not open output file %s", sFile.c_str());
    return false;
  }

  int iOffset = 0;
  for (std::vector<std::pair<std::string, CShapeData *>>::iterator it = trackSectionAy.begin(); it != trackSectionAy.end(); ++it) {
   ExportShape_Internal(out, iOffset, it->second, it->first, sName);
  }
  for (int i = 0; i < (int)signAy.size(); ++i) {
    std::string sSignName = "Sign " + std::to_string(i);
    ExportShape_Internal(out, iOffset, signAy[i], sSignName, sBld);
  }
  for (int i = 0; i < (int)signBackAy.size(); ++i) {
    std::string sSignName = "Sign " + std::to_string(i) + " (Back)";
    ExportShape_Internal(out, iOffset, signBackAy[i], sSignName, sBld);
  }

  out.close();
  return true;
}

//-------------------------------------------------------------------------------------------------

void CObjExporter::ExportShape_Internal(std::ofstream &out, int &iOffset, CShapeData *pShapeData, const std::string &sName, const std::string &sMtlName)
{
  out << "o " << sName << "\n";
  out << "#vertices\n";
  for (int i = 0; i < (int)pShapeData->m_uiNumVerts; ++i) {
    out << "v "
      << std::to_string(pShapeData->m_vertices[i].position.x / 100.0f) << " "
      << std::to_string(pShapeData->m_vertices[i].position.y / 100.0f) << " "
      << std::to_string(pShapeData->m_vertices[i].position.z / 100.0f) << "\n";
  }
  out << "#tex coords\n";
  for (int i = 0; i < (int)pShapeData->m_uiNumVerts; ++i) {
    out << "vt "
      << std::to_string(pShapeData->m_vertices[i].texCoords.x) << " "
      << std::to_string(pShapeData->m_vertices[i].texCoords.y) << "\n";
  }
  out << "#normals\n";
  for (int i = 0; i < (int)pShapeData->m_uiNumVerts; ++i) {
    out << "vn "
      << std::to_string(pShapeData->m_vertices[i].normal.x) << " "
      << std::to_string(pShapeData->m_vertices[i].normal.y) << " "
      << std::to_string(pShapeData->m_vertices[i].normal.z) << "\n";
  }
  out << "usemtl " << sMtlName << "\n";
  if (pShapeData->m_drawType == eShapePrimitive::TRIANGLES) {
    out << "#pols\n";
    int iNumPols = (int)pShapeData->m_uiNumIndices / 3;
    for (int i = 0; i < iNumPols; ++i) {
      out << "f ";
      for (int j = 0; j < 3; ++j) {
        std::string sIndex = std::to_string(pShapeData->m_indices[i * 3 + j] + iOffset + 1);
        out << sIndex << "/" << sIndex << "/" << sIndex;
        if (j != 2)
          out << " ";
      }
      out << "\n";
    }
  } else if (pShapeData->m_drawType == eShapePrimitive::LINES) {
    out << "#lines\n";
    out << "l ";
    for (int i = 0; i < (int)pShapeData->m_uiNumVerts; ++i) {
      out << std::to_string(i + 1 + iOffset) << " ";
    }
    out << std::to_string(iOffset + 1);
    out << "\n";
  }
  //update offset
  iOffset += pShapeData->m_uiNumVerts;
}

//-------------------------------------------------------------------------------------------------
