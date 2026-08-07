// E4-S2. The glTF exporter is a pure function of one canonical extraction, so
// it is exercised here without a render worker, a loaded track, or Qt.
//
// Assertions are made against the document read back through cgltf's own
// parser rather than against the JSON text: a structural round trip plus
// cgltf_validate catches the mistakes that matter (dangling indices, wrong
// component types, missing accessor bounds) where a substring match would not.
//
// This also writes the sample files tests/check_gltf_in_blender.py imports,
// which is why it runs from the binary directory.
#include "EditorGltfExporter.h"
#include "EditorExportCommon.h"

#include "cgltf.h"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
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
class CExtractionBuilder
{
public:
  uint32_t AddTexturedMaterial(uint32_t uiTextureSet, uint32_t uiFlags,
                               float fBiasU, float fBiasV)
  {
    tEdMaterial Material = {};
    Material.uiKind = ROLLER_ED_MATERIAL_TEXTURED_TILE;
    Material.uiTextureSet = uiTextureSet;
    Material.uiFlags = uiFlags;
    Material.fAtlasScale[0] = 0.125f;
    Material.fAtlasScale[1] = 0.125f;
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

  void AddQuad(uint16_t unSurfaceClass, uint16_t unContentClass,
               uint32_t uiFrontMaterial, uint32_t uiBackMaterial,
               uint16_t unFlags = 0)
  {
    const uint32_t uiFirstVertex = static_cast<uint32_t>(m_Vertices.size());
    for (int i = 0; i < 4; ++i) {
      tEdVertex Vertex = {};
      Vertex.fPosition[0] = (i == 1 || i == 2) ? 100.0f : 0.0f;
      Vertex.fPosition[1] = (i >= 2) ? 200.0f : 0.0f;
      Vertex.fPosition[2] = 300.0f;
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

    const uint32_t auiOrder[6] = { 0, 1, 2, 0, 2, 3 };
    for (int i = 0; i < 6; ++i)
      m_Indices.push_back(uiFirstVertex + auiOrder[i]);
    m_Primitives.push_back(Primitive);
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

// Four bytes that stand in for a PNG. Nothing under test decodes them; they
// only have to survive the trip into the binary chunk.
std::vector<uint8_t> FakePng()
{
  return std::vector<uint8_t>{ 0x89, 'P', 'N', 'G', 0x0d };
}

tEdGltfExportOptions DefaultOptions()
{
  tEdGltfExportOptions Options;
  Options.sBaseName = "TRACK3";
  Options.sBufferUri = "TRACK3.bin";
  tEdGltfTextureSource Track;
  Track.uiTextureSet = ROLLER_ED_TEXTURE_SET_TRACK;
  Track.sUri = "TRACK3.png";
  Track.PngBytes = FakePng();
  Options.Textures.push_back(Track);
  tEdGltfTextureSource Sign;
  Sign.uiTextureSet = ROLLER_ED_TEXTURE_SET_BUILDING_SIGN;
  Sign.sUri = "TRACK3_BLD.png";
  Sign.PngBytes = FakePng();
  Options.Textures.push_back(Sign);
  return Options;
}

// Parses the emitted document the way an importer would. The caller owns the
// returned data and must free it with cgltf_free.
cgltf_data *ParseGltf(const tEdGltfExportOutput &Output, bool bBinary)
{
  cgltf_options Options;
  std::memset(&Options, 0, sizeof(Options));
  cgltf_data *pData = nullptr;
  cgltf_result eResult;
  if (bBinary) {
    // Re-pack the container exactly as ExportToFiles does, then parse it.
    assert(false && "binary parsing goes through ParseGlb");
    return nullptr;
  }
  eResult = cgltf_parse(&Options, Output.sJson.data(), Output.sJson.size(),
                        &pData);
  assert(eResult == cgltf_result_success);
  assert(cgltf_validate(pData) == cgltf_result_success);
  return pData;
}

const cgltf_mesh *FindMesh(const cgltf_data *pData, const char *szName)
{
  for (cgltf_size i = 0; i < pData->meshes_count; ++i) {
    if (pData->meshes[i].name && std::strcmp(pData->meshes[i].name, szName) == 0)
      return &pData->meshes[i];
  }
  return nullptr;
}

const cgltf_accessor *FindAttribute(const cgltf_primitive *pPrimitive,
                                    cgltf_attribute_type eType)
{
  for (cgltf_size i = 0; i < pPrimitive->attributes_count; ++i) {
    if (pPrimitive->attributes[i].type == eType)
      return pPrimitive->attributes[i].data;
  }
  return nullptr;
}

uint32_t ReadUint32(const std::vector<uint8_t> &Bytes, size_t uiOffset)
{
  return static_cast<uint32_t>(Bytes[uiOffset])
      | (static_cast<uint32_t>(Bytes[uiOffset + 1]) << 8)
      | (static_cast<uint32_t>(Bytes[uiOffset + 2]) << 16)
      | (static_cast<uint32_t>(Bytes[uiOffset + 3]) << 24);
}

void test_the_document_parses_and_validates()
{
  CExtractionBuilder Builder;
  const uint32_t uiMaterial =
      Builder.AddTexturedMaterial(ROLLER_ED_TEXTURE_SET_TRACK, 0, 0.0f, 0.0f);
  for (uint32_t c = 0;
       c < CEditorExportConventions::ExportedSurfaceClassCount(); ++c) {
    Builder.AddQuad(CEditorExportConventions::ExportedSurfaceClass(c),
                    ROLLER_ED_CONTENT_AUTHORED_TRACK, uiMaterial,
                    ROLLER_ED_INVALID_MATERIAL_ID);
  }

  tEdGltfExportOutput Output;
  std::string sError;
  assert(CEditorGltfExporter::Export(Builder.View(), DefaultOptions(), nullptr,
                                     0, Output, sError));
  cgltf_data *pData = ParseGltf(Output, false);

  // One mesh and one node per surface class, each named as the pre-migration
  // exporter named its objects, so a Blender import produces recognisable
  // objects rather than "mesh.001".
  assert(pData->meshes_count
         == CEditorExportConventions::ExportedSurfaceClassCount());
  assert(pData->nodes_count == pData->meshes_count);
  assert(pData->scenes_count == 1);
  assert(pData->scene == &pData->scenes[0]);
  assert(pData->scenes[0].nodes_count == pData->nodes_count);
  for (uint32_t c = 0;
       c < CEditorExportConventions::ExportedSurfaceClassCount(); ++c) {
    const char *szName = CEditorExportConventions::SurfaceClassName(
        CEditorExportConventions::ExportedSurfaceClass(c));
    const cgltf_mesh *pMesh = FindMesh(pData, szName);
    assert(pMesh != nullptr);
    assert(pMesh->primitives_count == 1);
  }
  for (cgltf_size n = 0; n < pData->nodes_count; ++n) {
    assert(pData->nodes[n].mesh != nullptr);
    assert(pData->nodes[n].name != nullptr);
    assert(std::strcmp(pData->nodes[n].name, pData->nodes[n].mesh->name) == 0);
  }
  cgltf_free(pData);
}

void test_attributes_and_indices_use_the_declared_types()
{
  CExtractionBuilder Builder;
  const uint32_t uiMaterial =
      Builder.AddTexturedMaterial(ROLLER_ED_TEXTURE_SET_TRACK, 0, 0.0f, 0.0f);
  Builder.AddQuad(ROLLER_ED_SURFACE_CLASS_CENTER,
                  ROLLER_ED_CONTENT_AUTHORED_TRACK, uiMaterial,
                  ROLLER_ED_INVALID_MATERIAL_ID);

  tEdGltfExportOutput Output;
  std::string sError;
  assert(CEditorGltfExporter::Export(Builder.View(), DefaultOptions(), nullptr,
                                     0, Output, sError));
  cgltf_data *pData = ParseGltf(Output, false);

  const cgltf_mesh *pMesh = FindMesh(pData, "Center");
  assert(pMesh != nullptr);
  const cgltf_primitive *pPrimitive = &pMesh->primitives[0];
  assert(pPrimitive->type == cgltf_primitive_type_triangles);

  const cgltf_accessor *pPosition =
      FindAttribute(pPrimitive, cgltf_attribute_type_position);
  const cgltf_accessor *pNormal =
      FindAttribute(pPrimitive, cgltf_attribute_type_normal);
  const cgltf_accessor *pTexCoord =
      FindAttribute(pPrimitive, cgltf_attribute_type_texcoord);
  assert(pPosition && pNormal && pTexCoord);
  assert(pPosition->type == cgltf_type_vec3);
  assert(pPosition->component_type == cgltf_component_type_r_32f);
  assert(pPosition->count == 4);
  assert(pNormal->type == cgltf_type_vec3);
  assert(pTexCoord->type == cgltf_type_vec2);
  // POSITION is the one accessor glTF requires bounds on.
  assert(pPosition->has_min && pPosition->has_max);

  assert(pPrimitive->indices != nullptr);
  assert(pPrimitive->indices->type == cgltf_type_scalar);
  assert(pPrimitive->indices->component_type == cgltf_component_type_r_32u);
  assert(pPrimitive->indices->count == 6);

  // Every accessor's view must start on a four-byte boundary or a viewer may
  // not be able to map it.
  for (cgltf_size i = 0; i < pData->buffer_views_count; ++i)
    assert((pData->buffer_views[i].offset % 4) == 0);
  cgltf_free(pData);
}

void test_the_axis_conversion_reaches_the_buffer()
{
  CExtractionBuilder Builder;
  const uint32_t uiMaterial =
      Builder.AddTexturedMaterial(ROLLER_ED_TEXTURE_SET_TRACK, 0, 0.0f, 0.0f);
  Builder.AddQuad(ROLLER_ED_SURFACE_CLASS_CENTER,
                  ROLLER_ED_CONTENT_AUTHORED_TRACK, uiMaterial,
                  ROLLER_ED_INVALID_MATERIAL_ID);

  tEdGltfExportOutput Output;
  std::string sError;
  assert(CEditorGltfExporter::Export(Builder.View(), DefaultOptions(), nullptr,
                                     0, Output, sError));

  // The first run in the buffer is the first batch's positions. ROLLER's
  // (100, 200, 300) becomes (1, 3, -2): +Z up to +Y up, scaled by 1/100.
  assert(Output.Binary.size() >= 4 * 3 * sizeof(float));
  const float *pPositions =
      reinterpret_cast<const float *>(Output.Binary.data());
  // Vertex 1 of the quad is the one with a non-zero X.
  assert(std::fabs(pPositions[3 + 0] - 1.0f) < 1e-6f);
  assert(std::fabs(pPositions[3 + 1] - 3.0f) < 1e-6f);
  assert(std::fabs(pPositions[3 + 2] - 0.0f) < 1e-6f);
  // Vertex 2 carries the +Y that becomes -Z.
  assert(std::fabs(pPositions[6 + 2] + 2.0f) < 1e-6f);
}

void test_uvs_are_not_flipped_the_way_obj_flips_them()
{
  // glTF's UV origin is top-left, the same as ROLLER's, so unlike OBJ there is
  // no V flip: the resolved atlas coordinate goes straight into the buffer.
  CExtractionBuilder Builder;
  const uint32_t uiMaterial = Builder.AddTexturedMaterial(
      ROLLER_ED_TEXTURE_SET_TRACK, 0, 0.625f, 0.0f);
  Builder.AddQuad(ROLLER_ED_SURFACE_CLASS_CENTER,
                  ROLLER_ED_CONTENT_AUTHORED_TRACK, uiMaterial,
                  ROLLER_ED_INVALID_MATERIAL_ID);

  tEdGltfExportOutput Output;
  std::string sError;
  assert(CEditorGltfExporter::Export(Builder.View(), DefaultOptions(), nullptr,
                                     0, Output, sError));
  cgltf_data *pData = ParseGltf(Output, false);

  const cgltf_primitive *pPrimitive = &FindMesh(pData, "Center")->primitives[0];
  const cgltf_accessor *pTexCoord =
      FindAttribute(pPrimitive, cgltf_attribute_type_texcoord);
  const size_t uiOffset = pTexCoord->buffer_view->offset;
  const float *pUV =
      reinterpret_cast<const float *>(Output.Binary.data() + uiOffset);
  // v0: (0, 0) through scale 0.125 and bias (0.625, 0) is (0.625, 0), and
  // stays there. OBJ would have written 1 - 0 = 1 for V.
  assert(std::fabs(pUV[0] - 0.625f) < 1e-6f);
  assert(std::fabs(pUV[1] - 0.0f) < 1e-6f);
  // v2: (1, 1) becomes (0.75, 0.125).
  assert(std::fabs(pUV[4] - 0.75f) < 1e-6f);
  assert(std::fabs(pUV[5] - 0.125f) < 1e-6f);
  cgltf_free(pData);
}

void test_a_two_sided_surface_becomes_a_double_sided_material()
{
  // ADR 0003: SURFACE_FLAG_CONCAVE makes the renderer bypass its facing test,
  // so the surface is visible from both sides. glTF can say that on the
  // material, so unlike OBJ it needs no duplicated reverse-wound geometry.
  CExtractionBuilder Builder;
  const uint32_t uiMaterial =
      Builder.AddTexturedMaterial(ROLLER_ED_TEXTURE_SET_TRACK, 0, 0.0f, 0.0f);
  Builder.AddQuad(ROLLER_ED_SURFACE_CLASS_ROOF,
                  ROLLER_ED_CONTENT_AUTHORED_TRACK, uiMaterial,
                  ROLLER_ED_INVALID_MATERIAL_ID,
                  ROLLER_ED_PRIMITIVE_FLAG_TWO_SIDED);
  Builder.AddQuad(ROLLER_ED_SURFACE_CLASS_CENTER,
                  ROLLER_ED_CONTENT_AUTHORED_TRACK, uiMaterial,
                  ROLLER_ED_INVALID_MATERIAL_ID);

  tEdGltfExportOutput Output;
  std::string sError;
  assert(CEditorGltfExporter::Export(Builder.View(), DefaultOptions(), nullptr,
                                     0, Output, sError));
  cgltf_data *pData = ParseGltf(Output, false);

  // No "(Back)" mesh: the two-sided surface is one primitive.
  assert(FindMesh(pData, "Roof (Back)") == nullptr);
  const cgltf_mesh *pRoof = FindMesh(pData, "Roof");
  assert(pRoof && pRoof->primitives_count == 1);
  assert(pRoof->primitives[0].indices->count == 6);
  assert(pRoof->primitives[0].material->double_sided);

  // The same canonical material used single-sided elsewhere must not inherit
  // the double-sidedness, so it becomes two glTF materials.
  const cgltf_mesh *pCenter = FindMesh(pData, "Center");
  assert(!pCenter->primitives[0].material->double_sided);
  assert(pRoof->primitives[0].material != pCenter->primitives[0].material);
  assert(pData->materials_count == 2);
  cgltf_free(pData);
}

void test_a_distinct_back_tile_still_gets_real_geometry()
{
  // One material cannot address two atlas tiles, so a surface whose
  // texture_back[] substitution names a different tile needs a reverse-wound
  // copy in every format.
  CExtractionBuilder Builder;
  const uint32_t uiFront =
      Builder.AddTexturedMaterial(ROLLER_ED_TEXTURE_SET_TRACK, 0, 0.0f, 0.0f);
  const uint32_t uiBack = Builder.AddTexturedMaterial(
      ROLLER_ED_TEXTURE_SET_TRACK, 0, 0.5f, 0.25f);
  Builder.AddQuad(ROLLER_ED_SURFACE_CLASS_CENTER,
                  ROLLER_ED_CONTENT_AUTHORED_TRACK, uiFront, uiBack);

  tEdGltfExportOutput Output;
  std::string sError;
  assert(CEditorGltfExporter::Export(Builder.View(), DefaultOptions(), nullptr,
                                     0, Output, sError));
  cgltf_data *pData = ParseGltf(Output, false);

  const cgltf_mesh *pBack = FindMesh(pData, "Center (Back)");
  assert(pBack != nullptr);
  assert(pBack->primitives_count == 1);
  assert(pBack->primitives[0].indices->count == 6);
  // Both sides are single-sided: each carries its own tile.
  assert(!pBack->primitives[0].material->double_sided);
  assert(!FindMesh(pData, "Center")->primitives[0].material->double_sided);

  // The reverse side's UVs come from the back material's bias, not the front's.
  const cgltf_accessor *pTexCoord =
      FindAttribute(&pBack->primitives[0], cgltf_attribute_type_texcoord);
  const float *pUV = reinterpret_cast<const float *>(
      Output.Binary.data() + pTexCoord->buffer_view->offset);
  assert(std::fabs(pUV[0] - 0.5f) < 1e-6f);
  assert(std::fabs(pUV[1] - 0.25f) < 1e-6f);

  // And its normal points the other way.
  const cgltf_accessor *pNormal =
      FindAttribute(&pBack->primitives[0], cgltf_attribute_type_normal);
  const float *pN = reinterpret_cast<const float *>(
      Output.Binary.data() + pNormal->buffer_view->offset);
  assert(std::fabs(pN[1] + 1.0f) < 1e-6f);
  cgltf_free(pData);
}

void test_alpha_modes_follow_the_material_flags()
{
  // The atlas alpha is binary - palette index 0 decodes to 0, everything else
  // to 255 - so a transparent textured surface is a cut-out, which is MASK.
  // A surface the game draws opaquely stays OPAQUE so the viewer ignores the
  // alpha channel entirely.
  CExtractionBuilder Builder;
  const uint32_t uiOpaque =
      Builder.AddTexturedMaterial(ROLLER_ED_TEXTURE_SET_TRACK, 0, 0.0f, 0.0f);
  const uint32_t uiCutout = Builder.AddTexturedMaterial(
      ROLLER_ED_TEXTURE_SET_TRACK, ROLLER_ED_MATERIAL_FLAG_ALPHA_BLEND, 0.25f,
      0.0f);
  const uint32_t uiDarken = Builder.AddDarkenMaterial(2);
  Builder.AddQuad(ROLLER_ED_SURFACE_CLASS_CENTER,
                  ROLLER_ED_CONTENT_AUTHORED_TRACK, uiOpaque,
                  ROLLER_ED_INVALID_MATERIAL_ID);
  Builder.AddQuad(ROLLER_ED_SURFACE_CLASS_LEFT_WALL,
                  ROLLER_ED_CONTENT_AUTHORED_TRACK, uiCutout,
                  ROLLER_ED_INVALID_MATERIAL_ID);
  Builder.AddQuad(ROLLER_ED_SURFACE_CLASS_RIGHT_WALL,
                  ROLLER_ED_CONTENT_AUTHORED_TRACK, uiDarken,
                  ROLLER_ED_INVALID_MATERIAL_ID);

  tEdGltfExportOutput Output;
  std::string sError;
  assert(CEditorGltfExporter::Export(Builder.View(), DefaultOptions(), nullptr,
                                     0, Output, sError));
  cgltf_data *pData = ParseGltf(Output, false);

  const cgltf_material *pOpaque =
      FindMesh(pData, "Center")->primitives[0].material;
  const cgltf_material *pCutout =
      FindMesh(pData, "Left Wall")->primitives[0].material;
  const cgltf_material *pDark =
      FindMesh(pData, "Right Wall")->primitives[0].material;
  assert(pOpaque->alpha_mode == cgltf_alpha_mode_opaque);
  assert(pCutout->alpha_mode == cgltf_alpha_mode_mask);
  assert(std::fabs(pCutout->alpha_cutoff - 0.5f) < 1e-6f);

  // Spec open item 2b, same approximation E4-S1 chose: black at the opacity
  // that leaves as much of the background as the renderer does. Never a
  // texture.
  assert(pDark->alpha_mode == cgltf_alpha_mode_blend);
  assert(pDark->pbr_metallic_roughness.base_color_texture.texture == nullptr);
  const float fExpected = CEditorExportConventions::ScreenDarkenAlpha(2);
  assert(std::fabs(pDark->pbr_metallic_roughness.base_color_factor[3]
                   - fExpected) < 1e-6f);
  assert(pDark->pbr_metallic_roughness.base_color_factor[0] == 0.0f);
  cgltf_free(pData);
}

void test_flat_palette_colours_are_converted_to_linear()
{
  // baseColorFactor is linear while the palette is sRGB; handing the sRGB
  // value over unconverted reads far too bright.
  CExtractionBuilder Builder;
  const uint32_t uiFlat = Builder.AddFlatMaterial(7);
  Builder.AddQuad(ROLLER_ED_SURFACE_CLASS_OUTER_WALL_FLOOR,
                  ROLLER_ED_CONTENT_AUTHORED_TRACK, uiFlat,
                  ROLLER_ED_INVALID_MATERIAL_ID);

  std::vector<tEdExportPaletteEntry> Palette(256);
  Palette[7].byRed = 255;
  Palette[7].byGreen = 128;
  Palette[7].byBlue = 0;

  tEdGltfExportOutput Output;
  std::string sError;
  assert(CEditorGltfExporter::Export(Builder.View(), DefaultOptions(),
                                     Palette.data(), 256, Output, sError));
  cgltf_data *pData = ParseGltf(Output, false);

  const cgltf_material *pMaterial =
      FindMesh(pData, "Outer Wall Floor")->primitives[0].material;
  const float *pFactor = pMaterial->pbr_metallic_roughness.base_color_factor;
  assert(std::fabs(pFactor[0] - 1.0f) < 1e-6f);
  assert(std::fabs(pFactor[1]
                   - CEditorGltfExporter::SrgbToLinear(128.0f / 255.0f))
         < 1e-6f);
  assert(pFactor[1] < 0.25f); // sRGB 0.502 is linear ~0.216, not ~0.502.
  assert(std::fabs(pFactor[2]) < 1e-6f);
  // A flat colour is never given a texture.
  assert(pMaterial->pbr_metallic_roughness.base_color_texture.texture
         == nullptr);
  assert(pData->textures_count == 0);
  cgltf_free(pData);
}

void test_only_referenced_atlases_become_textures()
{
  CExtractionBuilder Builder;
  const uint32_t uiTrack =
      Builder.AddTexturedMaterial(ROLLER_ED_TEXTURE_SET_TRACK, 0, 0.0f, 0.0f);
  Builder.AddQuad(ROLLER_ED_SURFACE_CLASS_CENTER,
                  ROLLER_ED_CONTENT_AUTHORED_TRACK, uiTrack,
                  ROLLER_ED_INVALID_MATERIAL_ID);

  tEdGltfExportOutput Output;
  std::string sError;
  assert(CEditorGltfExporter::Export(Builder.View(), DefaultOptions(), nullptr,
                                     0, Output, sError));
  cgltf_data *pData = ParseGltf(Output, false);

  // The building/sign atlas was offered but nothing uses it.
  assert(pData->textures_count == 1);
  assert(pData->images_count == 1);
  assert(pData->images[0].uri != nullptr);
  assert(std::strcmp(pData->images[0].uri, "TRACK3.png") == 0);
  // Nearest and clamped: the atlas packs tiles edge to edge, so filtering
  // would bleed neighbours into every seam.
  assert(pData->samplers_count == 1);
  assert(pData->samplers[0].mag_filter == cgltf_filter_type_nearest);
  assert(pData->samplers[0].wrap_s == cgltf_wrap_mode_clamp_to_edge);
  // A .gltf points at its buffer rather than carrying it.
  assert(pData->buffers_count == 1);
  assert(pData->buffers[0].uri != nullptr);
  assert(std::strcmp(pData->buffers[0].uri, "TRACK3.bin") == 0);
  assert(pData->buffers[0].size == Output.Binary.size());
  cgltf_free(pData);
}

void test_atlas_tiles_collapse_into_one_material()
{
  // The canonical table interns one material per atlas *tile*, but the
  // exported UVs are already atlas space, so every tile of a bank resolves
  // through the same image and settings. Emitting one glTF material each would
  // hand an importer 92 duplicates to rename to TRACK3.001 and up; retail
  // TRACK3 does exactly that without this.
  CExtractionBuilder Builder;
  for (int i = 0; i < 8; ++i) {
    const uint32_t uiTile = Builder.AddTexturedMaterial(
        ROLLER_ED_TEXTURE_SET_TRACK, 0, 0.125f * i, 0.0f);
    Builder.AddQuad(ROLLER_ED_SURFACE_CLASS_CENTER,
                    ROLLER_ED_CONTENT_AUTHORED_TRACK, uiTile,
                    ROLLER_ED_INVALID_MATERIAL_ID);
  }
  // A cut-out tile from the same bank is a different glTF material, because
  // its alpha mode differs, and it says so in its name.
  const uint32_t uiCutout = Builder.AddTexturedMaterial(
      ROLLER_ED_TEXTURE_SET_TRACK, ROLLER_ED_MATERIAL_FLAG_ALPHA_BLEND, 0.0f,
      0.0f);
  Builder.AddQuad(ROLLER_ED_SURFACE_CLASS_CENTER,
                  ROLLER_ED_CONTENT_AUTHORED_TRACK, uiCutout,
                  ROLLER_ED_INVALID_MATERIAL_ID);

  tEdGltfExportOutput Output;
  std::string sError;
  assert(CEditorGltfExporter::Export(Builder.View(), DefaultOptions(), nullptr,
                                     0, Output, sError));
  cgltf_data *pData = ParseGltf(Output, false);

  assert(pData->materials_count == 2);
  const cgltf_mesh *pCenter = FindMesh(pData, "Center");
  assert(pCenter->primitives_count == 2);
  // Eight tiles' worth of quads share one primitive and one material.
  cgltf_size uiOpaqueIndices = 0;
  cgltf_size uiCutoutIndices = 0;
  for (cgltf_size p = 0; p < pCenter->primitives_count; ++p) {
    if (pCenter->primitives[p].material->alpha_mode
        == cgltf_alpha_mode_opaque) {
      uiOpaqueIndices = pCenter->primitives[p].indices->count;
    } else {
      uiCutoutIndices = pCenter->primitives[p].indices->count;
    }
  }
  assert(uiOpaqueIndices == 8 * 6);
  assert(uiCutoutIndices == 6);

  // No two materials share a name, which is what forces an importer to rename.
  for (cgltf_size a = 0; a < pData->materials_count; ++a) {
    for (cgltf_size b = a + 1; b < pData->materials_count; ++b)
      assert(std::strcmp(pData->materials[a].name, pData->materials[b].name)
             != 0);
  }
  cgltf_free(pData);
}

void test_runtime_scenery_never_reaches_the_export()
{
  CExtractionBuilder Builder;
  const uint32_t uiMaterial =
      Builder.AddTexturedMaterial(ROLLER_ED_TEXTURE_SET_TRACK, 0, 0.0f, 0.0f);
  Builder.AddQuad(ROLLER_ED_SURFACE_CLASS_CENTER,
                  ROLLER_ED_CONTENT_RUNTIME_SCENERY, uiMaterial,
                  ROLLER_ED_INVALID_MATERIAL_ID);
  Builder.AddQuad(ROLLER_ED_SURFACE_CLASS_CENTER,
                  ROLLER_ED_CONTENT_AUTHORED_TRACK, uiMaterial,
                  ROLLER_ED_INVALID_MATERIAL_ID);

  tEdGltfExportOutput Output;
  std::string sError;
  assert(CEditorGltfExporter::Export(Builder.View(), DefaultOptions(), nullptr,
                                     0, Output, sError));
  cgltf_data *pData = ParseGltf(Output, false);
  assert(pData->meshes_count == 1);
  assert(FindMesh(pData, "Center")->primitives[0].indices->count == 6);
  cgltf_free(pData);
}

// E4A-S6. Both canonical exporters gained signs and scenery from one core
// change, so glTF gets the same named meshes OBJ does -- and, unlike OBJ, the
// building/sign atlas it had been offering all along finally gets used.
void test_signs_and_scenery_reach_the_gltf_scene()
{
  CExtractionBuilder Builder;
  const uint32_t uiTrack =
      Builder.AddTexturedMaterial(ROLLER_ED_TEXTURE_SET_TRACK, 0, 0.0f, 0.0f);
  const uint32_t uiBld = Builder.AddTexturedMaterial(
      ROLLER_ED_TEXTURE_SET_BUILDING_SIGN, 0, 0.0f, 0.0f);
  Builder.AddQuad(ROLLER_ED_SURFACE_CLASS_CENTER,
                  ROLLER_ED_CONTENT_AUTHORED_TRACK, uiTrack,
                  ROLLER_ED_INVALID_MATERIAL_ID);
  Builder.AddQuad(ROLLER_ED_SURFACE_CLASS_SIGN,
                  ROLLER_ED_CONTENT_AUTHORED_SIGN, uiBld,
                  ROLLER_ED_INVALID_MATERIAL_ID);
  Builder.AddQuad(ROLLER_ED_SURFACE_CLASS_SIGN,
                  ROLLER_ED_CONTENT_AUTHORED_SIGN, uiBld,
                  ROLLER_ED_INVALID_MATERIAL_ID);
  Builder.AddQuad(ROLLER_ED_SURFACE_CLASS_BUILDING,
                  ROLLER_ED_CONTENT_AUTHORED_SCENERY, uiBld,
                  ROLLER_ED_INVALID_MATERIAL_ID);

  tEdGltfExportOutput Output;
  std::string sError;
  assert(CEditorGltfExporter::Export(Builder.View(), DefaultOptions(), nullptr,
                                     0, Output, sError));
  cgltf_data *pData = ParseGltf(Output, false);
  assert(FindMesh(pData, "Center") != nullptr);
  assert(FindMesh(pData, "Sign 0") != nullptr);
  assert(FindMesh(pData, "Sign 1") != nullptr);
  assert(FindMesh(pData, "Sign 2") == nullptr);
  assert(FindMesh(pData, "Scenery") != nullptr);
  assert(pData->meshes_count == 4);
  // The second atlas is referenced now, so E4-S4's _BLD.png is no longer a
  // file nothing points at.
  assert(pData->images_count == 2);
  cgltf_free(pData);

  // Unticking Include signs reproduces the E4-S2 export exactly: track only,
  // and one image again.
  tEdGltfExportOptions TrackOnly = DefaultOptions();
  TrackOnly.bExportScenery = false;
  tEdGltfExportOutput Reduced;
  assert(CEditorGltfExporter::Export(Builder.View(), TrackOnly, nullptr, 0,
                                     Reduced, sError));
  cgltf_data *pReduced = ParseGltf(Reduced, false);
  assert(pReduced->meshes_count == 1);
  assert(FindMesh(pReduced, "Center") != nullptr);
  assert(pReduced->images_count == 1);
  cgltf_free(pReduced);
}

void test_combined_sections_produce_one_node()
{
  CExtractionBuilder Builder;
  const uint32_t uiMaterial =
      Builder.AddTexturedMaterial(ROLLER_ED_TEXTURE_SET_TRACK, 0, 0.0f, 0.0f);
  Builder.AddQuad(ROLLER_ED_SURFACE_CLASS_CENTER,
                  ROLLER_ED_CONTENT_AUTHORED_TRACK, uiMaterial,
                  ROLLER_ED_INVALID_MATERIAL_ID);
  Builder.AddQuad(ROLLER_ED_SURFACE_CLASS_ROOF,
                  ROLLER_ED_CONTENT_AUTHORED_TRACK, uiMaterial,
                  ROLLER_ED_INVALID_MATERIAL_ID);

  tEdGltfExportOptions Options = DefaultOptions();
  Options.bSeparateSections = false;
  tEdGltfExportOutput Output;
  std::string sError;
  assert(CEditorGltfExporter::Export(Builder.View(), Options, nullptr, 0,
                                     Output, sError));
  cgltf_data *pData = ParseGltf(Output, false);
  assert(pData->meshes_count == 1);
  const cgltf_mesh *pTrack = FindMesh(pData, "Track");
  assert(pTrack != nullptr);
  // One material across both surfaces, so one primitive carrying both quads.
  assert(pTrack->primitives_count == 1);
  assert(pTrack->primitives[0].indices->count == 12);
  cgltf_free(pData);
}

void test_the_glb_container_is_well_formed()
{
  CExtractionBuilder Builder;
  const uint32_t uiMaterial =
      Builder.AddTexturedMaterial(ROLLER_ED_TEXTURE_SET_TRACK, 0, 0.0f, 0.0f);
  Builder.AddQuad(ROLLER_ED_SURFACE_CLASS_CENTER,
                  ROLLER_ED_CONTENT_AUTHORED_TRACK, uiMaterial,
                  ROLLER_ED_INVALID_MATERIAL_ID);

  tEdGltfExportOptions Options = DefaultOptions();
  Options.bBinary = true;
  tEdGltfExportOutput Output;
  std::string sError;
  assert(CEditorGltfExporter::Export(Builder.View(), Options, nullptr, 0,
                                     Output, sError));

  // A .glb's buffer is its binary chunk, so it carries no uri, and its images
  // live in buffer views rather than beside the file.
  cgltf_options ParseOptions;
  std::memset(&ParseOptions, 0, sizeof(ParseOptions));
  cgltf_data *pData = nullptr;
  assert(cgltf_parse(&ParseOptions, Output.sJson.data(), Output.sJson.size(),
                     &pData)
         == cgltf_result_success);
  assert(pData->buffers_count == 1);
  assert(pData->buffers[0].uri == nullptr);
  assert(pData->images_count == 1);
  assert(pData->images[0].uri == nullptr);
  assert(pData->images[0].buffer_view != nullptr);
  assert(std::strcmp(pData->images[0].mime_type, "image/png") == 0);
  cgltf_free(pData);

  // The packed container: header, JSON chunk, BIN chunk, everything padded to
  // four bytes.
  std::string sWriteError;
  assert(CEditorGltfExporter::ExportToFiles(Builder.View(), Options, nullptr, 0,
                                            "e4s2-sample.glb", std::string(),
                                            sWriteError));
  std::FILE *pFile = std::fopen("e4s2-sample.glb", "rb");
  assert(pFile != nullptr);
  std::fseek(pFile, 0, SEEK_END);
  const long lSize = std::ftell(pFile);
  std::fseek(pFile, 0, SEEK_SET);
  std::vector<uint8_t> Glb(static_cast<size_t>(lSize));
  assert(std::fread(Glb.data(), 1, Glb.size(), pFile) == Glb.size());
  std::fclose(pFile);

  assert(ReadUint32(Glb, 0) == 0x46546c67u); // "glTF"
  assert(ReadUint32(Glb, 4) == 2u);
  assert(ReadUint32(Glb, 8) == Glb.size());
  const uint32_t uiJsonChunk = ReadUint32(Glb, 12);
  assert(ReadUint32(Glb, 16) == 0x4e4f534au); // "JSON"
  assert((uiJsonChunk % 4) == 0);
  const size_t uiBinHeader = 20 + uiJsonChunk;
  const uint32_t uiBinChunk = ReadUint32(Glb, uiBinHeader);
  assert(ReadUint32(Glb, uiBinHeader + 4) == 0x004e4942u); // "BIN\0"
  assert((uiBinChunk % 4) == 0);
  assert(uiBinHeader + 8 + uiBinChunk == Glb.size());
  assert(uiBinChunk >= Output.Binary.size());

  // The whole file parses as glTF, which is what an importer will do.
  cgltf_data *pFromFile = nullptr;
  assert(cgltf_parse_file(&ParseOptions, "e4s2-sample.glb", &pFromFile)
         == cgltf_result_success);
  assert(cgltf_validate(pFromFile) == cgltf_result_success);
  assert(pFromFile->file_type == cgltf_file_type_glb);
  cgltf_free(pFromFile);
}

void test_a_binary_export_without_image_bytes_is_refused()
{
  CExtractionBuilder Builder;
  const uint32_t uiMaterial =
      Builder.AddTexturedMaterial(ROLLER_ED_TEXTURE_SET_TRACK, 0, 0.0f, 0.0f);
  Builder.AddQuad(ROLLER_ED_SURFACE_CLASS_CENTER,
                  ROLLER_ED_CONTENT_AUTHORED_TRACK, uiMaterial,
                  ROLLER_ED_INVALID_MATERIAL_ID);

  tEdGltfExportOptions Options = DefaultOptions();
  Options.bBinary = true;
  for (size_t i = 0; i < Options.Textures.size(); ++i)
    Options.Textures[i].PngBytes.clear();

  tEdGltfExportOutput Output;
  std::string sError;
  // A .glb that referenced a file it did not carry would not be
  // self-contained, which is the only reason to pick .glb.
  assert(!CEditorGltfExporter::Export(Builder.View(), Options, nullptr, 0,
                                      Output, sError));
  assert(!sError.empty());

  // An empty or inconsistent extraction is refused before anything is written.
  CExtractionBuilder Empty;
  assert(!CEditorGltfExporter::Export(Empty.View(), DefaultOptions(), nullptr,
                                      0, Output, sError));
  assert(!sError.empty());

  CExtractionBuilder Broken;
  Broken.AddTexturedMaterial(ROLLER_ED_TEXTURE_SET_TRACK, 0, 0.0f, 0.0f);
  Broken.AddQuad(ROLLER_ED_SURFACE_CLASS_CENTER,
                 ROLLER_ED_CONTENT_AUTHORED_TRACK, 0,
                 ROLLER_ED_INVALID_MATERIAL_ID);
  Broken.m_Primitives[0].uiFrontMaterialId = 9;
  assert(!CEditorGltfExporter::Export(Broken.View(), DefaultOptions(), nullptr,
                                      0, Output, sError));
  assert(!sError.empty());
}

// Writes the documents tests/check_gltf_in_blender.py imports. Kept
// deliberately small and fully synthetic so the Blender check needs no retail
// track data.
void write_blender_samples()
{
  CExtractionBuilder Builder;
  const uint32_t uiTrack =
      Builder.AddTexturedMaterial(ROLLER_ED_TEXTURE_SET_TRACK, 0, 0.0f, 0.0f);
  const uint32_t uiFlat = Builder.AddFlatMaterial(7);
  Builder.AddQuad(ROLLER_ED_SURFACE_CLASS_CENTER,
                  ROLLER_ED_CONTENT_AUTHORED_TRACK, uiTrack,
                  ROLLER_ED_INVALID_MATERIAL_ID);
  Builder.AddQuad(ROLLER_ED_SURFACE_CLASS_ROOF,
                  ROLLER_ED_CONTENT_AUTHORED_TRACK, uiTrack,
                  ROLLER_ED_INVALID_MATERIAL_ID,
                  ROLLER_ED_PRIMITIVE_FLAG_TWO_SIDED);
  Builder.AddQuad(ROLLER_ED_SURFACE_CLASS_LEFT_WALL,
                  ROLLER_ED_CONTENT_AUTHORED_TRACK, uiFlat,
                  ROLLER_ED_INVALID_MATERIAL_ID);

  std::vector<tEdExportPaletteEntry> Palette(256);
  Palette[7].byRed = 200;
  Palette[7].byGreen = 40;
  Palette[7].byBlue = 40;

  // The .gltf sample has no PNGs beside it on purpose: an importer must still
  // load the geometry when a referenced image is missing, and the geometry is
  // what this check is about.
  tEdGltfExportOptions Json = DefaultOptions();
  std::string sError;
  assert(CEditorGltfExporter::ExportToFiles(Builder.View(), Json,
                                            Palette.data(), 256,
                                            "e4s2-sample.gltf", "TRACK3.bin",
                                            sError));

  tEdGltfExportOptions Binary = DefaultOptions();
  Binary.bBinary = true;
  assert(CEditorGltfExporter::ExportToFiles(Builder.View(), Binary,
                                            Palette.data(), 256,
                                            "e4s2-sample-full.glb",
                                            std::string(), sError));
}
}

int main()
{
  test_the_document_parses_and_validates();
  test_attributes_and_indices_use_the_declared_types();
  test_the_axis_conversion_reaches_the_buffer();
  test_uvs_are_not_flipped_the_way_obj_flips_them();
  test_a_two_sided_surface_becomes_a_double_sided_material();
  test_a_distinct_back_tile_still_gets_real_geometry();
  test_alpha_modes_follow_the_material_flags();
  test_flat_palette_colours_are_converted_to_linear();
  test_only_referenced_atlases_become_textures();
  test_atlas_tiles_collapse_into_one_material();
  test_runtime_scenery_never_reaches_the_export();
  test_signs_and_scenery_reach_the_gltf_scene();
  test_combined_sections_produce_one_node();
  test_the_glb_container_is_well_formed();
  test_a_binary_export_without_image_bytes_is_refused();
  write_blender_samples();
  std::puts("editor glTF exporter tests passed");
  return 0;
}
