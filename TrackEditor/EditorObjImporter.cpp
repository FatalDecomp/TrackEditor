#include "EditorObjImporter.h"
#include "Logging.h"
#include <fstream>
#include <sstream>
#include <vector>
//-------------------------------------------------------------------------------------------------
#if defined(_DEBUG) && defined(IS_WINDOWS)
#define new new(_CLIENT_BLOCK, __FILE__, __LINE__)
#endif
//-------------------------------------------------------------------------------------------------

namespace
{
  struct tFloat3
  {
    float x, y, z;
  };

  void AppendFloat3(std::vector<float> &Target, const tFloat3 &Value)
  {
    Target.push_back(Value.x);
    Target.push_back(Value.y);
    Target.push_back(Value.z);
  }
}

//-------------------------------------------------------------------------------------------------

bool EditorObjImporter::ImportObj(const std::string &sFile, tEditorImportedMesh &Mesh)
{
  Mesh = tEditorImportedMesh();

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
  std::vector<tFloat3> vAy;
  std::vector<tFloat3> vnAy;
  int iNumTexCoords = 0;
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
      // E4-S6: raw file units and raw file axes. The unit scale and the
      // +Y-up to +Z-up rotation are one mapping, and it lives with its export
      // inverse in CEditorExportConventions rather than half here.
      vAy.push_back(tFloat3{(float)std::stod(lineAy[1]),
                            (float)std::stod(lineAy[2]),
                            (float)std::stod(lineAy[3])});
    } else if (lineAy[0].compare("vt") == 0) {
      //load tex coord line
      if (lineAy.size() != 3) {
        Logging::LogMessage("Invalid tex coord line (%d in obj file %s)", iLineIndex, sFile.c_str());
        bSuccess = false;
        break;
      }
      // The values themselves are dropped; only the count is kept, because the
      // face parser still bounds-checks texture indices against it.
      ++iNumTexCoords;
    } else if (lineAy[0].compare("vn") == 0) {
      //load normal line
      if (lineAy.size() != 4) {
        Logging::LogMessage("Invalid normal line (%d in obj file %s)", iLineIndex, sFile.c_str());
        bSuccess = false;
        break;
      }
      vnAy.push_back(tFloat3{(float)std::stod(lineAy[1]),
                             (float)std::stod(lineAy[2]),
                             (float)std::stod(lineAy[3])});
    } else if (lineAy[0].compare("f") == 0) {
      //load polygon line
      if (lineAy.size() != 4 && lineAy.size() != 5) {
        Logging::LogMessage("Invalid polygon line (%d in obj file %s)", iLineIndex, sFile.c_str());
        bSuccess = false;
        break;
      }

      for (int i = 1; i < (int)lineAy.size(); ++i) {
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
        int ivnIdx = -1;
        if (ivIdx >= (int)vAy.size() || ivIdx < 0) {
          Logging::LogMessage("Vertex index out of bounds (%d in obj file %s)", iLineIndex, sFile.c_str());
          bSuccess = false;
          break;
        }
        if (polygonAy.size() > 1) {
          const int ivtIdx = std::stoi(polygonAy[1]) - 1;
          if (ivtIdx >= iNumTexCoords || ivtIdx < 0) {
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

        AppendFloat3(Mesh.Positions, vAy[ivIdx]);
        AppendFloat3(Mesh.Normals,
                     ivnIdx >= 0 ? vnAy[ivnIdx] : tFloat3{0.0f, 0.0f, 0.0f});
      }

      // It's a quad, so complete a second triangle. The four corners just
      // emitted are v0..v3 and the second triangle is (v3, v0, v2) — same
      // winding as (v0, v2, v3). The previous importer expressed this as two
      // appends of "size - 4" then "size - 3", where the second index lands on
      // v2 only because the first append already grew the vector; the offsets
      // are spelled out here so that is not left to be re-derived.
      if (lineAy.size() == 5) {
        const size_t uiCount = Mesh.VertexCount();
        for (size_t uiSource : { uiCount - 4, uiCount - 2 }) {
          AppendFloat3(Mesh.Positions, tFloat3{Mesh.Positions[uiSource * 3 + 0],
                                               Mesh.Positions[uiSource * 3 + 1],
                                               Mesh.Positions[uiSource * 3 + 2]});
          AppendFloat3(Mesh.Normals, tFloat3{Mesh.Normals[uiSource * 3 + 0],
                                             Mesh.Normals[uiSource * 3 + 1],
                                             Mesh.Normals[uiSource * 3 + 2]});
        }
      }
    }
    ++iLineIndex;
  }
  file.close();

  //vertices are already in face order, so the index buffer is the identity
  Mesh.Indices.resize(Mesh.VertexCount());
  for (uint32_t i = 0; i < (uint32_t)Mesh.Indices.size(); ++i)
    Mesh.Indices[i] = i;

  return bSuccess;
}

//-------------------------------------------------------------------------------------------------
