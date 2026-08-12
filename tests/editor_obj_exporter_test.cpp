// E4-S1. The OBJ exporter is a pure function of one canonical extraction, so
// it is exercised here without a render worker, a loaded track, or Qt.
#include "EditorObjExporter.h"
#include "EditorExportCommon.h"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>

// CMake's Release configuration defines NDEBUG, which would turn every assert
// below into a no-op and make this test pass by doing nothing.
#ifdef assert
#undef assert
#endif
inline void ExportTestAssertFailed(const char *szCondition, const char *szFile,
                                   int iLine)
{
  std::fprintf(stderr, "assertion failed: %s (%s:%d)\n", szCondition, szFile,
               iLine);
  std::abort();
}
#define assert(condition) \
  ((condition) ? (void)0 : ExportTestAssertFailed(#condition, __FILE__, __LINE__))

namespace
{
// A minimal extraction builder: one quad per call, four vertices and six
// indices, exactly the shape RollerEd_FillGeometry produces.
class CExtractionBuilder
{
public:
  uint32_t AddTexturedMaterial(uint32_t uiTextureSet, float fScaleU,
                               float fScaleV, float fBiasU, float fBiasV)
  {
    tEdMaterial Material = {};
    Material.uiKind = ROLLER_ED_MATERIAL_TEXTURED_TILE;
    Material.uiTextureSet = uiTextureSet;
    Material.fAtlasScale[0] = fScaleU;
    Material.fAtlasScale[1] = fScaleV;
    Material.fAtlasBias[0] = fBiasU;
    Material.fAtlasBias[1] = fBiasV;
    m_Materials.push_back(Material);
    return static_cast<uint32_t>(m_Materials.size() - 1);
  }

  uint32_t AddFlatMaterial(uint32_t uiPaletteColour)
  {
    tEdMaterial Material = {};
    Material.uiKind = ROLLER_ED_MATERIAL_FLAT_PALETTE_COLOR;
    Material.uiPaletteColour = uiPaletteColour;
    m_Materials.push_back(Material);
    return static_cast<uint32_t>(m_Materials.size() - 1);
  }

  uint32_t AddDarkenMaterial(uint32_t uiDarkenLevel)
  {
    tEdMaterial Material = {};
    Material.uiKind = ROLLER_ED_MATERIAL_SCREEN_DARKEN;
    Material.uiDarkenLevel = uiDarkenLevel;
    m_Materials.push_back(Material);
    return static_cast<uint32_t>(m_Materials.size() - 1);
  }

  uint32_t AddQuad(uint16_t unSurfaceClass, uint16_t unContentClass,
                   uint32_t uiFrontMaterial, uint32_t uiBackMaterial,
                   uint16_t unFlags = 0)
  {
    const uint32_t uiFirstVertex =
        static_cast<uint32_t>(m_Vertices.size());
    for (int i = 0; i < 4; ++i) {
      tEdVertex Vertex = {};
      // A unit quad in the world XY plane, so the +Z-up to +Y-up rotation is
      // visible in the exported positions.
      Vertex.fPosition[0] = (i == 1 || i == 2) ? 100.0f : 0.0f;
      Vertex.fPosition[1] = (i >= 2) ? 200.0f : 0.0f;
      Vertex.fPosition[2] = 300.0f;
      Vertex.fNormal[0] = 0.0f;
      Vertex.fNormal[1] = 0.0f;
      Vertex.fNormal[2] = 1.0f;
      Vertex.fUV[0] = (i == 1 || i == 2) ? 1.0f : 0.0f;
      Vertex.fUV[1] = (i >= 2) ? 1.0f : 0.0f;
      m_Vertices.push_back(Vertex);
    }

    tEdPrimitive Primitive = {};
    Primitive.uiFirstIndex = static_cast<uint32_t>(m_Indices.size());
    Primitive.uiIndexCount = 6;
    Primitive.uiChunkId = static_cast<uint32_t>(m_Primitives.size());
    Primitive.uiFrontMaterialId = uiFrontMaterial;
    Primitive.uiBackMaterialId = uiBackMaterial;
    Primitive.unSurfaceClass = unSurfaceClass;
    Primitive.unContentClass = unContentClass;
    Primitive.unFlags = unFlags;
    Primitive.byTopology = ROLLER_ED_TOPOLOGY_TRIANGLE_LIST;

    // Both triangles start at v0, as E4A-S5 emits them.
    const uint32_t auiOrder[6] = { 0, 1, 2, 0, 2, 3 };
    for (int i = 0; i < 6; ++i)
      m_Indices.push_back(uiFirstVertex + auiOrder[i]);

    m_Primitives.push_back(Primitive);
    return static_cast<uint32_t>(m_Primitives.size() - 1);
  }

  tEdExportGeometry View() const
  {
    tEdExportGeometry Geometry;
    Geometry.pVertices = m_Vertices.data();
    Geometry.uiVertexCount = static_cast<uint32_t>(m_Vertices.size());
    Geometry.puiIndices = m_Indices.data();
    Geometry.uiIndexCount = static_cast<uint32_t>(m_Indices.size());
    Geometry.pPrimitives = m_Primitives.data();
    Geometry.uiPrimitiveCount = static_cast<uint32_t>(m_Primitives.size());
    Geometry.pMaterials = m_Materials.data();
    Geometry.uiMaterialCount = static_cast<uint32_t>(m_Materials.size());
    return Geometry;
  }

  std::vector<tEdVertex> m_Vertices;
  std::vector<uint32_t> m_Indices;
  std::vector<tEdPrimitive> m_Primitives;
  std::vector<tEdMaterial> m_Materials;
};

tEdObjExportOptions DefaultOptions()
{
  tEdObjExportOptions Options;
  // Most focused tests exercise the pre-existing selective reverse-side
  // rules. Complete-model duplication has its own regression below.
  Options.bCompleteReverseGeometry = false;
  Options.sBaseName = "TRACK3";
  Options.sMtlFileName = "TRACK3.mtl";
  return Options;
}

bool Contains(const std::string &sHaystack, const std::string &sNeedle)
{
  return sHaystack.find(sNeedle) != std::string::npos;
}

// Whole-line match, so "o Sign 0" does not also match "o Sign 0 (Back)".
bool ContainsLine(const std::string &sText, const std::string &sLine)
{
  const std::string sTerminated = sLine + "\n";
  return sText.rfind(sTerminated, 0) == 0
      || Contains(sText, "\n" + sTerminated);
}

size_t CountLinesStartingWith(const std::string &sText,
                              const std::string &sPrefix)
{
  size_t uiCount = 0;
  size_t uiPos = 0;
  while (uiPos <= sText.size()) {
    const size_t uiEnd = sText.find('\n', uiPos);
    const std::string sLine = sText.substr(
        uiPos, uiEnd == std::string::npos ? std::string::npos : uiEnd - uiPos);
    if (sLine.compare(0, sPrefix.size(), sPrefix) == 0)
      ++uiCount;
    if (uiEnd == std::string::npos)
      break;
    uiPos = uiEnd + 1;
  }
  return uiCount;
}

// The face line "f a/a/a b/b/b c/c/c" as its three vertex indices.
std::vector<int> FaceIndices(const std::string &sText, size_t uiWhich)
{
  size_t uiPos = 0;
  size_t uiSeen = 0;
  while (uiPos <= sText.size()) {
    const size_t uiEnd = sText.find('\n', uiPos);
    const std::string sLine = sText.substr(
        uiPos, uiEnd == std::string::npos ? std::string::npos : uiEnd - uiPos);
    if (sLine.compare(0, 2, "f ") == 0) {
      if (uiSeen == uiWhich) {
        std::vector<int> Indices;
        std::istringstream Line(sLine.substr(2));
        std::string sCorner;
        while (Line >> sCorner)
          Indices.push_back(std::atoi(sCorner.c_str()));
        return Indices;
      }
      ++uiSeen;
    }
    if (uiEnd == std::string::npos)
      break;
    uiPos = uiEnd + 1;
  }
  return std::vector<int>();
}

void test_the_axis_conversion_matches_adr_0003()
{
  // ROLLER world is right-handed with +Z up; OBJ consumers want right-handed
  // +Y up, so the exporter rotates -90 degrees about X and scales by 1/100.
  const float afRoller[3] = { 100.0f, 200.0f, 300.0f };
  float afObj[3] = { 0.0f, 0.0f, 0.0f };
  CEditorExportConventions::ConvertPosition(afRoller, afObj);
  assert(std::fabs(afObj[0] - 1.0f) < 1e-6f);
  assert(std::fabs(afObj[1] - 3.0f) < 1e-6f);
  assert(std::fabs(afObj[2] + 2.0f) < 1e-6f);

  // Direction carries the same rotation without the scale, so a unit normal
  // stays unit length.
  const float afUp[3] = { 0.0f, 0.0f, 1.0f };
  CEditorExportConventions::ConvertDirection(afUp, afObj);
  assert(std::fabs(afObj[0]) < 1e-6f);
  assert(std::fabs(afObj[1] - 1.0f) < 1e-6f);
  assert(std::fabs(afObj[2]) < 1e-6f);

  // The mapping preserves handedness, which is why no winding flip is needed:
  // newX x newY must equal newZ.
  const float afX[3] = { 1.0f, 0.0f, 0.0f };
  const float afY[3] = { 0.0f, 1.0f, 0.0f };
  const float afZ[3] = { 0.0f, 0.0f, 1.0f };
  float afNewX[3], afNewY[3], afNewZ[3];
  CEditorExportConventions::ConvertDirection(afX, afNewX);
  CEditorExportConventions::ConvertDirection(afY, afNewY);
  CEditorExportConventions::ConvertDirection(afZ, afNewZ);
  const float afCross[3] = {
    afNewX[1] * afNewY[2] - afNewX[2] * afNewY[1],
    afNewX[2] * afNewY[0] - afNewX[0] * afNewY[2],
    afNewX[0] * afNewY[1] - afNewX[1] * afNewY[0]
  };
  for (int i = 0; i < 3; ++i)
    assert(std::fabs(afCross[i] - afNewZ[i]) < 1e-6f);
}

void test_the_import_conversion_is_the_exact_inverse_of_the_export()
{
  // E4-S6. The reference-model importer has to undo exactly what the exporter
  // did, or a track this editor exported comes back lying on its side. Two
  // halves of one mapping in two files is how they drift, so they live
  // together and this pins them to each other.
  const float aafRoller[][3] = {
    { 100.0f, 200.0f, 300.0f },
    { -4250.5f, 90000.0f, 14.25f },
    { 0.0f, 0.0f, 0.0f },
    { 1.0f, -1.0f, 1.0f }
  };
  for (size_t i = 0; i < sizeof(aafRoller) / sizeof(aafRoller[0]); ++i) {
    float afFile[3];
    float afBack[3];
    CEditorExportConventions::ConvertPosition(aafRoller[i], afFile);
    CEditorExportConventions::ImportPosition(afFile, afBack);
    for (int c = 0; c < 3; ++c)
      assert(std::fabs(afBack[c] - aafRoller[i][c]) < 1e-3f);

    CEditorExportConventions::ConvertDirection(aafRoller[i], afFile);
    CEditorExportConventions::ImportDirection(afFile, afBack);
    for (int c = 0; c < 3; ++c)
      assert(std::fabs(afBack[c] - aafRoller[i][c]) < 1e-6f);
  }

  // The import is a real rotation, not a copy: an OBJ's +Y is ROLLER's +Z.
  const float afObjUp[3] = { 0.0f, 1.0f, 0.0f };
  float afRollerUp[3];
  CEditorExportConventions::ImportDirection(afObjUp, afRollerUp);
  assert(std::fabs(afRollerUp[0]) < 1e-6f);
  assert(std::fabs(afRollerUp[1]) < 1e-6f);
  assert(std::fabs(afRollerUp[2] - 1.0f) < 1e-6f);

  // And it restores the unit scale, so a metre in the file is 100 track units.
  const float afObjMetre[3] = { 1.0f, 0.0f, 0.0f };
  float afRollerMetre[3];
  CEditorExportConventions::ImportPosition(afObjMetre, afRollerMetre);
  assert(std::fabs(afRollerMetre[0] - 100.0f) < 1e-3f);

  // Handedness survives the round trip, which is why neither direction flips
  // winding.
  const float afX[3] = { 1.0f, 0.0f, 0.0f };
  const float afY[3] = { 0.0f, 1.0f, 0.0f };
  const float afZ[3] = { 0.0f, 0.0f, 1.0f };
  float afNewX[3], afNewY[3], afNewZ[3];
  CEditorExportConventions::ImportDirection(afX, afNewX);
  CEditorExportConventions::ImportDirection(afY, afNewY);
  CEditorExportConventions::ImportDirection(afZ, afNewZ);
  const float afCross[3] = {
    afNewX[1] * afNewY[2] - afNewX[2] * afNewY[1],
    afNewX[2] * afNewY[0] - afNewX[0] * afNewY[2],
    afNewX[0] * afNewY[1] - afNewX[1] * afNewY[0]
  };
  for (int i = 0; i < 3; ++i)
    assert(std::fabs(afCross[i] - afNewZ[i]) < 1e-6f);
}

void test_every_named_surface_group_is_emitted()
{
  CExtractionBuilder Builder;
  const uint32_t uiMaterial = Builder.AddTexturedMaterial(
      ROLLER_ED_TEXTURE_SET_TRACK, 0.125f, 0.125f, 0.0f, 0.0f);
  for (uint16_t unClass = ROLLER_ED_SURFACE_CLASS_CENTER;
       unClass <= ROLLER_ED_SURFACE_CLASS_RIGHT_UPPER_OUTER_WALL; ++unClass) {
    Builder.AddQuad(unClass, ROLLER_ED_CONTENT_AUTHORED_TRACK, uiMaterial,
                    ROLLER_ED_INVALID_MATERIAL_ID);
  }

  tEdObjExportOptions Options = DefaultOptions();
  Options.bSeparateBackFaces = false;
  std::ostringstream Obj;
  std::ostringstream Mtl;
  std::string sError;
  assert(CEditorObjExporter::Export(Builder.View(), Options, nullptr, 0, Obj,
                                    Mtl, sError));
  const std::string sObj = Obj.str();

  // The names the pre-migration exporter used, unchanged.
  static const char *aszExpected[] = {
    "o Center", "o Left Shoulder", "o Right Shoulder", "o Left Wall",
    "o Right Wall", "o Roof", "o Outer Wall Floor", "o Left Lower Outer Wall",
    "o Right Lower Outer Wall", "o Left Upper Outer Wall",
    "o Right Upper Outer Wall"
  };
  for (size_t i = 0; i < sizeof(aszExpected) / sizeof(aszExpected[0]); ++i)
    assert(Contains(sObj, aszExpected[i]));
  assert(CountLinesStartingWith(sObj, "o ") == 11);
  assert(Contains(sObj, "mtllib TRACK3.mtl"));
}

void test_combined_sections_produce_one_object()
{
  CExtractionBuilder Builder;
  const uint32_t uiMaterial = Builder.AddTexturedMaterial(
      ROLLER_ED_TEXTURE_SET_TRACK, 0.125f, 0.125f, 0.0f, 0.0f);
  Builder.AddQuad(ROLLER_ED_SURFACE_CLASS_CENTER,
                  ROLLER_ED_CONTENT_AUTHORED_TRACK, uiMaterial,
                  ROLLER_ED_INVALID_MATERIAL_ID);
  Builder.AddQuad(ROLLER_ED_SURFACE_CLASS_ROOF,
                  ROLLER_ED_CONTENT_AUTHORED_TRACK, uiMaterial,
                  ROLLER_ED_INVALID_MATERIAL_ID);

  tEdObjExportOptions Options = DefaultOptions();
  Options.bSeparateSections = false;
  Options.bSeparateBackFaces = false;
  std::ostringstream Obj;
  std::ostringstream Mtl;
  std::string sError;
  assert(CEditorObjExporter::Export(Builder.View(), Options, nullptr, 0, Obj,
                                    Mtl, sError));
  const std::string sObj = Obj.str();
  assert(CountLinesStartingWith(sObj, "o ") == 1);
  assert(Contains(sObj, "o Track\n"));
  assert(CountLinesStartingWith(sObj, "f ") == 4);
}

void test_back_faces_use_the_back_material_and_reverse_the_winding()
{
  CExtractionBuilder Builder;
  // Deliberately different atlas rectangles: reusing the front material's
  // transform would sample the wrong tile whenever texture_back[] substitutes.
  const uint32_t uiFront = Builder.AddTexturedMaterial(
      ROLLER_ED_TEXTURE_SET_TRACK, 0.125f, 0.125f, 0.0f, 0.0f);
  const uint32_t uiBack = Builder.AddTexturedMaterial(
      ROLLER_ED_TEXTURE_SET_TRACK, 0.125f, 0.125f, 0.5f, 0.25f);
  Builder.AddQuad(ROLLER_ED_SURFACE_CLASS_CENTER,
                  ROLLER_ED_CONTENT_AUTHORED_TRACK, uiFront, uiBack);

  tEdObjExportOptions Options = DefaultOptions();
  std::ostringstream Obj;
  std::ostringstream Mtl;
  std::string sError;
  assert(CEditorObjExporter::Export(Builder.View(), Options, nullptr, 0, Obj,
                                    Mtl, sError));
  const std::string sObj = Obj.str();

  assert(Contains(sObj, "o Center\n"));
  assert(Contains(sObj, "o Center (Back)\n"));

  // Two triangles per side, and the back side is the same triangle wound the
  // other way.
  assert(CountLinesStartingWith(sObj, "f ") == 4);
  const std::vector<int> Front = FaceIndices(sObj, 0);
  const std::vector<int> Back = FaceIndices(sObj, 2);
  assert(Front.size() == 3 && Back.size() == 3);
  // Different objects, so the absolute indices differ; the ordering does not.
  const int iFrontBase = Front[0];
  const int iBackBase = Back[2];
  assert(Front[0] - iFrontBase == 0);
  assert(Front[1] - iFrontBase == 1);
  assert(Front[2] - iFrontBase == 2);
  assert(Back[0] - iBackBase == 2);
  assert(Back[1] - iBackBase == 1);
  assert(Back[2] - iBackBase == 0);

  // The back side's UVs come from the back material's bias without an extra
  // mirror: directional reverse textures are already authored for that side.
  assert(Contains(sObj, "vt 0.500000 0.750000"));
  assert(Contains(sObj, "vt 0.625000 0.750000"));
  // ROLLER's UV origin is top-left and OBJ's is bottom-left, so V is flipped.
  assert(Contains(sObj, "vt 0.000000 1.000000"));
}

void test_a_two_sided_surface_without_a_back_material_still_gets_a_back()
{
  // ADR 0003: SURFACE_FLAG_CONCAVE makes the renderer bypass its facing test,
  // so the surface is visible from both sides even though texture_back[]
  // substitutes nothing. Exporting it single-sided would leave a hole.
  CExtractionBuilder Builder;
  const uint32_t uiMaterial = Builder.AddTexturedMaterial(
      ROLLER_ED_TEXTURE_SET_TRACK, 0.125f, 0.125f, 0.0f, 0.0f);
  Builder.AddQuad(ROLLER_ED_SURFACE_CLASS_ROOF,
                  ROLLER_ED_CONTENT_AUTHORED_TRACK, uiMaterial,
                  ROLLER_ED_INVALID_MATERIAL_ID,
                  ROLLER_ED_PRIMITIVE_FLAG_TWO_SIDED);

  std::ostringstream Obj;
  std::ostringstream Mtl;
  std::string sError;
  assert(CEditorObjExporter::Export(Builder.View(), DefaultOptions(), nullptr,
                                    0, Obj, Mtl, sError));
  const std::string sObj = Obj.str();
  assert(Contains(sObj, "o Roof (Back)\n"));
  assert(CountLinesStartingWith(sObj, "f ") == 4);
  // The reverse side points the other way.
  assert(Contains(sObj, "vn 0.000000 1.000000 0.000000"));
  assert(Contains(sObj, "vn 0.000000 -1.000000 0.000000"));
}

void test_complete_reverse_geometry_doubles_every_quad()
{
  CExtractionBuilder Builder;
  const uint32_t uiMaterial = Builder.AddTexturedMaterial(
      ROLLER_ED_TEXTURE_SET_TRACK, 0.125f, 0.125f, 0.25f, 0.0f);
  // No alternate material and no two-sided source flag: the complete export
  // still has to close this quad from the reverse side.
  Builder.AddQuad(ROLLER_ED_SURFACE_CLASS_CENTER,
                  ROLLER_ED_CONTENT_AUTHORED_TRACK, uiMaterial,
                  ROLLER_ED_INVALID_MATERIAL_ID);

  tEdObjExportOptions Options = DefaultOptions();
  Options.bCompleteReverseGeometry = true;
  std::ostringstream Obj;
  std::ostringstream Mtl;
  std::string sError;
  assert(CEditorObjExporter::Export(Builder.View(), Options, nullptr, 0, Obj,
                                    Mtl, sError));
  const std::string sObj = Obj.str();
  assert(ContainsLine(sObj, "o Center"));
  assert(ContainsLine(sObj, "o Center (Back)"));
  assert(CountLinesStartingWith(sObj, "f ") == 4);
  // With no authored alternate, both sides repeat the same atlas rectangle.
  assert(CountLinesStartingWith(sObj, "vt 0.250000 1.000000") == 2);
}

void test_merged_backs_land_in_the_front_object()
{
  CExtractionBuilder Builder;
  const uint32_t uiFront = Builder.AddTexturedMaterial(
      ROLLER_ED_TEXTURE_SET_TRACK, 0.125f, 0.125f, 0.0f, 0.0f);
  const uint32_t uiBack = Builder.AddTexturedMaterial(
      ROLLER_ED_TEXTURE_SET_TRACK, 0.125f, 0.125f, 0.5f, 0.0f);
  Builder.AddQuad(ROLLER_ED_SURFACE_CLASS_CENTER,
                  ROLLER_ED_CONTENT_AUTHORED_TRACK, uiFront, uiBack);

  tEdObjExportOptions Options = DefaultOptions();
  Options.bSeparateBackFaces = false;
  std::ostringstream Obj;
  std::ostringstream Mtl;
  std::string sError;
  assert(CEditorObjExporter::Export(Builder.View(), Options, nullptr, 0, Obj,
                                    Mtl, sError));
  const std::string sObj = Obj.str();
  // Reverse-side faces are always generated; the option only chooses whether
  // they get their own object, exactly as eBackModeling did.
  assert(!Contains(sObj, "(Back)"));
  assert(CountLinesStartingWith(sObj, "f ") == 4);
}

void test_uvs_resolve_through_the_material_atlas_transform()
{
  CExtractionBuilder Builder;
  // Tile 5 of an 8x8 atlas: the exporter must use this rectangle rather than
  // do tile arithmetic of its own (AD-7b).
  const uint32_t uiMaterial = Builder.AddTexturedMaterial(
      ROLLER_ED_TEXTURE_SET_TRACK, 0.125f, 0.125f, 0.625f, 0.0f);
  Builder.AddQuad(ROLLER_ED_SURFACE_CLASS_CENTER,
                  ROLLER_ED_CONTENT_AUTHORED_TRACK, uiMaterial,
                  ROLLER_ED_INVALID_MATERIAL_ID);

  tEdObjExportOptions Options = DefaultOptions();
  Options.bSeparateBackFaces = false;
  std::ostringstream Obj;
  std::ostringstream Mtl;
  std::string sError;
  assert(CEditorObjExporter::Export(Builder.View(), Options, nullptr, 0, Obj,
                                    Mtl, sError));
  const std::string sObj = Obj.str();
  // u = 0 * 0.125 + 0.625, v = 1 - (0 * 0.125 + 0) = 1
  assert(Contains(sObj, "vt 0.625000 1.000000"));
  // u = 1 * 0.125 + 0.625 = 0.75, v = 1 - 0.125 = 0.875
  assert(Contains(sObj, "vt 0.750000 0.875000"));
}

void test_texture_sets_choose_their_own_atlas_png()
{
  CExtractionBuilder Builder;
  const uint32_t uiTrack = Builder.AddTexturedMaterial(
      ROLLER_ED_TEXTURE_SET_TRACK, 0.125f, 0.125f, 0.0f, 0.0f);
  const uint32_t uiSign = Builder.AddTexturedMaterial(
      ROLLER_ED_TEXTURE_SET_BUILDING_SIGN, 0.125f, 0.125f, 0.0f, 0.0f);
  Builder.AddQuad(ROLLER_ED_SURFACE_CLASS_CENTER,
                  ROLLER_ED_CONTENT_AUTHORED_TRACK, uiTrack,
                  ROLLER_ED_INVALID_MATERIAL_ID);
  Builder.AddQuad(ROLLER_ED_SURFACE_CLASS_ROOF,
                  ROLLER_ED_CONTENT_AUTHORED_TRACK, uiSign,
                  ROLLER_ED_INVALID_MATERIAL_ID);

  std::ostringstream Obj;
  std::ostringstream Mtl;
  std::string sError;
  assert(CEditorObjExporter::Export(Builder.View(), DefaultOptions(), nullptr,
                                    0, Obj, Mtl, sError));
  const std::string sMtl = Mtl.str();
  assert(Contains(sMtl, "newmtl TRACK3\nmap_Kd TRACK3.png\nmap_d TRACK3.png"));
  assert(Contains(sMtl,
                  "newmtl TRACK3_BLD\nmap_Kd TRACK3_BLD.png\n"
                  "map_d TRACK3_BLD.png"));
  assert(Contains(Obj.str(), "usemtl TRACK3\n"));
  assert(Contains(Obj.str(), "usemtl TRACK3_BLD\n"));
}

void test_a_texture_set_declares_one_material_however_many_tiles_use_it()
{
  CExtractionBuilder Builder;
  const uint32_t uiTileA = Builder.AddTexturedMaterial(
      ROLLER_ED_TEXTURE_SET_TRACK, 0.125f, 0.125f, 0.0f, 0.0f);
  const uint32_t uiTileB = Builder.AddTexturedMaterial(
      ROLLER_ED_TEXTURE_SET_TRACK, 0.125f, 0.125f, 0.25f, 0.5f);
  Builder.AddQuad(ROLLER_ED_SURFACE_CLASS_CENTER,
                  ROLLER_ED_CONTENT_AUTHORED_TRACK, uiTileA,
                  ROLLER_ED_INVALID_MATERIAL_ID);
  Builder.AddQuad(ROLLER_ED_SURFACE_CLASS_CENTER,
                  ROLLER_ED_CONTENT_AUTHORED_TRACK, uiTileB,
                  ROLLER_ED_INVALID_MATERIAL_ID);

  std::ostringstream Obj;
  std::ostringstream Mtl;
  std::string sError;
  assert(CEditorObjExporter::Export(Builder.View(), DefaultOptions(), nullptr,
                                    0, Obj, Mtl, sError));
  assert(CountLinesStartingWith(Mtl.str(), "newmtl ") == 1);
}

void test_flat_palette_colours_become_diffuse_materials()
{
  CExtractionBuilder Builder;
  const uint32_t uiFlat = Builder.AddFlatMaterial(7);
  Builder.AddQuad(ROLLER_ED_SURFACE_CLASS_OUTER_WALL_FLOOR,
                  ROLLER_ED_CONTENT_AUTHORED_TRACK, uiFlat,
                  ROLLER_ED_INVALID_MATERIAL_ID);

  std::vector<tEdExportPaletteEntry> Palette(256);
  Palette[7].byRed = 255;
  Palette[7].byGreen = 0;
  Palette[7].byBlue = 51;

  std::ostringstream Obj;
  std::ostringstream Mtl;
  std::string sError;
  assert(CEditorObjExporter::Export(Builder.View(), DefaultOptions(),
                                    Palette.data(), 256, Obj, Mtl, sError));
  const std::string sMtl = Mtl.str();
  assert(Contains(sMtl, "newmtl TRACK3_color_7"));
  assert(Contains(sMtl, "Kd 1.000000 0.000000 0.200000"));
  // A flat colour is never given a texture map.
  assert(!Contains(sMtl, "map_Kd"));
}

void test_screen_darken_is_a_documented_transparent_material()
{
  // Spec open item 2b. A static format cannot reproduce a framebuffer
  // darkening operation, so the level becomes a black material whose dissolve
  // leaves the same fraction of what is behind it as the renderer does.
  CExtractionBuilder Builder;
  const uint32_t uiDarken = Builder.AddDarkenMaterial(2);
  Builder.AddQuad(ROLLER_ED_SURFACE_CLASS_LEFT_WALL,
                  ROLLER_ED_CONTENT_AUTHORED_TRACK, uiDarken,
                  ROLLER_ED_INVALID_MATERIAL_ID);

  std::ostringstream Obj;
  std::ostringstream Mtl;
  std::string sError;
  assert(CEditorObjExporter::Export(Builder.View(), DefaultOptions(), nullptr,
                                    0, Obj, Mtl, sError));
  const std::string sMtl = Mtl.str();
  assert(Contains(sMtl, "newmtl TRACK3_darken_2"));
  assert(Contains(sMtl, "Kd 0.000000 0.000000 0.000000"));
  assert(Contains(sMtl, "d 0.600000"));
  // Never an ordinary texture.
  assert(!Contains(sMtl, "map_Kd"));

  assert(std::fabs(CEditorExportConventions::ScreenDarkenAlpha(0) - 0.2f) < 1e-6f);
  assert(std::fabs(CEditorExportConventions::ScreenDarkenAlpha(3) - 0.8f) < 1e-6f);
  // Levels past the table clamp rather than read out of bounds.
  assert(std::fabs(CEditorExportConventions::ScreenDarkenAlpha(4)
                   - CEditorExportConventions::ScreenDarkenAlpha(99)) < 1e-6f);
}

// E4A-S6. ROLLER now publishes advert panels and buildings, and the wizard's
// Include-signs checkbox finally has something to switch off.
void test_signs_and_scenery_get_their_own_named_objects()
{
  CExtractionBuilder Builder;
  const uint32_t uiTrack = Builder.AddTexturedMaterial(
      ROLLER_ED_TEXTURE_SET_TRACK, 0.125f, 0.125f, 0.0f, 0.0f);
  const uint32_t uiBld = Builder.AddTexturedMaterial(
      ROLLER_ED_TEXTURE_SET_BUILDING_SIGN, 0.25f, 0.25f, 0.0f, 0.0f);
  Builder.AddQuad(ROLLER_ED_SURFACE_CLASS_CENTER,
                  ROLLER_ED_CONTENT_AUTHORED_TRACK, uiTrack,
                  ROLLER_ED_INVALID_MATERIAL_ID);
  // Every building plan carries at most one real-sign polygon, so a sign
  // primitive and an advert panel are the same thing: one object each.
  Builder.AddQuad(ROLLER_ED_SURFACE_CLASS_SIGN,
                  ROLLER_ED_CONTENT_AUTHORED_SIGN, uiBld,
                  ROLLER_ED_INVALID_MATERIAL_ID);
  Builder.AddQuad(ROLLER_ED_SURFACE_CLASS_SIGN,
                  ROLLER_ED_CONTENT_AUTHORED_SIGN, uiBld,
                  ROLLER_ED_INVALID_MATERIAL_ID);
  // Buildings share one group: the canonical stream publishes no per-object
  // identity to split them on.
  Builder.AddQuad(ROLLER_ED_SURFACE_CLASS_BUILDING,
                  ROLLER_ED_CONTENT_AUTHORED_SCENERY, uiBld,
                  ROLLER_ED_INVALID_MATERIAL_ID);
  Builder.AddQuad(ROLLER_ED_SURFACE_CLASS_BUILDING,
                  ROLLER_ED_CONTENT_AUTHORED_SCENERY, uiBld,
                  ROLLER_ED_INVALID_MATERIAL_ID);

  tEdObjExportOptions Options = DefaultOptions();
  Options.bSeparateBackFaces = false;
  std::ostringstream Obj;
  std::ostringstream Mtl;
  std::string sError;
  assert(CEditorObjExporter::Export(Builder.View(), Options, nullptr, 0, Obj,
                                    Mtl, sError));
  const std::string sObj = Obj.str();

  // The legacy FBX exporter's node names, restored.
  assert(ContainsLine(sObj, "o Sign 0"));
  assert(ContainsLine(sObj, "o Sign 1"));
  assert(!ContainsLine(sObj, "o Sign 2"));
  assert(ContainsLine(sObj, "o Scenery"));
  assert(ContainsLine(sObj, "o Center"));
  // Track, two signs, one scenery group.
  assert(CountLinesStartingWith(sObj, "o ") == 4);
  assert(CountLinesStartingWith(sObj, "f ") == 10);
  // Signs and buildings address the building/sign atlas, so E4-S4's
  // <name>_BLD.png stops being an unreferenced file.
  assert(ContainsLine(Mtl.str(), "map_Kd TRACK3_BLD.png"));

  // Grouping is dispatched on the content class, never on the surface class
  // (AD-8): a sign wearing a track surface class is still a sign.
  CExtractionBuilder Mislabelled;
  const uint32_t uiOther = Mislabelled.AddTexturedMaterial(
      ROLLER_ED_TEXTURE_SET_BUILDING_SIGN, 0.25f, 0.25f, 0.0f, 0.0f);
  Mislabelled.AddQuad(ROLLER_ED_SURFACE_CLASS_CENTER,
                      ROLLER_ED_CONTENT_AUTHORED_SIGN, uiOther,
                      ROLLER_ED_INVALID_MATERIAL_ID);
  std::ostringstream Obj2;
  std::ostringstream Mtl2;
  assert(CEditorObjExporter::Export(Mislabelled.View(), Options, nullptr, 0,
                                    Obj2, Mtl2, sError));
  assert(ContainsLine(Obj2.str(), "o Sign 0"));
  assert(!ContainsLine(Obj2.str(), "o Center"));
}

void test_the_sign_option_removes_signs_and_scenery_but_keeps_the_track()
{
  CExtractionBuilder Builder;
  const uint32_t uiTrack = Builder.AddTexturedMaterial(
      ROLLER_ED_TEXTURE_SET_TRACK, 0.125f, 0.125f, 0.0f, 0.0f);
  const uint32_t uiBld = Builder.AddTexturedMaterial(
      ROLLER_ED_TEXTURE_SET_BUILDING_SIGN, 0.25f, 0.25f, 0.0f, 0.0f);
  Builder.AddQuad(ROLLER_ED_SURFACE_CLASS_CENTER,
                  ROLLER_ED_CONTENT_AUTHORED_TRACK, uiTrack,
                  ROLLER_ED_INVALID_MATERIAL_ID);
  Builder.AddQuad(ROLLER_ED_SURFACE_CLASS_SIGN,
                  ROLLER_ED_CONTENT_AUTHORED_SIGN, uiBld,
                  ROLLER_ED_INVALID_MATERIAL_ID);
  Builder.AddQuad(ROLLER_ED_SURFACE_CLASS_BUILDING,
                  ROLLER_ED_CONTENT_AUTHORED_SCENERY, uiBld,
                  ROLLER_ED_INVALID_MATERIAL_ID);

  tEdObjExportOptions Options = DefaultOptions();
  Options.bSeparateBackFaces = false;
  Options.bExportScenery = false;
  std::ostringstream Obj;
  std::ostringstream Mtl;
  std::string sError;
  assert(CEditorObjExporter::Export(Builder.View(), Options, nullptr, 0, Obj,
                                    Mtl, sError));
  const std::string sObj = Obj.str();
  // Unticking the box reproduces exactly what E4-S1 through E4-S5 exported.
  assert(CountLinesStartingWith(sObj, "o ") == 1);
  assert(ContainsLine(sObj, "o Center"));
  assert(!ContainsLine(sObj, "o Sign 0"));
  assert(!ContainsLine(sObj, "o Scenery"));
  assert(CountLinesStartingWith(sObj, "f ") == 2);
}

void test_sign_backs_are_numbered_with_their_fronts()
{
  CExtractionBuilder Builder;
  const uint32_t uiFront = Builder.AddTexturedMaterial(
      ROLLER_ED_TEXTURE_SET_BUILDING_SIGN, 0.25f, 0.25f, 0.0f, 0.0f);
  const uint32_t uiBack = Builder.AddTexturedMaterial(
      ROLLER_ED_TEXTURE_SET_BUILDING_SIGN, 0.25f, 0.25f, 0.25f, 0.0f);
  Builder.AddQuad(ROLLER_ED_SURFACE_CLASS_SIGN,
                  ROLLER_ED_CONTENT_AUTHORED_SIGN, uiFront, uiBack);
  Builder.AddQuad(ROLLER_ED_SURFACE_CLASS_SIGN,
                  ROLLER_ED_CONTENT_AUTHORED_SIGN, uiFront, uiBack);

  tEdObjExportOptions Options = DefaultOptions();
  std::ostringstream Obj;
  std::ostringstream Mtl;
  std::string sError;
  assert(CEditorObjExporter::Export(Builder.View(), Options, nullptr, 0, Obj,
                                    Mtl, sError));
  const std::string sObj = Obj.str();
  assert(ContainsLine(sObj, "o Sign 0"));
  assert(ContainsLine(sObj, "o Sign 1"));
  assert(ContainsLine(sObj, "o Sign 0 (Back)"));
  assert(ContainsLine(sObj, "o Sign 1 (Back)"));
  assert(CountLinesStartingWith(sObj, "o ") == 4);

  // Merged backs land in the panel's own object rather than a fifth one.
  Options.bSeparateBackFaces = false;
  std::ostringstream Merged;
  std::ostringstream MergedMtl;
  assert(CEditorObjExporter::Export(Builder.View(), Options, nullptr, 0,
                                    Merged, MergedMtl, sError));
  assert(CountLinesStartingWith(Merged.str(), "o ") == 2);
  assert(!ContainsLine(Merged.str(), "o Sign 0 (Back)"));
  assert(CountLinesStartingWith(Merged.str(), "f ") == 8);
}

void test_runtime_scenery_never_reaches_the_export()
{
  // AD-6d/AD-6e: the export is authored content, filtered on the content
  // class the producer published rather than inferred from anything else.
  CExtractionBuilder Builder;
  const uint32_t uiMaterial = Builder.AddTexturedMaterial(
      ROLLER_ED_TEXTURE_SET_TRACK, 0.125f, 0.125f, 0.0f, 0.0f);
  Builder.AddQuad(ROLLER_ED_SURFACE_CLASS_CENTER,
                  ROLLER_ED_CONTENT_RUNTIME_SCENERY, uiMaterial,
                  ROLLER_ED_INVALID_MATERIAL_ID);
  Builder.AddQuad(ROLLER_ED_SURFACE_CLASS_CENTER,
                  ROLLER_ED_CONTENT_AUTHORED_TRACK, uiMaterial,
                  ROLLER_ED_INVALID_MATERIAL_ID);

  tEdObjExportOptions Options = DefaultOptions();
  Options.bSeparateBackFaces = false;
  std::ostringstream Obj;
  std::ostringstream Mtl;
  std::string sError;
  assert(CEditorObjExporter::Export(Builder.View(), Options, nullptr, 0, Obj,
                                    Mtl, sError));
  // Only the authored quad survives.
  assert(CountLinesStartingWith(Obj.str(), "f ") == 2);

  assert(CEditorExportConventions::IsAuthoredContent(
      ROLLER_ED_CONTENT_AUTHORED_TRACK));
  assert(CEditorExportConventions::IsAuthoredContent(
      ROLLER_ED_CONTENT_AUTHORED_SIGN));
  assert(CEditorExportConventions::IsAuthoredContent(
      ROLLER_ED_CONTENT_AUTHORED_SCENERY));
  assert(!CEditorExportConventions::IsAuthoredContent(
      ROLLER_ED_CONTENT_RUNTIME_SCENERY));
}

void test_an_inconsistent_extraction_is_refused()
{
  CExtractionBuilder Builder;
  const uint32_t uiMaterial = Builder.AddTexturedMaterial(
      ROLLER_ED_TEXTURE_SET_TRACK, 0.125f, 0.125f, 0.0f, 0.0f);
  Builder.AddQuad(ROLLER_ED_SURFACE_CLASS_CENTER,
                  ROLLER_ED_CONTENT_AUTHORED_TRACK, uiMaterial,
                  ROLLER_ED_INVALID_MATERIAL_ID);
  // A material id the extraction does not contain.
  Builder.m_Primitives[0].uiFrontMaterialId = 9;

  std::ostringstream Obj;
  std::ostringstream Mtl;
  std::string sError;
  assert(!CEditorObjExporter::Export(Builder.View(), DefaultOptions(), nullptr,
                                     0, Obj, Mtl, sError));
  assert(!sError.empty());

  // An empty extraction is refused rather than written as an empty file.
  CExtractionBuilder Empty;
  std::ostringstream EmptyObj;
  std::ostringstream EmptyMtl;
  assert(!CEditorObjExporter::Export(Empty.View(), DefaultOptions(), nullptr, 0,
                                     EmptyObj, EmptyMtl, sError));
  assert(!sError.empty());
}

void test_vertex_indices_are_continuous_across_objects()
{
  CExtractionBuilder Builder;
  const uint32_t uiMaterial = Builder.AddTexturedMaterial(
      ROLLER_ED_TEXTURE_SET_TRACK, 0.125f, 0.125f, 0.0f, 0.0f);
  Builder.AddQuad(ROLLER_ED_SURFACE_CLASS_CENTER,
                  ROLLER_ED_CONTENT_AUTHORED_TRACK, uiMaterial,
                  ROLLER_ED_INVALID_MATERIAL_ID);
  Builder.AddQuad(ROLLER_ED_SURFACE_CLASS_ROOF,
                  ROLLER_ED_CONTENT_AUTHORED_TRACK, uiMaterial,
                  ROLLER_ED_INVALID_MATERIAL_ID);

  tEdObjExportOptions Options = DefaultOptions();
  Options.bSeparateBackFaces = false;
  std::ostringstream Obj;
  std::ostringstream Mtl;
  std::string sError;
  assert(CEditorObjExporter::Export(Builder.View(), Options, nullptr, 0, Obj,
                                    Mtl, sError));
  const std::string sObj = Obj.str();

  // OBJ indices are file-global and one-based; the second object continues
  // where the first left off.
  assert(CountLinesStartingWith(sObj, "v ") == 8);
  assert(CountLinesStartingWith(sObj, "vt ") == 8);
  assert(CountLinesStartingWith(sObj, "vn ") == 8);
  const std::vector<int> First = FaceIndices(sObj, 0);
  const std::vector<int> Third = FaceIndices(sObj, 2);
  assert(First[0] == 1);
  assert(Third[0] == 5);
}
}

int main()
{
  test_the_axis_conversion_matches_adr_0003();
  test_the_import_conversion_is_the_exact_inverse_of_the_export();
  test_every_named_surface_group_is_emitted();
  test_combined_sections_produce_one_object();
  test_back_faces_use_the_back_material_and_reverse_the_winding();
  test_a_two_sided_surface_without_a_back_material_still_gets_a_back();
  test_complete_reverse_geometry_doubles_every_quad();
  test_merged_backs_land_in_the_front_object();
  test_uvs_resolve_through_the_material_atlas_transform();
  test_texture_sets_choose_their_own_atlas_png();
  test_a_texture_set_declares_one_material_however_many_tiles_use_it();
  test_flat_palette_colours_become_diffuse_materials();
  test_screen_darken_is_a_documented_transparent_material();
  test_signs_and_scenery_get_their_own_named_objects();
  test_the_sign_option_removes_signs_and_scenery_but_keeps_the_track();
  test_sign_backs_are_numbered_with_their_fronts();
  test_runtime_scenery_never_reaches_the_export();
  test_an_inconsistent_extraction_is_refused();
  test_vertex_indices_are_continuous_across_objects();
  std::puts("editor OBJ exporter tests passed");
  return 0;
}
