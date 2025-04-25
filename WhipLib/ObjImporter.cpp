#include "ObjImporter.h"
#include "Logging.h"
#include "Vertex.h"
#include <fstream>
#include <sstream>
#include <string.h>
#include <vector>
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
  if (sFile.empty()) {
    Logging::LogMessage("Reference model filename empty");
    return false;
  }

  //open input file
  std::ifstream file(sFile);
  if (!file.is_open()) {
    Logging::LogMessage("Failed to open file %s", sFile.c_str());
    return false;
  }

  bool bSuccess = true;
  std::vector<glm::vec3> vAy;
  std::vector<glm::vec2> vtAy;
  std::vector<glm::vec3> vnAy;
  std::vector<tVertex> vertexAy;
  std::string sLine;
  int iLineIndex = 0;
  while (std::getline(file, sLine) && bSuccess) {
    std::vector<std::string> lineAy;
    std::stringstream ssLine(sLine);
    while (ssLine.good()) {
      std::string sSubStr;
      getline(ssLine, sSubStr, ' ');
      if (!sSubStr.empty())
        lineAy.push_back(sSubStr);
    }

    if (lineAy.empty())
      continue;

    if (lineAy[0].compare("v") == 0) {
      //load vertex line
      if (lineAy.size() != 4) {
        Logging::LogMessage("Invalid vertex line (%d in obj file %s)", iLineIndex, sFile.c_str());
        bSuccess = false;
        break;
      }
      double dX = std::stod(lineAy[1]);
      double dY = std::stod(lineAy[2]);
      double dZ = std::stod(lineAy[3]);
      vAy.push_back(glm::vec3(dX, dY, dZ));
    } else if (lineAy[0].compare("vt") == 0) {
      //load tex coord line
      if (lineAy.size() != 3) {
        Logging::LogMessage("Invalid tex coord line (%d in obj file %s)", iLineIndex, sFile.c_str());
        bSuccess = false;
        break;
      }
      double dX = std::stod(lineAy[1]);
      double dY = std::stod(lineAy[2]);
      vtAy.push_back(glm::vec2(dX, dY));
    } else if (lineAy[0].compare("vn") == 0) {
      //load normal line
      if (lineAy.size() != 4) {
        Logging::LogMessage("Invalid normal line (%d in obj file %s)", iLineIndex, sFile.c_str());
        bSuccess = false;
        break;
      }
      double dX = std::stod(lineAy[1]);
      double dY = std::stod(lineAy[2]);
      double dZ = std::stod(lineAy[3]);
      vnAy.push_back(glm::vec3(dX, dY, dZ));
    } else if (lineAy[0].compare("f") == 0) {
      //load polygon line
      if (lineAy.size() != 4) {
        Logging::LogMessage("Invalid polygon line (%d in obj file %s)", iLineIndex, sFile.c_str());
        bSuccess = false;
        break;
      }

      for (int i = 1; i < 4; ++i) {
        std::vector<std::string> polygonAy;
        std::stringstream ssPol(lineAy[i]);
        while (ssPol.good()) {
          std::string sIndex;
          getline(ssPol, sIndex, '/');
          if (!sIndex.empty())
            polygonAy.push_back(sIndex);
        }

        if (polygonAy.size() != 3) {
          Logging::LogMessage("Polygon has wrong number of indices (%d in obj file %s)", iLineIndex, sFile.c_str());
          bSuccess = false;
          break;
        }

        int ivIdx = std::stoi(polygonAy[0]);
        int ivtIdx = std::stoi(polygonAy[1]);
        int ivnIdx = std::stoi(polygonAy[2]);
        if (ivIdx >= (int)vAy.size() || ivIdx < 0) {
          Logging::LogMessage("Vertex index out of bounds (%d in obj file %s)", iLineIndex, sFile.c_str());
          bSuccess = false;
          break;
        }
        if (ivtIdx >= (int)vtAy.size() || ivtIdx < 0) {
          Logging::LogMessage("Tex coord index out of bounds (%d in obj file %s)", iLineIndex, sFile.c_str());
          bSuccess = false;
          break;
        }
        if (ivnIdx >= (int)vnAy.size() || ivnIdx < 0) {
          Logging::LogMessage("Normal index out of bounds (%d in obj file %s)", iLineIndex, sFile.c_str());
          bSuccess = false;
          break;
        }

        tVertex vertex;
        vertex.position = vAy[ivIdx];
        vertex.texCoords = vtAy[ivtIdx];
        vertex.normal = vnAy[ivnIdx];
        vertexAy.push_back(vertex);
      }
    }
    ++iLineIndex;
  }
  file.close();

  return bSuccess;
}

//-------------------------------------------------------------------------------------------------