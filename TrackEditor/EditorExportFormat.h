#ifndef TRACKEDITOR_EDITOREXPORTFORMAT_H
#define TRACKEDITOR_EDITOREXPORTFORMAT_H

#include <cstdint>
#include <string>

// E4-S5. The export formats the editor offers, and everything about choosing a
// file name for one: the save-dialog filter, the extension that filter implies,
// and how the glTF container is picked. One table rather than switch statements
// spread across the UI.
//
// E4-S3 removed FBX outright, so the story's "hide FBX when compiled out" no
// longer has anything to hide: every format in this table is always offered.
//
// Qt-free on purpose, so the file-name rules are unit-tested without a dialog.

// The container a glTF export writes is chosen from the file name, so this
// enum stops at the format and does not enumerate .gltf versus .glb.
enum eExportType
{
  EXPORT_OBJ = 0,
  EXPORT_GLTF
};

struct tEdExportFormat
{
  eExportType eType;
  // Qt save-dialog filter. glTF offers two clauses because its two containers
  // are the same exporter with a different file name.
  const char *szDialogFilter;
  // Used when the user types a name with no extension and the platform's
  // dialog does not add one.
  const char *szDefaultSuffix;
};

class CEditorExportFormats
{
public:
  static uint32_t Count();
  static const tEdExportFormat &At(uint32_t uiIndex);
  static const tEdExportFormat &For(eExportType eType);

  // Pulls the extension out of one filter clause: "glTF Binary (*.glb)" gives
  // "glb". Empty when the clause names no single extension.
  static std::string SuffixFromFilter(const std::string &sFilter);

  // The extension of a path, lowercased, empty when it has none. Only looks
  // after the last separator, so a dot in a directory name is not a suffix.
  static std::string SuffixOf(const std::string &sPath);

  // The path the export should actually write. Qt's static getSaveFileName
  // sets no default suffix, and only the Windows native dialog adds one, so a
  // name typed without an extension arrives bare. Appending the one the
  // selected filter implies is what keeps the chosen format the format that
  // gets written - it decides the glTF container outright.
  static std::string ApplyDefaultSuffix(const std::string &sPath,
                                        const std::string &sSelectedFilter,
                                        eExportType eType);

  // A glTF export writes one self-contained binary for .glb and JSON beside
  // its buffer for anything else.
  static bool IsBinaryGltf(const std::string &sPath);
};

#endif
