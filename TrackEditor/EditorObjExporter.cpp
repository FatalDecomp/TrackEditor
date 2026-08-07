#include "EditorObjExporter.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <locale>
#include <unordered_map>
#include <utility>
#include <vector>

namespace
{
// One approximation per shade_palette darkening level. ROLLER's renderer
// keeps g_sceneGpuShadeFactor[] (scene_render_gpu.c) of what is behind the
// surface; black composited at dissolve d keeps (1 - d) of it.
const float g_afScreenDarkenShadeFactor[] = { 0.8f, 0.6f, 0.4f, 0.2f, 0.3f };
const uint32_t g_uiScreenDarkenLevels =
    static_cast<uint32_t>(sizeof(g_afScreenDarkenShadeFactor)
                          / sizeof(g_afScreenDarkenShadeFactor[0]));

// Canonical class order. This is the order the pre-migration exporter listed
// its objects in, and it matches eRollerEdSurfaceClass, so the two agree
// without a translation table.
const uint16_t g_aunExportedSurfaceClasses[] = {
  ROLLER_ED_SURFACE_CLASS_CENTER,
  ROLLER_ED_SURFACE_CLASS_LEFT_SHOULDER,
  ROLLER_ED_SURFACE_CLASS_RIGHT_SHOULDER,
  ROLLER_ED_SURFACE_CLASS_LEFT_WALL,
  ROLLER_ED_SURFACE_CLASS_RIGHT_WALL,
  ROLLER_ED_SURFACE_CLASS_ROOF,
  ROLLER_ED_SURFACE_CLASS_OUTER_WALL_FLOOR,
  ROLLER_ED_SURFACE_CLASS_LEFT_LOWER_OUTER_WALL,
  ROLLER_ED_SURFACE_CLASS_RIGHT_LOWER_OUTER_WALL,
  ROLLER_ED_SURFACE_CLASS_LEFT_UPPER_OUTER_WALL,
  ROLLER_ED_SURFACE_CLASS_RIGHT_UPPER_OUTER_WALL
};
const uint32_t g_uiExportedSurfaceClassCount =
    static_cast<uint32_t>(sizeof(g_aunExportedSurfaceClasses)
                          / sizeof(g_aunExportedSurfaceClasses[0]));

// One face-set to emit: a primitive taken from one of its two sides, with the
// vertices it uses already resolved to a local numbering.
struct tExportEntry
{
  uint32_t uiPrimitive = 0;
  uint32_t uiMaterial = 0;
  bool bBack = false;
  std::vector<uint32_t> Vertices;  // global vertex ids, first-use order
  std::vector<uint32_t> Triangles; // indices into Vertices
};

bool HasReverseSide(const tEdPrimitive &Primitive)
{
  // A missing back material does not imply single-sidedness (editor_api.h):
  // SURFACE_FLAG_CONCAVE surfaces are drawn from both sides with the same
  // material, and exporting them single-sided would leave holes (ADR 0003).
  return Primitive.uiBackMaterialId != ROLLER_ED_INVALID_MATERIAL_ID
      || (Primitive.unFlags & ROLLER_ED_PRIMITIVE_FLAG_TWO_SIDED) != 0;
}

uint32_t ReverseSideMaterial(const tEdPrimitive &Primitive)
{
  return Primitive.uiBackMaterialId != ROLLER_ED_INVALID_MATERIAL_ID
      ? Primitive.uiBackMaterialId
      : Primitive.uiFrontMaterialId;
}

bool IsTexturedKind(uint32_t uiKind)
{
  return uiKind == ROLLER_ED_MATERIAL_TEXTURED_TILE
      || uiKind == ROLLER_ED_MATERIAL_TEXTURED_PAIR;
}

// -0.0 is valid but prints as "-0.000000", which is noise in a diff.
float Normalize(float fValue)
{
  return fValue == 0.0f ? 0.0f : fValue;
}

void WriteColour(std::ostream &Out, const tEdObjExportPaletteEntry &Colour)
{
  Out << Normalize(static_cast<float>(Colour.byRed) / 255.0f) << " "
      << Normalize(static_cast<float>(Colour.byGreen) / 255.0f) << " "
      << Normalize(static_cast<float>(Colour.byBlue) / 255.0f);
}

// Every primitive this exporter emits is a triangle list over the shared index
// array. Collects the primitive's own vertices in first-use order and rewrites
// its triangles against that local numbering, so the exporter does not assume
// the extraction gave each quad four private vertices.
bool GatherPrimitive(const tEdObjExportGeometry &Geometry,
                     const tEdPrimitive &Primitive,
                     tExportEntry &Entry)
{
  std::unordered_map<uint32_t, uint32_t> Remap;
  for (uint32_t i = 0; i < Primitive.uiIndexCount; ++i) {
    const uint32_t uiGlobal = Geometry.puiIndices[Primitive.uiFirstIndex + i];
    if (uiGlobal >= Geometry.uiVertexCount)
      return false;
    const std::unordered_map<uint32_t, uint32_t>::const_iterator Found =
        Remap.find(uiGlobal);
    if (Found != Remap.end()) {
      Entry.Triangles.push_back(Found->second);
      continue;
    }
    const uint32_t uiLocal = static_cast<uint32_t>(Entry.Vertices.size());
    Remap.emplace(uiGlobal, uiLocal);
    Entry.Vertices.push_back(uiGlobal);
    Entry.Triangles.push_back(uiLocal);
  }
  return true;
}

bool ValidateGeometry(const tEdObjExportGeometry &Geometry,
                      std::string &sError)
{
  if (Geometry.uiPrimitiveCount == 0) {
    sError = "the loaded track produced no exportable geometry";
    return false;
  }
  if (!Geometry.pVertices || !Geometry.puiIndices || !Geometry.pPrimitives
      || !Geometry.pMaterials) {
    sError = "the geometry extraction is missing one of its arrays";
    return false;
  }
  for (uint32_t i = 0; i < Geometry.uiPrimitiveCount; ++i) {
    const tEdPrimitive &Primitive = Geometry.pPrimitives[i];
    if (Primitive.uiIndexCount == 0 || (Primitive.uiIndexCount % 3) != 0
        || Primitive.uiFirstIndex > Geometry.uiIndexCount
        || Primitive.uiIndexCount
               > Geometry.uiIndexCount - Primitive.uiFirstIndex) {
      sError = "a primitive's index range falls outside the extraction";
      return false;
    }
    if (Primitive.uiFrontMaterialId >= Geometry.uiMaterialCount) {
      sError = "a primitive names a front material the extraction does not "
               "contain";
      return false;
    }
    if (Primitive.uiBackMaterialId != ROLLER_ED_INVALID_MATERIAL_ID
        && Primitive.uiBackMaterialId >= Geometry.uiMaterialCount) {
      sError = "a primitive names a back material the extraction does not "
               "contain";
      return false;
    }
  }
  return true;
}

void WriteMaterial(std::ostream &Out, const std::string &sBaseName,
                   const tEdMaterial &Material,
                   const tEdObjExportPaletteEntry *pPalette,
                   uint32_t uiPaletteCount)
{
  Out << "newmtl " << CEditorObjExporter::MaterialName(sBaseName, Material)
      << "\n";

  if (IsTexturedKind(Material.uiKind)) {
    const std::string sTexture =
        CEditorObjExporter::TextureFileName(sBaseName, Material.uiTextureSet);
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
        << Normalize(CEditorObjExporter::ScreenDarkenAlpha(
               Material.uiDarkenLevel))
        << "\n";
    return;
  }

  tEdObjExportPaletteEntry Colour;
  if (pPalette && Material.uiPaletteColour < uiPaletteCount)
    Colour = pPalette[Material.uiPaletteColour];
  Out << "Ka ";
  WriteColour(Out, Colour);
  Out << "\n";
  Out << "Kd ";
  WriteColour(Out, Colour);
  Out << "\n";
}

void WriteObject(std::ostream &Out, const std::string &sName,
                 const std::string &sBaseName,
                 const tEdObjExportGeometry &Geometry,
                 const std::vector<tExportEntry> &Entries,
                 int &iVertexOffset)
{
  if (Entries.empty())
    return;

  Out << "o " << sName << "\n";

  Out << "#vertices\n";
  for (size_t e = 0; e < Entries.size(); ++e) {
    for (size_t v = 0; v < Entries[e].Vertices.size(); ++v) {
      float afPosition[3];
      CEditorObjExporter::ConvertPosition(
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
      CEditorObjExporter::ConvertDirection(
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

void SortByMaterial(std::vector<tExportEntry> &Entries)
{
  std::stable_sort(Entries.begin(), Entries.end(),
                   [](const tExportEntry &Left, const tExportEntry &Right) {
                     return Left.uiMaterial < Right.uiMaterial;
                   });
}

void AppendEntries(std::vector<tExportEntry> &Target,
                   std::vector<tExportEntry> &Source)
{
  Target.insert(Target.end(), std::make_move_iterator(Source.begin()),
                std::make_move_iterator(Source.end()));
  Source.clear();
}
}

const char *CEditorObjExporter::SurfaceClassName(uint16_t unSurfaceClass)
{
  switch (unSurfaceClass) {
    case ROLLER_ED_SURFACE_CLASS_CENTER:                 return "Center";
    case ROLLER_ED_SURFACE_CLASS_LEFT_SHOULDER:          return "Left Shoulder";
    case ROLLER_ED_SURFACE_CLASS_RIGHT_SHOULDER:         return "Right Shoulder";
    case ROLLER_ED_SURFACE_CLASS_LEFT_WALL:              return "Left Wall";
    case ROLLER_ED_SURFACE_CLASS_RIGHT_WALL:             return "Right Wall";
    case ROLLER_ED_SURFACE_CLASS_ROOF:                   return "Roof";
    case ROLLER_ED_SURFACE_CLASS_OUTER_WALL_FLOOR:       return "Outer Wall Floor";
    case ROLLER_ED_SURFACE_CLASS_LEFT_LOWER_OUTER_WALL:  return "Left Lower Outer Wall";
    case ROLLER_ED_SURFACE_CLASS_RIGHT_LOWER_OUTER_WALL: return "Right Lower Outer Wall";
    case ROLLER_ED_SURFACE_CLASS_LEFT_UPPER_OUTER_WALL:  return "Left Upper Outer Wall";
    case ROLLER_ED_SURFACE_CLASS_RIGHT_UPPER_OUTER_WALL: return "Right Upper Outer Wall";
    default:                                             return nullptr;
  }
}

bool CEditorObjExporter::IsAuthoredContent(uint16_t unContentClass)
{
  return unContentClass == ROLLER_ED_CONTENT_AUTHORED_TRACK
      || unContentClass == ROLLER_ED_CONTENT_AUTHORED_SIGN
      || unContentClass == ROLLER_ED_CONTENT_AUTHORED_SCENERY;
}

float CEditorObjExporter::ScreenDarkenAlpha(uint32_t uiDarkenLevel)
{
  const uint32_t uiLevel = uiDarkenLevel < g_uiScreenDarkenLevels
      ? uiDarkenLevel
      : g_uiScreenDarkenLevels - 1;
  return 1.0f - g_afScreenDarkenShadeFactor[uiLevel];
}

void CEditorObjExporter::ConvertPosition(const float afRollerXYZ[3],
                                         float afObjXYZ[3])
{
  afObjXYZ[0] = afRollerXYZ[0] * ED_OBJ_EXPORT_UNIT_SCALE;
  afObjXYZ[1] = afRollerXYZ[2] * ED_OBJ_EXPORT_UNIT_SCALE;
  afObjXYZ[2] = -afRollerXYZ[1] * ED_OBJ_EXPORT_UNIT_SCALE;
}

void CEditorObjExporter::ConvertDirection(const float afRollerXYZ[3],
                                          float afObjXYZ[3])
{
  afObjXYZ[0] = afRollerXYZ[0];
  afObjXYZ[1] = afRollerXYZ[2];
  afObjXYZ[2] = -afRollerXYZ[1];
}

std::string CEditorObjExporter::MaterialName(const std::string &sBaseName,
                                             const tEdMaterial &Material)
{
  if (IsTexturedKind(Material.uiKind)) {
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

std::string CEditorObjExporter::TextureFileName(const std::string &sBaseName,
                                                uint32_t uiTextureSet)
{
  return uiTextureSet == ROLLER_ED_TEXTURE_SET_BUILDING_SIGN
      ? sBaseName + "_BLD.png"
      : sBaseName + ".png";
}

bool CEditorObjExporter::Export(const tEdObjExportGeometry &Geometry,
                                const tEdObjExportOptions &Options,
                                const tEdObjExportPaletteEntry *pPalette,
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
  if (!ValidateGeometry(Geometry, sError))
    return false;

  // A decimal separator taken from the user's locale would produce an OBJ no
  // importer can read. Six fixed decimals matches the pre-migration exporter.
  ObjStream.imbue(std::locale::classic());
  MtlStream.imbue(std::locale::classic());
  ObjStream << std::fixed << std::setprecision(6);
  MtlStream << std::fixed << std::setprecision(6);

  // Bucket every authored primitive by the object it belongs to. The eleven
  // canonical classes keep their pre-migration names; sign, building, and
  // tower surfaces do not reach RollerEd_FillGeometry today, so nothing else
  // can appear here.
  std::vector<std::vector<tExportEntry>> FrontObjects(
      g_uiExportedSurfaceClassCount);
  std::vector<std::vector<tExportEntry>> BackObjects(
      g_uiExportedSurfaceClassCount);

  for (uint32_t i = 0; i < Geometry.uiPrimitiveCount; ++i) {
    const tEdPrimitive &Primitive = Geometry.pPrimitives[i];
    if (Primitive.byTopology != ROLLER_ED_TOPOLOGY_TRIANGLE_LIST)
      continue;
    // AD-6d/AD-6e: authored content only, filtered on the content class the
    // producer published rather than inferred from the surface class.
    if (!IsAuthoredContent(Primitive.unContentClass))
      continue;

    uint32_t uiSlot = g_uiExportedSurfaceClassCount;
    for (uint32_t c = 0; c < g_uiExportedSurfaceClassCount; ++c) {
      if (g_aunExportedSurfaceClasses[c] == Primitive.unSurfaceClass) {
        uiSlot = c;
        break;
      }
    }
    if (uiSlot == g_uiExportedSurfaceClassCount)
      continue;

    tExportEntry Front;
    Front.uiPrimitive = i;
    Front.uiMaterial = Primitive.uiFrontMaterialId;
    Front.bBack = false;
    if (!GatherPrimitive(Geometry, Primitive, Front)) {
      sError = "a primitive references a vertex the extraction does not "
               "contain";
      return false;
    }

    tExportEntry Back = Front;
    const bool bHasBack = HasReverseSide(Primitive);
    FrontObjects[uiSlot].push_back(std::move(Front));
    if (bHasBack) {
      Back.uiMaterial = ReverseSideMaterial(Primitive);
      Back.bBack = true;
      (Options.bSeparateBackFaces ? BackObjects : FrontObjects)[uiSlot]
          .push_back(std::move(Back));
    }
  }

  std::vector<std::pair<std::string, std::vector<tExportEntry>>> Objects;
  if (Options.bSeparateSections) {
    for (uint32_t c = 0; c < g_uiExportedSurfaceClassCount; ++c) {
      SortByMaterial(FrontObjects[c]);
      Objects.emplace_back(
          std::string(SurfaceClassName(g_aunExportedSurfaceClasses[c])),
          std::move(FrontObjects[c]));
    }
    if (Options.bSeparateBackFaces) {
      for (uint32_t c = 0; c < g_uiExportedSurfaceClassCount; ++c) {
        SortByMaterial(BackObjects[c]);
        Objects.emplace_back(
            std::string(SurfaceClassName(g_aunExportedSurfaceClasses[c]))
                + " (Back)",
            std::move(BackObjects[c]));
      }
    }
  } else {
    std::vector<tExportEntry> Combined;
    std::vector<tExportEntry> CombinedBacks;
    for (uint32_t c = 0; c < g_uiExportedSurfaceClassCount; ++c) {
      AppendEntries(Combined, FrontObjects[c]);
      AppendEntries(CombinedBacks, BackObjects[c]);
    }
    SortByMaterial(Combined);
    Objects.emplace_back(std::string("Track"), std::move(Combined));
    if (Options.bSeparateBackFaces) {
      SortByMaterial(CombinedBacks);
      Objects.emplace_back(std::string("Track (Back)"),
                           std::move(CombinedBacks));
    }
  }

  // Only the materials the export actually references reach the .mtl.
  std::vector<uint32_t> UsedMaterials;
  for (size_t o = 0; o < Objects.size(); ++o) {
    for (size_t e = 0; e < Objects[o].second.size(); ++e) {
      const uint32_t uiMaterial = Objects[o].second[e].uiMaterial;
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
    WriteObject(ObjStream, Objects[o].first, Options.sBaseName, Geometry,
                Objects[o].second, iVertexOffset);
  }

  if (iVertexOffset == 0) {
    sError = "the loaded track produced no authored surfaces to export";
    return false;
  }
  return true;
}

bool CEditorObjExporter::ExportToFiles(const tEdObjExportGeometry &Geometry,
                                       const tEdObjExportOptions &Options,
                                       const tEdObjExportPaletteEntry *pPalette,
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
