#include "EditorObjExporter.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <locale>
#include <vector>

namespace
{
// -0.0 is valid but prints as "-0.000000", which is noise in a diff.
float Normalize(float fValue)
{
  return fValue == 0.0f ? 0.0f : fValue;
}

void WriteColour(std::ostream &Out, const tEdExportPaletteEntry &Colour)
{
  Out << Normalize(static_cast<float>(Colour.byRed) / 255.0f) << " "
      << Normalize(static_cast<float>(Colour.byGreen) / 255.0f) << " "
      << Normalize(static_cast<float>(Colour.byBlue) / 255.0f);
}

void WriteMaterial(std::ostream &Out, const std::string &sBaseName,
                   const tEdMaterial &Material,
                   const tEdExportPaletteEntry *pPalette,
                   uint32_t uiPaletteCount)
{
  Out << "newmtl " << CEditorObjExporter::MaterialName(sBaseName, Material)
      << "\n";

  if (CEditorExportConventions::IsTexturedKind(Material.uiKind)) {
    const std::string sTexture = CEditorExportConventions::TextureFileName(
        sBaseName, Material.uiTextureSet);
    Out << "map_Kd " << sTexture << "\n";
    Out << "map_d " << sTexture << "\n";
    return;
  }

  if (Material.uiKind == ROLLER_ED_MATERIAL_SCREEN_DARKEN) {
    // Never a texture. The surface exists in the track and has a shape, so it
    // is exported, but as the documented darkening approximation.
    Out << "Ka 0.000000 0.000000 0.000000\n";
    Out << "Kd 0.000000 0.000000 0.000000\n";
    Out << "d "
        << Normalize(CEditorExportConventions::ScreenDarkenAlpha(
               Material.uiDarkenLevel))
        << "\n";
    return;
  }

  tEdExportPaletteEntry Colour;
  if (pPalette && Material.uiPaletteColour < uiPaletteCount)
    Colour = pPalette[Material.uiPaletteColour];
  Out << "Ka ";
  WriteColour(Out, Colour);
  Out << "\n";
  Out << "Kd ";
  WriteColour(Out, Colour);
  Out << "\n";
}

void WriteObject(std::ostream &Out, const tEdExportObject &Object,
                 const std::string &sBaseName,
                 const tEdExportGeometry &Geometry, int &iVertexOffset)
{
  const std::vector<tEdExportEntry> &Entries = Object.Entries;
  Out << "o " << Object.sName << "\n";

  Out << "#vertices\n";
  for (size_t e = 0; e < Entries.size(); ++e) {
    for (size_t v = 0; v < Entries[e].Vertices.size(); ++v) {
      float afPosition[3];
      CEditorExportConventions::ConvertPosition(
          Geometry.pVertices[Entries[e].Vertices[v]].fPosition, afPosition);
      Out << "v " << Normalize(afPosition[0]) << " "
          << Normalize(afPosition[1]) << " " << Normalize(afPosition[2])
          << "\n";
    }
  }

  // Material-local UVs resolve through the selected material's atlas transform
  // (AD-7b); the reverse side deliberately resolves through the back material,
  // because texture_back[] can substitute a different tile and the front
  // material's rectangle would sample the wrong one. The exporter does no tile
  // arithmetic of its own. The final flip is the exported PNG's row origin:
  // ROLLER's UV origin is top-left, OBJ's is bottom-left.
  Out << "#tex coords\n";
  for (size_t e = 0; e < Entries.size(); ++e) {
    const tEdMaterial &Material = Geometry.pMaterials[Entries[e].uiMaterial];
    for (size_t v = 0; v < Entries[e].Vertices.size(); ++v) {
      const tEdVertex &Vertex = Geometry.pVertices[Entries[e].Vertices[v]];
      const float fU = Vertex.fUV[0] * Material.fAtlasScale[0]
          + Material.fAtlasBias[0];
      const float fV = Vertex.fUV[1] * Material.fAtlasScale[1]
          + Material.fAtlasBias[1];
      Out << "vt " << Normalize(fU) << " " << Normalize(1.0f - fV) << "\n";
    }
  }

  Out << "#normals\n";
  for (size_t e = 0; e < Entries.size(); ++e) {
    const float fSign = Entries[e].bBack ? -1.0f : 1.0f;
    for (size_t v = 0; v < Entries[e].Vertices.size(); ++v) {
      float afNormal[3];
      CEditorExportConventions::ConvertDirection(
          Geometry.pVertices[Entries[e].Vertices[v]].fNormal, afNormal);
      Out << "vn " << Normalize(fSign * afNormal[0]) << " "
          << Normalize(fSign * afNormal[1]) << " "
          << Normalize(fSign * afNormal[2]) << "\n";
    }
  }

  Out << "#pols\n";
  int iEntryBase = iVertexOffset;
  uint32_t uiCurrentMaterial = UINT32_MAX;
  for (size_t e = 0; e < Entries.size(); ++e) {
    if (Entries[e].uiMaterial != uiCurrentMaterial) {
      uiCurrentMaterial = Entries[e].uiMaterial;
      Out << "usemtl "
          << CEditorObjExporter::MaterialName(
                 sBaseName, Geometry.pMaterials[uiCurrentMaterial])
          << "\n";
    }

    const std::vector<uint32_t> &Triangles = Entries[e].Triangles;
    for (size_t t = 0; t + 2 < Triangles.size(); t += 3) {
      // The axis conversion preserves handedness, so a front face keeps the
      // emitted vertex order and OBJ's counter-clockwise front lands on the
      // side uiFrontMaterialId describes. The reverse side is the same
      // triangle wound the other way.
      const size_t aiCorner[3] = {
        Entries[e].bBack ? t + 2 : t,
        t + 1,
        Entries[e].bBack ? t : t + 2
      };
      Out << "f";
      for (int c = 0; c < 3; ++c) {
        const int iIndex =
            iEntryBase + static_cast<int>(Triangles[aiCorner[c]]) + 1;
        Out << " " << iIndex << "/" << iIndex << "/" << iIndex;
      }
      Out << "\n";
    }
    iEntryBase += static_cast<int>(Entries[e].Vertices.size());
  }
  iVertexOffset = iEntryBase;
}
}

