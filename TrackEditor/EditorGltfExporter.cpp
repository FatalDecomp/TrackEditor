#include "EditorGltfExporter.h"

#include "cgltf.h"
#include "cgltf_write.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <deque>
#include <fstream>
#include <map>

namespace
{
// glTF requires accessor and image buffer views to start on a four-byte
// boundary, and the GLB container pads both chunks to four as well.
const size_t g_uiGltfAlignment = 4;

size_t AlignUp(size_t uiValue)
{
  const size_t uiRemainder = uiValue % g_uiGltfAlignment;
  return uiRemainder == 0 ? uiValue : uiValue + (g_uiGltfAlignment - uiRemainder);
}

// One glTF primitive: a run of faces from a single mesh sharing one material
// and one double-sidedness. glTF primitives carry exactly one material, so a
// mesh gets as many primitives as the surface class has distinct materials.
struct tGltfBatch
{
  uint32_t uiCanonicalMaterial = 0;
  bool bDoubleSided = false;
  std::vector<float> Positions;
  std::vector<float> Normals;
  std::vector<float> TexCoords;
  std::vector<uint32_t> Indices;
  float afMin[3] = { 0.0f, 0.0f, 0.0f };
  float afMax[3] = { 0.0f, 0.0f, 0.0f };
  // Assigned once the flat material and accessor arrays are laid out.
  size_t uiMaterialIndex = 0;
  size_t uiFirstAccessor = 0;
};

struct tGltfMeshBuild
{
  std::string sName;
  std::vector<tGltfBatch> Batches;
};

// Keeps every string cgltf points at alive and at a stable address for the
// lifetime of the write. A vector would invalidate c_str() as it grew.
class CNamePool
{
public:
  char *Add(const std::string &sValue)
  {
    m_Pool.push_back(sValue);
    return &m_Pool.back()[0];
  }

private:
  std::deque<std::string> m_Pool;
};

// The atlas alpha is binary - palette index 0 decodes to alpha 0 and every
// other index to 255 - so a transparent textured surface is a cut-out rather
// than a blend. A surface the game draws opaquely stays OPAQUE, which makes
// the viewer ignore the alpha channel and keep index 0's palette colour, as
// the renderer does.
cgltf_alpha_mode AlphaModeFor(const tEdMaterial &Material)
{
  if (Material.uiKind == ROLLER_ED_MATERIAL_SCREEN_DARKEN)
    return cgltf_alpha_mode_blend;
  if (CEditorExportConventions::IsTexturedKind(Material.uiKind)
      && (Material.uiFlags & ROLLER_ED_MATERIAL_FLAG_ALPHA_BLEND) != 0) {
    return cgltf_alpha_mode_mask;
  }
  return cgltf_alpha_mode_opaque;
}

// Building/sign polygons use a dedicated renderer path. Advert textures are
// cut-outs there even though building_polygon_surface_info deliberately strips
// PARTIAL_TRANS for depth routing, so their canonical material does not carry
// the ordinary alpha flag. Preserve that implicit sign-pipeline behavior in
// glTF without making opaque building surfaces cut-outs too.
tEdMaterial ExportMaterialForEntry(const tEdExportGeometry &Geometry,
                                   const tEdExportEntry &Entry)
{
  tEdMaterial Material = Geometry.pMaterials[Entry.uiMaterial];
  const tEdPrimitive &Primitive = Geometry.pPrimitives[Entry.uiPrimitive];
  if (Primitive.unContentClass == ROLLER_ED_CONTENT_AUTHORED_SIGN
      && CEditorExportConventions::IsTexturedKind(Material.uiKind)) {
    Material.uiFlags |= ROLLER_ED_MATERIAL_FLAG_ALPHA_BLEND;
  }
  return Material;
}

bool IsDoubleSidedEntry(const tEdExportGeometry &Geometry,
                        const tEdGltfExportOptions &Options,
                        const tEdExportEntry &Entry)
{
  if (Options.bCompleteReverseGeometry
      || !Options.bDoubleSidedMaterials || Entry.bBack)
    return false;
  const tEdPrimitive &Primitive = Geometry.pPrimitives[Entry.uiPrimitive];
  // A surface with a genuinely different reverse tile got real reverse
  // geometry instead, and both sides of it are single-sided.
  if (CEditorExportConventions::HasDistinctReverseMaterial(Primitive))
    return false;
  return CEditorExportConventions::HasReverseSide(Primitive);
}

void AppendEntryToBatch(const tEdExportGeometry &Geometry,
                        const tEdExportEntry &Entry, tGltfBatch &Batch)
{
  const tEdMaterial &Material = Geometry.pMaterials[Entry.uiMaterial];
  const uint32_t uiBase = static_cast<uint32_t>(Batch.Positions.size() / 3);
  const float fNormalSign = Entry.bBack ? -1.0f : 1.0f;

  for (size_t v = 0; v < Entry.Vertices.size(); ++v) {
    const tEdVertex &Vertex = Geometry.pVertices[Entry.Vertices[v]];

    float afPosition[3];
    CEditorExportConventions::ConvertPosition(Vertex.fPosition, afPosition);
    for (int c = 0; c < 3; ++c) {
      if (Batch.Positions.empty() && v == 0) {
        Batch.afMin[c] = afPosition[c];
        Batch.afMax[c] = afPosition[c];
      } else {
        Batch.afMin[c] = std::min(Batch.afMin[c], afPosition[c]);
        Batch.afMax[c] = std::max(Batch.afMax[c], afPosition[c]);
      }
    }
    Batch.Positions.push_back(afPosition[0]);
    Batch.Positions.push_back(afPosition[1]);
    Batch.Positions.push_back(afPosition[2]);

    float afNormal[3];
    CEditorExportConventions::ConvertDirection(Vertex.fNormal, afNormal);
    Batch.Normals.push_back(fNormalSign * afNormal[0]);
    Batch.Normals.push_back(fNormalSign * afNormal[1]);
    Batch.Normals.push_back(fNormalSign * afNormal[2]);

    // Material-local UVs resolve through the selected material's atlas
    // transform (AD-7b); the reverse side resolves through the back material.
    // glTF's UV origin is top-left, the same as ROLLER's, so unlike OBJ there
    // is no V flip here.
    Batch.TexCoords.push_back(Vertex.fUV[0] * Material.fAtlasScale[0]
                              + Material.fAtlasBias[0]);
    Batch.TexCoords.push_back(Vertex.fUV[1] * Material.fAtlasScale[1]
                              + Material.fAtlasBias[1]);
  }

  for (size_t t = 0; t + 2 < Entry.Triangles.size(); t += 3) {
    // The axis conversion preserves handedness, so a front face keeps the
    // emitted vertex order and glTF's counter-clockwise front lands on the
    // side uiFrontMaterialId describes. The reverse side is the same triangle
    // wound the other way.
    const size_t aiCorner[3] = {
      Entry.bBack ? t + 2 : t,
      t + 1,
      Entry.bBack ? t : t + 2
    };
    for (int c = 0; c < 3; ++c)
      Batch.Indices.push_back(uiBase + Entry.Triangles[aiCorner[c]]);
  }
}

size_t AppendBuffer(std::vector<uint8_t> &Blob, const void *pData,
                    size_t uiBytes, size_t &uiOffsetOut)
{
  Blob.resize(AlignUp(Blob.size()));
  uiOffsetOut = Blob.size();
  const uint8_t *pBytes = static_cast<const uint8_t *>(pData);
  Blob.insert(Blob.end(), pBytes, pBytes + uiBytes);
  return uiBytes;
}

void WriteUint32(std::vector<uint8_t> &Out, uint32_t uiValue)
{
  Out.push_back(static_cast<uint8_t>(uiValue & 0xffu));
  Out.push_back(static_cast<uint8_t>((uiValue >> 8) & 0xffu));
  Out.push_back(static_cast<uint8_t>((uiValue >> 16) & 0xffu));
  Out.push_back(static_cast<uint8_t>((uiValue >> 24) & 0xffu));
}

// The GLB container: a 12-byte header, then a JSON chunk padded with spaces,
// then an optional BIN chunk padded with zeros. Both chunk payloads are
// four-byte aligned, which is what makes the file mappable.
void PackGlb(const std::string &sJson, const std::vector<uint8_t> &Binary,
             std::vector<uint8_t> &Out)
{
  const size_t uiJsonPad = AlignUp(sJson.size()) - sJson.size();
  const size_t uiBinPad = AlignUp(Binary.size()) - Binary.size();
  const size_t uiJsonChunk = sJson.size() + uiJsonPad;
  const size_t uiBinChunk = Binary.empty() ? 0 : Binary.size() + uiBinPad;
  size_t uiTotal = 12 + 8 + uiJsonChunk;
  if (uiBinChunk != 0)
    uiTotal += 8 + uiBinChunk;

  Out.clear();
  Out.reserve(uiTotal);
  WriteUint32(Out, 0x46546c67u); // "glTF"
  WriteUint32(Out, 2u);
  WriteUint32(Out, static_cast<uint32_t>(uiTotal));

  WriteUint32(Out, static_cast<uint32_t>(uiJsonChunk));
  WriteUint32(Out, 0x4e4f534au); // "JSON"
  Out.insert(Out.end(), sJson.begin(), sJson.end());
  Out.insert(Out.end(), uiJsonPad, static_cast<uint8_t>(' '));

  if (uiBinChunk == 0)
    return;
  WriteUint32(Out, static_cast<uint32_t>(uiBinChunk));
  WriteUint32(Out, 0x004e4942u); // "BIN\0"
  Out.insert(Out.end(), Binary.begin(), Binary.end());
  Out.insert(Out.end(), uiBinPad, static_cast<uint8_t>(0));
}

bool WriteWholeFile(const std::string &sPath, const void *pData,
                    size_t uiBytes, std::string &sError)
{
  std::ofstream File(sPath.c_str(), std::ios::binary);
  if (!File.is_open()) {
    sError = "could not open " + sPath;
    return false;
  }
  if (uiBytes != 0)
    File.write(static_cast<const char *>(pData),
               static_cast<std::streamsize>(uiBytes));
  File.close();
  if (!File.good()) {
    sError = "could not write " + sPath;
    return false;
  }
  return true;
}
}

