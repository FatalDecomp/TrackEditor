#ifndef TRACKEDITOR_EDITOROBJEXPORTER_H
#define TRACKEDITOR_EDITOROBJEXPORTER_H

#include "EditorExportCommon.h"

#include <cstdint>
#include <ostream>
#include <string>

// E4-S1. The OBJ exporter's input is ROLLER's canonical geometry
// (tEdVertex / tEdPrimitive / tEdMaterial), never the editor's own CPU
// derivation: ROLLER is the geometric authority (AD-6a). Everything the
// exporters share - axis, scale, scope, grouping, reverse-side rules - lives in
// EditorExportCommon.h; only OBJ text lives here.
//
// This translation unit owns no Qt type, no WhipLib type, and calls no
// RollerEd_* entry point, so it unit-tests without a render worker, an event
// loop, or a loaded track. See docs/obj-export.md.

struct tEdObjExportOptions
{
  // Legacy "Include signs" checkbox, re-enabled in E4A-S6. False exports the
  // track body alone.
  bool bExportScenery = true;
  // Legacy "Sections" checkbox.
  bool bSeparateSections = true;
  // Legacy "Backs" checkbox.
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
  // Material name as it appears in both the OBJ usemtl and the MTL newmtl.
  static std::string MaterialName(const std::string &sBaseName,
                                  const tEdMaterial &Material);

  // Writes the OBJ and its MTL. Returns false and fills sError without
  // writing a usable file when the extraction is inconsistent.
  static bool Export(const tEdExportGeometry &Geometry,
                     const tEdObjExportOptions &Options,
                     const tEdExportPaletteEntry *pPalette,
                     uint32_t uiPaletteCount,
                     std::ostream &ObjStream,
                     std::ostream &MtlStream,
                     std::string &sError);

  // File-backed convenience wrapper. sObjFile and sMtlFile are native paths.
  static bool ExportToFiles(const tEdExportGeometry &Geometry,
                            const tEdObjExportOptions &Options,
                            const tEdExportPaletteEntry *pPalette,
                            uint32_t uiPaletteCount,
                            const std::string &sObjFile,
                            const std::string &sMtlFile,
                            std::string &sError);
};

#endif
