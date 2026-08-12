#ifndef TRACKEDITOR_EDITORCARMODEL_H
#define TRACKEDITOR_EDITORCARMODEL_H

#include "EditorExportCommon.h"

#include <cstdint>
#include <string>
#include <vector>

struct tEdCarGeometry
{
  std::vector<tEdVertex> Vertices;
  std::vector<uint32_t> Indices;
  std::vector<tEdPrimitive> Primitives;
  std::vector<tEdMaterial> Materials;

  void Clear();
  tEdExportGeometry View() const;
};

// Adapts ROLLER's immutable, canonical CarDesigns plans to the same geometry
// vocabulary the track exporters consume. The car's texture bank is supplied
// by the batch caller because it comes from the selected FATDATA folder.
class CEditorCarModel
{
public:
  static uint32_t Count();
  static const char *Name(uint32_t uiDesign);
  static const char *TextureFileName(uint32_t uiDesign);

  static bool Build(uint32_t uiDesign, uint32_t uiTextureTileCount,
                    tEdCarGeometry &GeometryOut, std::string &sError);
};

#endif