std::string CEditorGltfExporter::MaterialName(const std::string &sBaseName,
                                              const tEdMaterial &Material,
                                              bool bDoubleSided)
{
  std::string sName;
  if (CEditorExportConventions::IsTexturedKind(Material.uiKind)) {
    // One material per texture set, not per tile: the UVs are already atlas
    // space, so every tile of a bank shares that bank's image.
    sName = Material.uiTextureSet == ROLLER_ED_TEXTURE_SET_BUILDING_SIGN
        ? sBaseName + "_BLD"
        : sBaseName;
    if (AlphaModeFor(Material) == cgltf_alpha_mode_mask)
      sName += "_cutout";
  } else if (Material.uiKind == ROLLER_ED_MATERIAL_SCREEN_DARKEN) {
    sName = sBaseName + "_darken_" + std::to_string(Material.uiDarkenLevel);
  } else {
    sName = sBaseName + "_color_" + std::to_string(Material.uiPaletteColour);
  }
  // The name is a pure function of everything that reaches the glTF material -
  // texture set, alpha mode, palette colour, darkening level, and
  // double-sidedness - which is exactly why it can be used as the identity
  // that collapses the canonical materials down (see Export).
  return bDoubleSided ? sName + "_two_sided" : sName;
}

float CEditorGltfExporter::SrgbToLinear(float fSrgb)
{
  if (fSrgb <= 0.04045f)
    return fSrgb / 12.92f;
  return std::pow((fSrgb + 0.055f) / 1.055f, 2.4f);
}