std::string CEditorObjExporter::MaterialName(const std::string &sBaseName,
                                             const tEdMaterial &Material)
{
  if (CEditorExportConventions::IsTexturedKind(Material.uiKind)) {
    // One material per texture set, not per tile: the UVs are already atlas
    // space, so every tile of a bank shares that bank's PNG. This is also the
    // material set the pre-migration exporter wrote.
    return Material.uiTextureSet == ROLLER_ED_TEXTURE_SET_BUILDING_SIGN
        ? sBaseName + "_BLD"
        : sBaseName;
  }
  if (Material.uiKind == ROLLER_ED_MATERIAL_SCREEN_DARKEN)
    return sBaseName + "_darken_" + std::to_string(Material.uiDarkenLevel);
  return sBaseName + "_color_" + std::to_string(Material.uiPaletteColour);
}

bool CEditorObjExporter::Export(const tEdExportGeometry &Geometry,
                                const tEdObjExportOptions &Options,
                                const tEdExportPaletteEntry *pPalette,
                                uint32_t uiPaletteCount,
                                std::ostream &ObjStream,
                                std::ostream &MtlStream,
                                std::string &sError)
{
  sError.clear();
  if (Options.sBaseName.empty() || Options.sMtlFileName.empty()) {
    sError = "the export needs a base name and a material file name";
    return false;
  }
  if (!CEditorExportConventions::ValidateGeometry(Geometry, sError))
    return false;

  tEdExportGrouping Grouping;
  Grouping.bExportScenery = Options.bExportScenery;
  Grouping.bSeparateSections = Options.bSeparateSections;
  Grouping.bSeparateBackFaces = Options.bSeparateBackFaces;
  // OBJ has no way to say "draw both sides", so a two-sided surface needs real
  // reverse geometry or it leaves a hole.
  Grouping.bReverseSideAsGeometry = true;

  std::vector<tEdExportObject> Objects;
  if (!CEditorExportConventions::BuildObjects(Geometry, Grouping, Objects,
                                              sError)) {
    return false;
  }

  // A decimal separator taken from the user's locale would produce an OBJ no
  // importer can read. Six fixed decimals matches the pre-migration exporter.
  ObjStream.imbue(std::locale::classic());
  MtlStream.imbue(std::locale::classic());
  ObjStream << std::fixed << std::setprecision(6);
  MtlStream << std::fixed << std::setprecision(6);

  // Only the materials the export actually references reach the .mtl.
  std::vector<uint32_t> UsedMaterials;
  for (size_t o = 0; o < Objects.size(); ++o) {
    for (size_t e = 0; e < Objects[o].Entries.size(); ++e) {
      const uint32_t uiMaterial = Objects[o].Entries[e].uiMaterial;
      if (std::find(UsedMaterials.begin(), UsedMaterials.end(), uiMaterial)
          == UsedMaterials.end()) {
        UsedMaterials.push_back(uiMaterial);
      }
    }
  }
  std::sort(UsedMaterials.begin(), UsedMaterials.end());

  std::vector<std::string> WrittenMaterials;
  for (size_t m = 0; m < UsedMaterials.size(); ++m) {
    const tEdMaterial &Material = Geometry.pMaterials[UsedMaterials[m]];
    const std::string sName = MaterialName(Options.sBaseName, Material);
    // Distinct atlas tiles share one texture-set material, so the same name is
    // reached many times; the .mtl declares it once.
    if (std::find(WrittenMaterials.begin(), WrittenMaterials.end(), sName)
        != WrittenMaterials.end()) {
      continue;
    }
    WrittenMaterials.push_back(sName);
    WriteMaterial(MtlStream, Options.sBaseName, Material, pPalette,
                  uiPaletteCount);
  }

  ObjStream << "mtllib " << Options.sMtlFileName << "\n";
  int iVertexOffset = 0;
  for (size_t o = 0; o < Objects.size(); ++o) {
    WriteObject(ObjStream, Objects[o], Options.sBaseName, Geometry,
                iVertexOffset);
  }
  return true;
}

bool CEditorObjExporter::ExportToFiles(const tEdExportGeometry &Geometry,
                                       const tEdObjExportOptions &Options,
                                       const tEdExportPaletteEntry *pPalette,
                                       uint32_t uiPaletteCount,
                                       const std::string &sObjFile,
                                       const std::string &sMtlFile,
                                       std::string &sError)
{
  std::ofstream Obj(sObjFile.c_str());
  if (!Obj.is_open()) {
    sError = "could not open " + sObjFile;
    return false;
  }
  std::ofstream Mtl(sMtlFile.c_str());
  if (!Mtl.is_open()) {
    sError = "could not open " + sMtlFile;
    return false;
  }

  if (!Export(Geometry, Options, pPalette, uiPaletteCount, Obj, Mtl, sError))
    return false;

  Obj.close();
  Mtl.close();
  if (!Obj.good() || !Mtl.good()) {
    sError = "could not write the exported model";
    return false;
  }
  return true;
}
