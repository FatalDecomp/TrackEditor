#ifndef TRACKEDITOR_EDITOROBJEXPORTER_H
#define TRACKEDITOR_EDITOROBJEXPORTER_H

#include "editor_api.h"

#include <cstdint>
#include <ostream>
#include <string>

// E4-S1. The OBJ exporter's input is ROLLER's canonical geometry
// (tEdVertex / tEdPrimitive / tEdMaterial), never the editor's own CPU
// derivation: ROLLER is the geometric authority (AD-6a). This translation unit
// owns no Qt type, no WhipLib type, and calls no RollerEd_* entry point, so it
// unit-tests without a render worker, an event loop, or a loaded track.
//
// Conventions are stated relative to ROLLER's
// docs/adr/0003-canonical-geometry-conventions.md rather than derived
// empirically, per that ADR's consequences. See docs/obj-export.md.

// Legacy track units to exported metres. Preserved verbatim from the
// pre-migration exporter so an existing .blend built against an older export
// still lines up in scale.
#define ED_OBJ_EXPORT_UNIT_SCALE 0.01f

// ROLLER world is right-handed with +Z up (ADR 0003). OBJ consumers expect
// right-handed +Y up, so the exporter rotates -90 degrees about X:
//   obj = ( X, Z, -Y ) * ED_OBJ_EXPORT_UNIT_SCALE
// The mapping preserves handedness, so the emitted vertex order already has
// OBJ's counter-clockwise front face on the side ROLLER calls front and no
// winding flip is needed.

// Palette entry used to resolve ROLLER_ED_MATERIAL_FLAT_PALETTE_COLOR. Passed
// in rather than read from CPalette so the exporter keeps no track-assets
// dependency.
struct tEdObjExportPaletteEntry
{
  uint8_t byRed = 0;
  uint8_t byGreen = 0;
  uint8_t byBlue = 0;
};

// A borrowed view of one canonical extraction. The exporter copies nothing and
// retains nothing past the Export call.
struct tEdObjExportGeometry
{
  const tEdVertex *pVertices = nullptr;
  uint32_t uiVertexCount = 0;
  const uint32_t *puiIndices = nullptr;
  uint32_t uiIndexCount = 0;
  const tEdPrimitive *pPrimitives = nullptr;
  uint32_t uiPrimitiveCount = 0;
  const tEdMaterial *pMaterials = nullptr;
  uint32_t uiMaterialCount = 0;
};

struct tEdObjExportOptions
{
  // Legacy "Sections" checkbox: one named object per surface class, or a
  // single combined "Track" object.
  bool bSeparateSections = true;
  // Legacy "Backs" checkbox. Reverse-side faces are always generated where a
  // reverse side exists; this only chooses whether they land in their own
  // "(Back)" objects or are merged into the front object, which is exactly
  // what eBackModeling::FRONTS + BACKS versus FRONTS_AND_BACKS did.
  bool bSeparateBackFaces = true;
  // Base name of the export, without extension. Drives the .mtl material
  // names and the atlas PNG file names, as the legacy exporter did.
  std::string sBaseName;
  // Written verbatim into the OBJ's mtllib line.
  std::string sMtlFileName;
};

class CEditorObjExporter
{
public:
  // The eleven canonical track surface classes, named exactly as the
  // pre-migration exporter named them. Returns nullptr for a class this
  // exporter does not emit (sign, building, tower).
  static const char *SurfaceClassName(uint16_t unSurfaceClass);

  // AD-6d/AD-6e: authored content only. Editor helpers, markers, selection
  // overlays, the test car, and the reference model never reach the canonical
  // emitter at all, and runtime scenery is filtered here.
  static bool IsAuthoredContent(uint16_t unContentClass);

  // AD-7e open item 2b, resolved. A static format cannot express a
  // framebuffer darkening operation, so SCREEN_DARKEN becomes a documented
  // black material whose dissolve reproduces the multiply the renderer
  // performs: ROLLER darkens what is behind the surface to
  // g_sceneGpuShadeFactor[level] of its brightness, and black composited at
  // alpha d leaves (1 - d) of it, so d = 1 - shadeFactor[level]. Levels past
  // the table clamp to its last entry. It is never emitted as a texture.
  static float ScreenDarkenAlpha(uint32_t uiDarkenLevel);

  // ROLLER world (+Z up) to OBJ (+Y up), including the unit scale.
  static void ConvertPosition(const float afRollerXYZ[3], float afObjXYZ[3]);
  // Same rotation without the scale, so unit normals stay unit length.
  static void ConvertDirection(const float afRollerXYZ[3], float afObjXYZ[3]);

  // Material name as it appears in both the OBJ usemtl and the MTL newmtl.
  static std::string MaterialName(const std::string &sBaseName,
                                  const tEdMaterial &Material);
  // Atlas PNG file name for a textured material's texture set, matching the
  // names CTrackAssets::ExportTextures writes.
  static std::string TextureFileName(const std::string &sBaseName,
                                     uint32_t uiTextureSet);

  // Writes the OBJ and its MTL. Returns false and fills sError without
  // writing a usable file when the extraction is inconsistent.
  static bool Export(const tEdObjExportGeometry &Geometry,
                     const tEdObjExportOptions &Options,
                     const tEdObjExportPaletteEntry *pPalette,
                     uint32_t uiPaletteCount,
                     std::ostream &ObjStream,
                     std::ostream &MtlStream,
                     std::string &sError);

  // File-backed convenience wrapper. sObjFile and sMtlFile are native paths.
  static bool ExportToFiles(const tEdObjExportGeometry &Geometry,
                            const tEdObjExportOptions &Options,
                            const tEdObjExportPaletteEntry *pPalette,
                            uint32_t uiPaletteCount,
                            const std::string &sObjFile,
                            const std::string &sMtlFile,
                            std::string &sError);
};

#endif
