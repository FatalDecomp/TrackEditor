#ifndef TRACKEDITOR_EDITORGLTFEXPORTER_H
#define TRACKEDITOR_EDITORGLTFEXPORTER_H

#include "EditorExportCommon.h"

#include <cstdint>
#include <string>
#include <vector>

// E4-S2. glTF 2.0 export over the vendored cgltf writer, reading the same
// canonical geometry the OBJ exporter reads. Everything the two share lives in
// EditorExportCommon.h; only the glTF document lives here.
//
// glTF fixes the coordinate system (right-handed, +Y up, -Z forward), the
// units (metres), and the front face (counter-clockwise), so this exporter has
// fewer choices to make than OBJ did - the shared conversion already lands
// exactly on them. See docs/gltf-export.md.
//
// This translation unit owns no Qt type, no WhipLib type, and calls no
// RollerEd_* entry point.

// One atlas the export references. Supply sUri for a .gltf that points at a
// PNG beside it, or PngBytes for a .glb that embeds the image in its binary
// chunk. Supplying neither drops the texture and leaves the material's base
// colour factor alone.
struct tEdGltfTextureSource
{
  uint32_t uiTextureSet = 0;
  std::string sUri;
  std::vector<uint8_t> PngBytes;
};

struct tEdGltfExportOptions
{
  // Legacy "Sections" checkbox: one glTF mesh and node per surface class, or
  // a single combined "Track" node.
  bool bSeparateSections = true;
  // Legacy "Backs" checkbox. Only reaches surfaces whose reverse side needs
  // its own geometry - see bDoubleSidedMaterials.
  bool bSeparateBackFaces = true;
  // glTF can say "draw both sides" on the material, so a merely two-sided
  // surface does not need a duplicated reverse-wound copy the way OBJ does.
  // A surface whose uiBackMaterialId names a *different* tile still gets real
  // geometry, because one material cannot address two tiles.
  bool bDoubleSidedMaterials = true;
  // True writes a self-contained .glb: JSON chunk plus one binary chunk
  // carrying the geometry and the embedded PNGs. False writes .gltf JSON that
  // references sBufferUri and each texture's sUri.
  bool bBinary = false;
  std::string sBaseName;
  // Written verbatim as the buffer's uri; ignored when bBinary is set.
  std::string sBufferUri;
  std::vector<tEdGltfTextureSource> Textures;
};

// The serialized document plus the binary payload it refers to. For .gltf the
// payload is the contents of sBufferUri; for .glb it is the BIN chunk and
// already includes the embedded images.
struct tEdGltfExportOutput
{
  std::string sJson;
  std::vector<uint8_t> Binary;
};

class CEditorGltfExporter
{
public:
  // Material name as it appears in the glTF materials array. A double-sided
  // material is named apart from its single-sided twin: glTF carries
  // double-sidedness on the material, so one canonical material used by both
  // kinds of surface has to become two.
  static std::string MaterialName(const std::string &sBaseName,
                                  const tEdMaterial &Material,
                                  bool bDoubleSided);

  // glTF baseColorFactor is linear; the palette and the PNGs are sRGB. A PNG
  // is decoded as sRGB by the viewer, but a factor is taken at face value, so
  // a flat colour has to be converted or it reads far too bright.
  static float SrgbToLinear(float fSrgb);

  // Builds the document without touching the filesystem. Used by the tests and
  // by ExportToFiles alike.
  static bool Export(const tEdExportGeometry &Geometry,
                     const tEdGltfExportOptions &Options,
                     const tEdExportPaletteEntry *pPalette,
                     uint32_t uiPaletteCount,
                     tEdGltfExportOutput &Output,
                     std::string &sError);

  // Writes <sGltfFile>, and for a non-binary export also <sBinaryFile>.
  // sBinaryFile is ignored when Options.bBinary is set, because the payload
  // goes into the .glb's own binary chunk.
  static bool ExportToFiles(const tEdExportGeometry &Geometry,
                            const tEdGltfExportOptions &Options,
                            const tEdExportPaletteEntry *pPalette,
                            uint32_t uiPaletteCount,
                            const std::string &sGltfFile,
                            const std::string &sBinaryFile,
                            std::string &sError);
};

#endif
