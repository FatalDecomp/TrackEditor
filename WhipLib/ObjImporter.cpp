#include "ObjImporter.h"
#include "Logging.h"
#include "Vertex.h"
#include "Texture.h"
#include "ShapeData.h"
#include "VertexBuffer.h"
#include "VertexArray.h"
#include "IndexBuffer.h"
#include <fstream>
#include <sstream>
#include <string.h>
#include <vector>
#include <glm.hpp>
#include <glew.h>
#include "gtc/matrix_transform.hpp"
#include "gtx/transform.hpp"
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

bool CObjImporter::ImportObj(const std::string &sFile, CShapeData **pShape, CShader *pShader, CTexture *pTexture)
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
      double dX = std::stod(lineAy[1]) * 100.0;
      double dY = std::stod(lineAy[2]) * 100.0;
      double dZ = std::stod(lineAy[3]) * 100.0;
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
      if (lineAy.size() != 4 && lineAy.size() != 5) {
        Logging::LogMessage("Invalid polygon line (%d in obj file %s)", iLineIndex, sFile.c_str());
        bSuccess = false;
        break;
      }

      for (int i = 1; i < lineAy.size(); ++i) {
        std::vector<std::string> polygonAy;
        std::stringstream ssPol(lineAy[i]);
        while (ssPol.good()) {
          std::string sIndex;
          getline(ssPol, sIndex, '/');
          if (!sIndex.empty())
            polygonAy.push_back(sIndex);
        }

        if (polygonAy.size() < 1 || polygonAy.size() > 3) {
          Logging::LogMessage("Polygon has wrong number of indices (%d in obj file %s)", iLineIndex, sFile.c_str());
          bSuccess = false;
          break;
        }

        int ivIdx = std::stoi(polygonAy[0]) - 1;
        int ivtIdx = -1;
        int ivnIdx = -1;
        if (ivIdx >= (int)vAy.size() || ivIdx < 0) {
          Logging::LogMessage("Vertex index out of bounds (%d in obj file %s)", iLineIndex, sFile.c_str());
          bSuccess = false;
          break;
        }
        if (polygonAy.size() > 1) {
          ivtIdx = std::stoi(polygonAy[1]) - 1;
          if (ivtIdx >= (int)vtAy.size() || ivtIdx < 0) {
            Logging::LogMessage("Tex coord index out of bounds (%d in obj file %s)", iLineIndex, sFile.c_str());
            bSuccess = false;
            break;
          }
        }
        if (polygonAy.size() > 2) {
          ivnIdx = std::stoi(polygonAy[2]) - 1;
          if (ivnIdx >= (int)vnAy.size() || ivnIdx < 0) {
            Logging::LogMessage("Normal index out of bounds (%d in obj file %s)", iLineIndex, sFile.c_str());
            bSuccess = false;
            break;
          }
        }

        tVertex vertex;
        vertex.position = vAy[ivIdx];
        //vertex.texCoords = vtAy[ivtIdx];
        vertex.texCoords = pTexture->GetColorCenterCoordinates(0x8c); //light grey
        if(ivnIdx >= 0)
          vertex.normal = vnAy[ivnIdx];
        vertexAy.push_back(vertex);
      }

      // it's a quad, so complete a second triangle
      if (lineAy.size() == 5) {
        vertexAy.push_back(vertexAy[vertexAy.size() - 4]);
        vertexAy.push_back(vertexAy[vertexAy.size() - 3]);
      }
    }
    ++iLineIndex;
  }
  file.close();

  //make shape
  uint32 uiNumVerts = (uint32)vertexAy.size();
  tVertex *vertices = new tVertex[uiNumVerts];
  uint32 uiNumIndices = (uint32)vertexAy.size();
  uint32 *indices = new uint32[uiNumIndices];
  GLenum drawType = GL_TRIANGLES;
  for (int i = 0; i < (int)uiNumVerts; ++i) {
    vertices[i] = vertexAy[i];
    indices[i] = i;
  }

  if (vertices && indices) {
    if (!*pShape) {
      CVertexBuffer *pVertexBuf = new CVertexBuffer(vertices, uiNumVerts, GL_DYNAMIC_DRAW);
      CIndexBuffer *pIndexBuf = new CIndexBuffer(indices, uiNumIndices, GL_DYNAMIC_DRAW);
      CVertexArray *pVertexArray = new CVertexArray(pVertexBuf);

      *pShape = new CShapeData(pVertexBuf, pIndexBuf, pVertexArray, pShader, pTexture, drawType);
    } else {
      (*pShape)->m_pVertexBuf->Update(vertices, uiNumVerts);
      (*pShape)->m_pIndexBuf->Update(indices, uiNumIndices);
    }

    delete[] vertices;
    delete[] indices;
  }

  return bSuccess;
}

//-------------------------------------------------------------------------------------------------