bool CEditorGltfExporter::Export(const tEdExportGeometry &Geometry,
                                 const tEdGltfExportOptions &Options,
                                 const tEdExportPaletteEntry *pPalette,
                                 uint32_t uiPaletteCount,
                                 tEdGltfExportOutput &Output,
                                 std::string &sError)
{
  sError.clear();
  Output.sJson.clear();
  Output.Binary.clear();
  if (Options.sBaseName.empty()) {
    sError = "the export needs a base name";
    return false;
  }
  if (!Options.bBinary && Options.sBufferUri.empty()) {
    sError = "a non-binary glTF export needs a buffer uri";
    return false;
  }
  if (!CEditorExportConventions::ValidateGeometry(Geometry, sError))
    return false;

  tEdExportGrouping Grouping;
  Grouping.sSingleObjectName = Options.sSingleObjectName;
  Grouping.bExportScenery = Options.bExportScenery;
  Grouping.bSeparateSections = Options.bSeparateSections;
  Grouping.bSeparateBackFaces = Options.bSeparateBackFaces;
  // Compact mode may use glTF's double-sided material support. Complete model
  // export overrides it and creates explicit reverse geometry for every quad.
  Grouping.bReverseSideAsGeometry = !Options.bDoubleSidedMaterials;
  Grouping.bGenerateAllReverseSides = Options.bCompleteReverseGeometry;

  std::vector<tEdExportObject> Objects;
  if (!CEditorExportConventions::BuildObjects(Geometry, Grouping, Objects,
                                              sError)) {
    return false;
  }

  // ---- Batch the faces by mesh and material -------------------------------
  // The canonical material table interns one entry per atlas *tile*, but the
  // exported UVs are already atlas space, so every tile of a bank resolves
  // through the same image and the same settings. Keying the glTF materials on
  // the resolved name collapses them: retail TRACK3's 92 canonical materials
  // become a handful, instead of 92 duplicates an importer has to rename to
  // TRACK3.001 and up. The name is a pure function of everything that reaches
  // the glTF material, so it is a sound identity.
  std::vector<tGltfMeshBuild> Meshes;
  std::map<std::string, size_t> MaterialKeys;
  // Resolved name plus the material to build it from. Sign entries may carry
  // an export-only cut-out flag that is implicit in ROLLER's sign pipeline,
  // so store the resolved value rather than only its canonical table index.
  std::vector<std::pair<tEdMaterial, bool>> MaterialOrder;

  for (size_t o = 0; o < Objects.size(); ++o) {
    tGltfMeshBuild Mesh;
    Mesh.sName = Objects[o].sName;
    std::map<std::string, size_t> BatchIndex;

    for (size_t e = 0; e < Objects[o].Entries.size(); ++e) {
      const tEdExportEntry &Entry = Objects[o].Entries[e];
      const bool bDoubleSided =
          IsDoubleSidedEntry(Geometry, Options, Entry);
      const tEdMaterial ExportMaterial =
          ExportMaterialForEntry(Geometry, Entry);
      const std::string sKey = MaterialName(
          Options.sBaseName, ExportMaterial, bDoubleSided);

      if (MaterialKeys.find(sKey) == MaterialKeys.end()) {
        MaterialKeys[sKey] = MaterialOrder.size();
        MaterialOrder.push_back(
            std::pair<tEdMaterial, bool>(ExportMaterial, bDoubleSided));
      }

      const std::map<std::string, size_t>::const_iterator Found =
          BatchIndex.find(sKey);
      size_t uiBatch;
      if (Found == BatchIndex.end()) {
        uiBatch = Mesh.Batches.size();
        BatchIndex[sKey] = uiBatch;
        tGltfBatch Batch;
        Batch.uiCanonicalMaterial = Entry.uiMaterial;
        Batch.bDoubleSided = bDoubleSided;
        Batch.uiMaterialIndex = MaterialKeys[sKey];
        Mesh.Batches.push_back(Batch);
      } else {
        uiBatch = Found->second;
      }
      AppendEntryToBatch(Geometry, Entry, Mesh.Batches[uiBatch]);
    }

    if (!Mesh.Batches.empty())
      Meshes.push_back(std::move(Mesh));
  }

  if (Meshes.empty()) {
    sError = "the loaded track produced no authored surfaces to export";
    return false;
  }

  // ---- Decide which atlases the export actually references ----------------
  std::vector<size_t> TextureForSet; // parallel to Options.Textures
  std::map<uint32_t, size_t> TextureIndexBySet;
  for (size_t k = 0; k < MaterialOrder.size(); ++k) {
    const tEdMaterial &Material = MaterialOrder[k].first;
    if (!CEditorExportConventions::IsTexturedKind(Material.uiKind))
      continue;
    if (TextureIndexBySet.find(Material.uiTextureSet)
        != TextureIndexBySet.end()) {
      continue;
    }
    for (size_t t = 0; t < Options.Textures.size(); ++t) {
      const tEdGltfTextureSource &Source = Options.Textures[t];
      if (Source.uiTextureSet != Material.uiTextureSet)
        continue;
      if (Source.sUri.empty() && Source.PngBytes.empty())
        continue;
      TextureIndexBySet[Material.uiTextureSet] = TextureForSet.size();
      TextureForSet.push_back(t);
      break;
    }
  }

  // ---- Pack the binary payload -------------------------------------------
  // Layout: every batch's positions, normals, texcoords, and indices in mesh
  // order, then the embedded images. Each run starts four-byte aligned.
  size_t uiAccessorCount = 0;
  for (size_t m = 0; m < Meshes.size(); ++m)
    uiAccessorCount += Meshes[m].Batches.size() * 4;

  std::vector<size_t> ViewOffsets(uiAccessorCount, 0);
  std::vector<size_t> ViewSizes(uiAccessorCount, 0);
  size_t uiAccessor = 0;
  for (size_t m = 0; m < Meshes.size(); ++m) {
    for (size_t b = 0; b < Meshes[m].Batches.size(); ++b) {
      tGltfBatch &Batch = Meshes[m].Batches[b];
      Batch.uiFirstAccessor = uiAccessor;
      ViewSizes[uiAccessor] = AppendBuffer(
          Output.Binary, Batch.Positions.data(),
          Batch.Positions.size() * sizeof(float), ViewOffsets[uiAccessor]);
      ++uiAccessor;
      ViewSizes[uiAccessor] = AppendBuffer(
          Output.Binary, Batch.Normals.data(),
          Batch.Normals.size() * sizeof(float), ViewOffsets[uiAccessor]);
      ++uiAccessor;
      ViewSizes[uiAccessor] = AppendBuffer(
          Output.Binary, Batch.TexCoords.data(),
          Batch.TexCoords.size() * sizeof(float), ViewOffsets[uiAccessor]);
      ++uiAccessor;
      ViewSizes[uiAccessor] = AppendBuffer(
          Output.Binary, Batch.Indices.data(),
          Batch.Indices.size() * sizeof(uint32_t), ViewOffsets[uiAccessor]);
      ++uiAccessor;
    }
  }

  // Embedded images only exist in a .glb; a .gltf points at the PNG files the
  // texture export already wrote beside it.
  const bool bEmbedImages = Options.bBinary;
  std::vector<size_t> ImageViewOffsets(TextureForSet.size(), 0);
  std::vector<size_t> ImageViewSizes(TextureForSet.size(), 0);
  if (bEmbedImages) {
    for (size_t i = 0; i < TextureForSet.size(); ++i) {
      const tEdGltfTextureSource &Source = Options.Textures[TextureForSet[i]];
      if (Source.PngBytes.empty()) {
        sError = "a binary glTF export needs the image bytes for every "
                 "referenced texture set";
        return false;
      }
      ImageViewSizes[i] =
          AppendBuffer(Output.Binary, Source.PngBytes.data(),
                       Source.PngBytes.size(), ImageViewOffsets[i]);
    }
  }

  // ---- Assemble the cgltf graph -------------------------------------------
  // Every array is sized exactly before it is filled: cgltf_write derives the
  // JSON indices from pointer arithmetic against these bases, so a
  // reallocation would silently corrupt the document.
  CNamePool Names;
  cgltf_data Data;
  std::memset(&Data, 0, sizeof(Data));
  Data.file_type = Options.bBinary ? cgltf_file_type_glb : cgltf_file_type_gltf;
  Data.asset.version = Names.Add("2.0");
  Data.asset.generator = Names.Add("ROLLER Track Editor (E4-S2)");

  const size_t uiViewCount = uiAccessorCount
      + (bEmbedImages ? ImageViewOffsets.size() : 0);

  std::vector<cgltf_buffer> Buffers(1);
  std::memset(Buffers.data(), 0, Buffers.size() * sizeof(cgltf_buffer));
  Buffers[0].size = Output.Binary.size();
  // A GLB's binary chunk is the buffer, and carries no uri by definition.
  if (!Options.bBinary)
    Buffers[0].uri = Names.Add(Options.sBufferUri);

  std::vector<cgltf_buffer_view> Views(uiViewCount);
  std::memset(Views.data(), 0, Views.size() * sizeof(cgltf_buffer_view));
  for (size_t i = 0; i < uiAccessorCount; ++i) {
    Views[i].buffer = Buffers.data();
    Views[i].offset = ViewOffsets[i];
    Views[i].size = ViewSizes[i];
  }
  if (bEmbedImages) {
    for (size_t i = 0; i < ImageViewOffsets.size(); ++i) {
      cgltf_buffer_view &View = Views[uiAccessorCount + i];
      View.buffer = Buffers.data();
      View.offset = ImageViewOffsets[i];
      View.size = ImageViewSizes[i];
    }
  }

  std::vector<cgltf_accessor> Accessors(uiAccessorCount);
  std::memset(Accessors.data(), 0, Accessors.size() * sizeof(cgltf_accessor));
  for (size_t m = 0; m < Meshes.size(); ++m) {
    for (size_t b = 0; b < Meshes[m].Batches.size(); ++b) {
      const tGltfBatch &Batch = Meshes[m].Batches[b];
      const size_t uiFirst = Batch.uiFirstAccessor;
      const size_t uiVertices = Batch.Positions.size() / 3;

      cgltf_accessor &Position = Accessors[uiFirst + 0];
      Position.component_type = cgltf_component_type_r_32f;
      Position.type = cgltf_type_vec3;
      Position.count = uiVertices;
      Position.buffer_view = &Views[uiFirst + 0];
      // POSITION is the one accessor glTF requires bounds on.
      Position.has_min = 1;
      Position.has_max = 1;
      for (int c = 0; c < 3; ++c) {
        Position.min[c] = Batch.afMin[c];
        Position.max[c] = Batch.afMax[c];
      }

      cgltf_accessor &Normal = Accessors[uiFirst + 1];
      Normal.component_type = cgltf_component_type_r_32f;
      Normal.type = cgltf_type_vec3;
      Normal.count = uiVertices;
      Normal.buffer_view = &Views[uiFirst + 1];

      cgltf_accessor &TexCoord = Accessors[uiFirst + 2];
      TexCoord.component_type = cgltf_component_type_r_32f;
      TexCoord.type = cgltf_type_vec2;
      TexCoord.count = uiVertices;
      TexCoord.buffer_view = &Views[uiFirst + 2];

      cgltf_accessor &Index = Accessors[uiFirst + 3];
      Index.component_type = cgltf_component_type_r_32u;
      Index.type = cgltf_type_scalar;
      Index.count = Batch.Indices.size();
      Index.buffer_view = &Views[uiFirst + 3];
    }
  }

  std::vector<cgltf_image> Images(TextureForSet.size());
  std::vector<cgltf_sampler> Samplers(TextureForSet.empty() ? 0u : 1u);
  std::vector<cgltf_texture> Textures(TextureForSet.size());
  std::memset(Images.data(), 0, Images.size() * sizeof(cgltf_image));
  std::memset(Textures.data(), 0, Textures.size() * sizeof(cgltf_texture));
  if (!Samplers.empty()) {
    std::memset(Samplers.data(), 0, Samplers.size() * sizeof(cgltf_sampler));
    // Nearest, because the atlas packs tiles edge to edge: bilinear filtering
    // would bleed neighbouring tiles into every seam, and nearest is what the
    // legacy renderer did anyway. Clamped for the same reason - a tile's UVs
    // never leave the atlas, so wrapping would only ever be a bug.
    Samplers[0].mag_filter = cgltf_filter_type_nearest;
    Samplers[0].min_filter = cgltf_filter_type_nearest;
    Samplers[0].wrap_s = cgltf_wrap_mode_clamp_to_edge;
    Samplers[0].wrap_t = cgltf_wrap_mode_clamp_to_edge;
  }
  for (size_t i = 0; i < TextureForSet.size(); ++i) {
    const tEdGltfTextureSource &Source = Options.Textures[TextureForSet[i]];
    Images[i].name = Names.Add(
        CEditorExportConventions::TextureFileName(Options.sBaseName,
                                                  Source.uiTextureSet));
    if (bEmbedImages) {
      Images[i].buffer_view = &Views[uiAccessorCount + i];
      Images[i].mime_type = Names.Add("image/png");
    } else {
      Images[i].uri = Names.Add(Source.sUri);
    }
    Textures[i].image = &Images[i];
    Textures[i].sampler = Samplers.empty() ? nullptr : Samplers.data();
  }

  std::vector<cgltf_material> Materials(MaterialOrder.size());
  std::memset(Materials.data(), 0, Materials.size() * sizeof(cgltf_material));
  for (size_t k = 0; k < MaterialOrder.size(); ++k) {
    const tEdMaterial &Source = MaterialOrder[k].first;
    const bool bDoubleSided = MaterialOrder[k].second;
    cgltf_material &Material = Materials[k];

    Material.name = Names.Add(
        MaterialName(Options.sBaseName, Source, bDoubleSided));
    Material.double_sided = bDoubleSided ? 1 : 0;
    Material.has_pbr_metallic_roughness = 1;
    // These are unlit palette textures from 1997; a metallic, glossy surface
    // is not a thing the source data can describe.
    Material.pbr_metallic_roughness.metallic_factor = 0.0f;
    Material.pbr_metallic_roughness.roughness_factor = 1.0f;
    for (int c = 0; c < 4; ++c)
      Material.pbr_metallic_roughness.base_color_factor[c] = 1.0f;
    // MASK, unlike BLEND, needs no depth sorting, which matters for a mesh
    // this size.
    Material.alpha_mode = AlphaModeFor(Source);
    Material.alpha_cutoff = 0.5f;

    if (CEditorExportConventions::IsTexturedKind(Source.uiKind)) {
      const std::map<uint32_t, size_t>::const_iterator Found =
          TextureIndexBySet.find(Source.uiTextureSet);
      if (Found != TextureIndexBySet.end()) {
        Material.pbr_metallic_roughness.base_color_texture.texture =
            &Textures[Found->second];
        Material.pbr_metallic_roughness.base_color_texture.texcoord = 0;
        Material.pbr_metallic_roughness.base_color_texture.scale = 1.0f;
      }
    } else if (Source.uiKind == ROLLER_ED_MATERIAL_SCREEN_DARKEN) {
      // Never a texture. The documented approximation from E4-S1: black at the
      // opacity that leaves as much of the background as the renderer does.
      const float fAlpha = CEditorExportConventions::ScreenDarkenAlpha(
          Source.uiDarkenLevel);
      Material.pbr_metallic_roughness.base_color_factor[0] = 0.0f;
      Material.pbr_metallic_roughness.base_color_factor[1] = 0.0f;
      Material.pbr_metallic_roughness.base_color_factor[2] = 0.0f;
      Material.pbr_metallic_roughness.base_color_factor[3] = fAlpha;
    } else {
      tEdExportPaletteEntry Colour;
      if (pPalette && Source.uiPaletteColour < uiPaletteCount)
        Colour = pPalette[Source.uiPaletteColour];
      // baseColorFactor is linear while the palette is sRGB.
      Material.pbr_metallic_roughness.base_color_factor[0] =
          SrgbToLinear(static_cast<float>(Colour.byRed) / 255.0f);
      Material.pbr_metallic_roughness.base_color_factor[1] =
          SrgbToLinear(static_cast<float>(Colour.byGreen) / 255.0f);
      Material.pbr_metallic_roughness.base_color_factor[2] =
          SrgbToLinear(static_cast<float>(Colour.byBlue) / 255.0f);
    }
  }

  size_t uiPrimitiveCount = 0;
  for (size_t m = 0; m < Meshes.size(); ++m)
    uiPrimitiveCount += Meshes[m].Batches.size();

  std::vector<cgltf_attribute> Attributes(uiPrimitiveCount * 3);
  std::memset(Attributes.data(), 0,
              Attributes.size() * sizeof(cgltf_attribute));
  std::vector<cgltf_primitive> Primitives(uiPrimitiveCount);
  std::memset(Primitives.data(), 0,
              Primitives.size() * sizeof(cgltf_primitive));
  std::vector<cgltf_mesh> GltfMeshes(Meshes.size());
  std::memset(GltfMeshes.data(), 0, GltfMeshes.size() * sizeof(cgltf_mesh));
  std::vector<cgltf_node> Nodes(Meshes.size());
  std::memset(Nodes.data(), 0, Nodes.size() * sizeof(cgltf_node));
  std::vector<cgltf_node *> NodePointers(Meshes.size(), nullptr);

  size_t uiPrimitive = 0;
  for (size_t m = 0; m < Meshes.size(); ++m) {
    GltfMeshes[m].name = Names.Add(Meshes[m].sName);
    GltfMeshes[m].primitives = &Primitives[uiPrimitive];
    GltfMeshes[m].primitives_count = Meshes[m].Batches.size();

    for (size_t b = 0; b < Meshes[m].Batches.size(); ++b) {
      const tGltfBatch &Batch = Meshes[m].Batches[b];
      cgltf_primitive &Primitive = Primitives[uiPrimitive];
      Primitive.type = cgltf_primitive_type_triangles;
      Primitive.material = &Materials[Batch.uiMaterialIndex];
      Primitive.indices = &Accessors[Batch.uiFirstAccessor + 3];
      Primitive.attributes = &Attributes[uiPrimitive * 3];
      Primitive.attributes_count = 3;

      cgltf_attribute &Position = Attributes[uiPrimitive * 3 + 0];
      Position.name = Names.Add("POSITION");
      Position.type = cgltf_attribute_type_position;
      Position.index = 0;
      Position.data = &Accessors[Batch.uiFirstAccessor + 0];

      cgltf_attribute &Normal = Attributes[uiPrimitive * 3 + 1];
      Normal.name = Names.Add("NORMAL");
      Normal.type = cgltf_attribute_type_normal;
      Normal.index = 0;
      Normal.data = &Accessors[Batch.uiFirstAccessor + 1];

      cgltf_attribute &TexCoord = Attributes[uiPrimitive * 3 + 2];
      TexCoord.name = Names.Add("TEXCOORD_0");
      TexCoord.type = cgltf_attribute_type_texcoord;
      TexCoord.index = 0;
      TexCoord.data = &Accessors[Batch.uiFirstAccessor + 2];

      ++uiPrimitive;
    }

    // One node per mesh, named the same, so a Blender import produces objects
    // named after the surface classes rather than "mesh.001".
    Nodes[m].name = Names.Add(Meshes[m].sName);
    Nodes[m].mesh = &GltfMeshes[m];
    NodePointers[m] = &Nodes[m];
  }

  std::vector<cgltf_scene> Scenes(1);
  std::memset(Scenes.data(), 0, Scenes.size() * sizeof(cgltf_scene));
  Scenes[0].name = Names.Add(Options.sBaseName);
  Scenes[0].nodes = NodePointers.data();
  Scenes[0].nodes_count = NodePointers.size();

  Data.buffers = Buffers.data();
  Data.buffers_count = Buffers.size();
  Data.buffer_views = Views.data();
  Data.buffer_views_count = Views.size();
  Data.accessors = Accessors.data();
  Data.accessors_count = Accessors.size();
  Data.images = Images.empty() ? nullptr : Images.data();
  Data.images_count = Images.size();
  Data.samplers = Samplers.empty() ? nullptr : Samplers.data();
  Data.samplers_count = Samplers.size();
  Data.textures = Textures.empty() ? nullptr : Textures.data();
  Data.textures_count = Textures.size();
  Data.materials = Materials.data();
  Data.materials_count = Materials.size();
  Data.meshes = GltfMeshes.data();
  Data.meshes_count = GltfMeshes.size();
  Data.nodes = Nodes.data();
  Data.nodes_count = Nodes.size();
  Data.scenes = Scenes.data();
  Data.scenes_count = Scenes.size();
  Data.scene = Scenes.data();

  cgltf_options WriteOptions;
  std::memset(&WriteOptions, 0, sizeof(WriteOptions));
  WriteOptions.type = Data.file_type;

  const cgltf_size uiExpected = cgltf_write(&WriteOptions, nullptr, 0, &Data);
  if (uiExpected == 0) {
    sError = "the glTF writer produced nothing";
    return false;
  }
  std::vector<char> JsonBuffer(uiExpected);
  const cgltf_size uiActual =
      cgltf_write(&WriteOptions, JsonBuffer.data(), uiExpected, &Data);
  if (uiActual != uiExpected) {
    sError = "the glTF writer disagreed with itself about the document size";
    return false;
  }
  // cgltf_write counts a terminating NUL that does not belong in the file.
  Output.sJson.assign(JsonBuffer.data(), uiActual - 1);
  return true;
}

bool CEditorGltfExporter::ExportToFiles(const tEdExportGeometry &Geometry,
                                        const tEdGltfExportOptions &Options,
                                        const tEdExportPaletteEntry *pPalette,
                                        uint32_t uiPaletteCount,
                                        const std::string &sGltfFile,
                                        const std::string &sBinaryFile,
                                        std::string &sError)
{
  tEdGltfExportOutput Output;
  if (!Export(Geometry, Options, pPalette, uiPaletteCount, Output, sError))
    return false;

  if (Options.bBinary) {
    std::vector<uint8_t> Glb;
    PackGlb(Output.sJson, Output.Binary, Glb);
    return WriteWholeFile(sGltfFile, Glb.data(), Glb.size(), sError);
  }

  if (sBinaryFile.empty()) {
    sError = "a non-binary glTF export needs a path for its buffer";
    return false;
  }
  if (!WriteWholeFile(sBinaryFile, Output.Binary.data(), Output.Binary.size(),
                      sError)) {
    return false;
  }
  return WriteWholeFile(sGltfFile, Output.sJson.data(), Output.sJson.size(),
                        sError);
}
