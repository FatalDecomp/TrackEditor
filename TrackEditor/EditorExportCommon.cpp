#include "EditorExportCommon.h"

#include <algorithm>
#include <unordered_map>

namespace
{
// One approximation per shade_palette darkening level. ROLLER's renderer keeps
// g_sceneGpuShadeFactor[] (scene_render_gpu.c) of what is behind the surface;
// black composited at opacity d keeps (1 - d) of it.
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

// Every primitive the exporters emit is a triangle list over the shared index
// array. Collects the primitive's own vertices in first-use order and rewrites
// its triangles against that local numbering, so no exporter assumes the
// extraction gave each quad four private vertices.
bool GatherPrimitive(const tEdExportGeometry &Geometry,
                     const tEdPrimitive &Primitive, tEdExportEntry &Entry)
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

void SortByMaterial(std::vector<tEdExportEntry> &Entries)
{
  std::stable_sort(Entries.begin(), Entries.end(),
                   [](const tEdExportEntry &Left, const tEdExportEntry &Right) {
                     return Left.uiMaterial < Right.uiMaterial;
                   });
}

void AppendEntries(std::vector<tEdExportEntry> &Target,
                   std::vector<tEdExportEntry> &Source)
{
  Target.insert(Target.end(), std::make_move_iterator(Source.begin()),
                std::make_move_iterator(Source.end()));
  Source.clear();
}

void AppendObject(std::vector<tEdExportObject> &Objects, std::string sName,
                  std::vector<tEdExportEntry> &Entries)
{
  if (Entries.empty())
    return;
  SortByMaterial(Entries);
  tEdExportObject Object;
  Object.sName = std::move(sName);
  Object.Entries = std::move(Entries);
  Objects.push_back(std::move(Object));
}
}

const char *CEditorExportConventions::SurfaceClassName(uint16_t unSurfaceClass)
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

uint32_t CEditorExportConventions::ExportedSurfaceClassCount()
{
  return g_uiExportedSurfaceClassCount;
}

uint16_t CEditorExportConventions::ExportedSurfaceClass(uint32_t uiIndex)
{
  return uiIndex < g_uiExportedSurfaceClassCount
      ? g_aunExportedSurfaceClasses[uiIndex]
      : static_cast<uint16_t>(ROLLER_ED_SURFACE_CLASS_COUNT);
}

bool CEditorExportConventions::IsAuthoredContent(uint16_t unContentClass)
{
  return unContentClass == ROLLER_ED_CONTENT_AUTHORED_TRACK
      || unContentClass == ROLLER_ED_CONTENT_AUTHORED_SIGN
      || unContentClass == ROLLER_ED_CONTENT_AUTHORED_SCENERY;
}

float CEditorExportConventions::ScreenDarkenAlpha(uint32_t uiDarkenLevel)
{
  const uint32_t uiLevel = uiDarkenLevel < g_uiScreenDarkenLevels
      ? uiDarkenLevel
      : g_uiScreenDarkenLevels - 1;
  return 1.0f - g_afScreenDarkenShadeFactor[uiLevel];
}

void CEditorExportConventions::ConvertPosition(const float afRollerXYZ[3],
                                               float afOutXYZ[3])
{
  afOutXYZ[0] = afRollerXYZ[0] * ED_EXPORT_UNIT_SCALE;
  afOutXYZ[1] = afRollerXYZ[2] * ED_EXPORT_UNIT_SCALE;
  afOutXYZ[2] = -afRollerXYZ[1] * ED_EXPORT_UNIT_SCALE;
}

void CEditorExportConventions::ConvertDirection(const float afRollerXYZ[3],
                                                float afOutXYZ[3])
{
  afOutXYZ[0] = afRollerXYZ[0];
  afOutXYZ[1] = afRollerXYZ[2];
  afOutXYZ[2] = -afRollerXYZ[1];
}

bool CEditorExportConventions::IsTexturedKind(uint32_t uiKind)
{
  return uiKind == ROLLER_ED_MATERIAL_TEXTURED_TILE
      || uiKind == ROLLER_ED_MATERIAL_TEXTURED_PAIR;
}

