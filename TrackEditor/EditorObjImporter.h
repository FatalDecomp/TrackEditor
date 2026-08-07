#ifndef _TRACKEDITOR_EDITOROBJIMPORTER_H
#define _TRACKEDITOR_EDITOROBJIMPORTER_H
//-------------------------------------------------------------------------------------------------
#include <cstdint>
#include <string>
#include <vector>
//-------------------------------------------------------------------------------------------------

// E4-S6's reference-model import. Formerly WhipLib's CObjImporter, which
// emitted a CShapeData of glm-typed tVertex; the caller threw all of that away
// immediately to build flat tEdReferenceVertex arrays, so the geometry is now
// flat floats from the start and glm has left the tree.
//
// Positions and normals come out in the FILE's own units and axes. The unit
// scale and the +Y-up to +Z-up rotation are one mapping and live with their
// export inverse in CEditorExportConventions, not half here.
//
// UVs are deliberately absent: a reference model has no texture of its own and
// roller-core draws it flat, so every consumer discarded them.
struct tEditorImportedMesh
{
  std::vector<float> Positions;  //3 per vertex
  std::vector<float> Normals;    //3 per vertex, zero where the file gave none
  std::vector<uint32_t> Indices; //triangle list

  size_t VertexCount() const { return Positions.size() / 3; }
};

//-------------------------------------------------------------------------------------------------

namespace EditorObjImporter
{
  //Failures are reported through Logging, matching the previous importer.
  extern bool ImportObj(const std::string &sFile, tEditorImportedMesh &Mesh);
}

//-------------------------------------------------------------------------------------------------
#endif
