#ifndef TRACKEDITOR_EDITOREXPORTCOMMON_H
#define TRACKEDITOR_EDITOREXPORTCOMMON_H

#include "editor_api.h"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

// Epic 4. Everything the exporters must agree on lives here: the coordinate
// conversion, the unit scale, which content is in scope, how a reverse side is
// identified, how surfaces are grouped and named, and how a framebuffer
// darkening level is approximated. Only the file format itself belongs in an
// individual exporter.
//
// This is the point of the migration: OBJ (E4-S1) and glTF (E4-S2) - and any
// exporter added later - must not re-derive these against their own importer.
// (E4-S3 removed FBX outright rather than retargeting it.) Every value is
// stated relative to ROLLER's
// docs/adr/0003-canonical-geometry-conventions.md; changing one there is a
// breaking change for all of them.
//
// Nothing here owns a Qt type, a WhipLib type, or a RollerEd_* call, so the
// whole layer unit-tests without a worker, an event loop, or a loaded track.

// Legacy track units to exported metres. Preserved verbatim from the
// pre-migration exporter so an existing .blend built against an older export
// still lines up in scale.
#define ED_EXPORT_UNIT_SCALE 0.01f

// ROLLER world is right-handed with +Z up (ADR 0003). Both OBJ consumers and
// glTF itself expect right-handed +Y up, so the exporters rotate -90 degrees
// about X:
//   out = ( X, Z, -Y ) * ED_EXPORT_UNIT_SCALE
// The mapping preserves handedness, so the emitted vertex order already has
// the counter-clockwise front face on the side ROLLER calls front, and no
// winding flip is needed in any format.

// Palette entry used to resolve ROLLER_ED_MATERIAL_FLAT_PALETTE_COLOR. Passed
// in rather than read from CPalette so this layer keeps no track-assets
// dependency.
struct tEdExportPaletteEntry
{
  uint8_t byRed = 0;
  uint8_t byGreen = 0;
  uint8_t byBlue = 0;
};

// A borrowed view of one canonical extraction. Nothing here copies or retains
// it past the call it is passed to.
struct tEdExportGeometry
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

// One face-set to emit: a primitive taken from one of its two sides, with the
// vertices it uses already resolved to a local numbering.
struct tEdExportEntry
{
  uint32_t uiPrimitive = 0;
  uint32_t uiMaterial = 0;
  bool bBack = false;
  std::vector<uint32_t> Vertices;  // global vertex ids, first-use order
  std::vector<uint32_t> Triangles; // indices into Vertices
};

// A named group of face-sets: one exported object, mesh, or node.
struct tEdExportObject
{
  std::string sName;
  std::vector<tEdExportEntry> Entries;
};

struct tEdExportGrouping
{
  // Legacy "Sections" checkbox: one named group per surface class, or a
  // single combined "Track" group.
  bool bSeparateSections = true;
  // Legacy "Backs" checkbox. It only chooses whether generated reverse-side
  // faces land in their own "(Back)" group or are merged into the front one,
  // which is exactly what eBackModeling::FRONTS + BACKS versus
  // FRONTS_AND_BACKS did.
  bool bSeparateBackFaces = true;
  // Whether a two-sided surface that has no distinct back material needs
  // generated reverse geometry. OBJ has no way to say "draw both sides", so it
  // does; glTF says it with a double-sided material instead and sets this
  // false. A surface with a genuinely different uiBackMaterialId always gets
  // geometry, in every format, because one material cannot carry two tiles.
  bool bReverseSideAsGeometry = true;
};

class CEditorExportConventions
{
public:
  // The eleven canonical track surface classes, named exactly as the
  // pre-migration exporter named them. Returns nullptr for a class the
  // exporters do not emit (sign, building, tower).
  static const char *SurfaceClassName(uint16_t unSurfaceClass);
  static uint32_t ExportedSurfaceClassCount();
  static uint16_t ExportedSurfaceClass(uint32_t uiIndex);

  // AD-6d/AD-6e: authored content only. Editor helpers, markers, selection
  // overlays, the test car, and the reference model never reach the canonical
  // emitter at all, and runtime scenery is filtered here.
  static bool IsAuthoredContent(uint16_t unContentClass);

  // AD-7e open item 2b, resolved in E4-S1. A static format cannot express a
  // framebuffer darkening operation, so SCREEN_DARKEN becomes a black material
  // whose opacity reproduces the multiply the renderer performs: ROLLER
  // darkens what is behind the surface to g_sceneGpuShadeFactor[level] of its
  // brightness, and black composited at alpha d leaves (1 - d) of it, so
  // d = 1 - shadeFactor[level]. Levels past the table clamp to its last entry.
  // It is never emitted as a texture in any format.
  static float ScreenDarkenAlpha(uint32_t uiDarkenLevel);

  // ROLLER world (+Z up) to exported (+Y up), including the unit scale.
  static void ConvertPosition(const float afRollerXYZ[3], float afOutXYZ[3]);
  // Same rotation without the scale, so unit normals stay unit length.
  static void ConvertDirection(const float afRollerXYZ[3], float afOutXYZ[3]);

  static bool IsTexturedKind(uint32_t uiKind);
  // A missing back material does not imply single-sidedness (editor_api.h):
  // SURFACE_FLAG_CONCAVE surfaces are drawn from both sides with the same
  // material, and exporting them single-sided would leave holes (ADR 0003).
  static bool HasReverseSide(const tEdPrimitive &Primitive);
  // True when the reverse side needs a *different* material, which no format
  // can express without separate geometry.
  static bool HasDistinctReverseMaterial(const tEdPrimitive &Primitive);
  static uint32_t ReverseSideMaterial(const tEdPrimitive &Primitive);

  // Atlas PNG file name for a textured material's texture set, matching the
  // names CTrackAssets::ExportTextures writes.
  static std::string TextureFileName(const std::string &sBaseName,
                                     uint32_t uiTextureSet);

  // Rejects an extraction whose indices, ranges, or material ids do not hold
  // together, before any file is written.
  static bool ValidateGeometry(const tEdExportGeometry &Geometry,
                               std::string &sError);

  // Buckets every authored primitive into named groups, applying the scope
  // filter, the grouping options, and the reverse-side rules, then orders each
  // group's faces by material so a format that batches by material emits one
  // run rather than one batch per quad. Empty groups are dropped.
  static bool BuildObjects(const tEdExportGeometry &Geometry,
                           const tEdExportGrouping &Grouping,
                           std::vector<tEdExportObject> &ObjectsOut,
                           std::string &sError);
};

#endif