bool CEditorExportConventions::HasDistinctReverseMaterial(
    const tEdPrimitive &Primitive)
{
  return Primitive.uiBackMaterialId != ROLLER_ED_INVALID_MATERIAL_ID;
}

bool CEditorExportConventions::HasReverseSide(const tEdPrimitive &Primitive)
{
  return HasDistinctReverseMaterial(Primitive)
      || (Primitive.unFlags & ROLLER_ED_PRIMITIVE_FLAG_TWO_SIDED) != 0;
}

uint32_t CEditorExportConventions::ReverseSideMaterial(
    const tEdPrimitive &Primitive)
{
  return HasDistinctReverseMaterial(Primitive) ? Primitive.uiBackMaterialId
                                               : Primitive.uiFrontMaterialId;
}

std::string CEditorExportConventions::TextureFileName(
    const std::string &sBaseName, uint32_t uiTextureSet)
{
  return uiTextureSet == ROLLER_ED_TEXTURE_SET_BUILDING_SIGN
      ? sBaseName + "_BLD.png"
      : sBaseName + ".png";
}

bool CEditorExportConventions::ValidateGeometry(
    const tEdExportGeometry &Geometry, std::string &sError)
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

bool CEditorExportConventions::BuildObjects(
    const tEdExportGeometry &Geometry, const tEdExportGrouping &Grouping,
    std::vector<tEdExportObject> &ObjectsOut, std::string &sError)
{
  ObjectsOut.clear();
  sError.clear();

  std::vector<std::vector<tEdExportEntry>> FrontGroups(
      g_uiExportedSurfaceClassCount);
  std::vector<std::vector<tEdExportEntry>> BackGroups(
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

    tEdExportEntry Front;
    Front.uiPrimitive = i;
    Front.uiMaterial = Primitive.uiFrontMaterialId;
    Front.bBack = false;
    if (!GatherPrimitive(Geometry, Primitive, Front)) {
      sError = "a primitive references a vertex the extraction does not "
               "contain";
      return false;
    }

    // A different back tile always needs its own geometry, because no format
    // lets one material address two tiles. A merely two-sided surface needs it
    // only where the format cannot say "draw both sides".
    const bool bNeedsBackGeometry = HasDistinctReverseMaterial(Primitive)
        || (Grouping.bReverseSideAsGeometry && HasReverseSide(Primitive));

    tEdExportEntry Back = Front;
    FrontGroups[uiSlot].push_back(std::move(Front));
    if (bNeedsBackGeometry) {
      Back.uiMaterial = ReverseSideMaterial(Primitive);
      Back.bBack = true;
      (Grouping.bSeparateBackFaces ? BackGroups : FrontGroups)[uiSlot]
          .push_back(std::move(Back));
    }
  }

  if (Grouping.bSeparateSections) {
    for (uint32_t c = 0; c < g_uiExportedSurfaceClassCount; ++c) {
      AppendObject(ObjectsOut,
                   std::string(SurfaceClassName(g_aunExportedSurfaceClasses[c])),
                   FrontGroups[c]);
    }
    if (Grouping.bSeparateBackFaces) {
      for (uint32_t c = 0; c < g_uiExportedSurfaceClassCount; ++c) {
        AppendObject(
            ObjectsOut,
            std::string(SurfaceClassName(g_aunExportedSurfaceClasses[c]))
                + " (Back)",
            BackGroups[c]);
      }
    }
  } else {
    std::vector<tEdExportEntry> Combined;
    std::vector<tEdExportEntry> CombinedBacks;
    for (uint32_t c = 0; c < g_uiExportedSurfaceClassCount; ++c) {
      AppendEntries(Combined, FrontGroups[c]);
      AppendEntries(CombinedBacks, BackGroups[c]);
    }
    AppendObject(ObjectsOut, std::string("Track"), Combined);
    if (Grouping.bSeparateBackFaces)
      AppendObject(ObjectsOut, std::string("Track (Back)"), CombinedBacks);
  }

  if (ObjectsOut.empty()) {
    sError = "the loaded track produced no authored surfaces to export";
    return false;
  }
  return true;
}
