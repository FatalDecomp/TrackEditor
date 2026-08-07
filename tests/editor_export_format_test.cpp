// E4-S5. The export format table and the file-name rules that decide which
// format - and for glTF, which container - actually gets written. Qt-free, so
// the rules are exercised without a save dialog.
#include "EditorExportFormat.h"

#include <cstdio>
#include <cstdlib>
#include <string>

namespace
{
void RequireImpl(bool bCondition, const char *szExpression, int iLine)
{
  if (!bCondition) {
    std::fprintf(stderr, "requirement failed at line %d: %s\n", iLine,
                 szExpression);
    std::abort();
  }
}
#define Require(condition) RequireImpl((condition), #condition, __LINE__)

void TestEveryFormatIsAlwaysOffered()
{
  // The story's acceptance criterion, minus the FBX clause E4-S3 made moot:
  // OBJ and glTF are both offered, unconditionally, with nothing to compile
  // in or out.
  Require(CEditorExportFormats::Count() == 2);

  bool bSawObj = false;
  bool bSawGltf = false;
  for (uint32_t i = 0; i < CEditorExportFormats::Count(); ++i) {
    const tEdExportFormat &Format = CEditorExportFormats::At(i);
    Require(Format.szDialogFilter != nullptr);
    Require(Format.szDefaultSuffix != nullptr);
    Require(std::string(Format.szDefaultSuffix).find('.') == std::string::npos);
    // Every format's default suffix has to be one its own filter offers, or
    // the dialog and the writer disagree.
    Require(!CEditorExportFormats::SuffixFromFilter(Format.szDialogFilter)
                 .empty());
    bSawObj = bSawObj || Format.eType == EXPORT_OBJ;
    bSawGltf = bSawGltf || Format.eType == EXPORT_GLTF;
  }
  Require(bSawObj);
  Require(bSawGltf);

  Require(CEditorExportFormats::For(EXPORT_OBJ).eType == EXPORT_OBJ);
  Require(CEditorExportFormats::For(EXPORT_GLTF).eType == EXPORT_GLTF);
  // The first glTF clause is the container a user gets by doing nothing.
  Require(CEditorExportFormats::SuffixFromFilter(
              CEditorExportFormats::For(EXPORT_GLTF).szDialogFilter)
          == "glb");
  Require(std::string(CEditorExportFormats::For(EXPORT_OBJ).szDefaultSuffix)
          == "obj");
}

void TestSuffixFromFilter()
{
  Require(CEditorExportFormats::SuffixFromFilter("OBJ Files (*.obj)")
          == "obj");
  Require(CEditorExportFormats::SuffixFromFilter("glTF Binary (*.glb)")
          == "glb");
  Require(CEditorExportFormats::SuffixFromFilter("glTF JSON (*.gltf)")
          == "gltf");
  // Qt hands back one clause, but a whole multi-clause filter must still
  // resolve to its first extension rather than to nothing.
  Require(CEditorExportFormats::SuffixFromFilter(
              "glTF Binary (*.glb);;glTF JSON (*.gltf)") == "glb");
  // Case is normalised, so a comparison never has to think about it.
  Require(CEditorExportFormats::SuffixFromFilter("Models (*.OBJ)") == "obj");
  // Nothing to append.
  Require(CEditorExportFormats::SuffixFromFilter("All Files (*.*)").empty());
  Require(CEditorExportFormats::SuffixFromFilter("no extension here").empty());
  Require(CEditorExportFormats::SuffixFromFilter("").empty());
}

void TestSuffixOf()
{
  Require(CEditorExportFormats::SuffixOf("track.obj") == "obj");
  Require(CEditorExportFormats::SuffixOf("track.GLB") == "glb");
  Require(CEditorExportFormats::SuffixOf("C:\\a\\b\\track.gltf") == "gltf");
  Require(CEditorExportFormats::SuffixOf("/home/a/track.gltf") == "gltf");
  Require(CEditorExportFormats::SuffixOf("track").empty());
  // A dot in a directory name is not the file's suffix.
  Require(CEditorExportFormats::SuffixOf("C:\\my.tracks\\export").empty());
  Require(CEditorExportFormats::SuffixOf("/my.tracks/export").empty());
  // A trailing dot names no extension.
  Require(CEditorExportFormats::SuffixOf("track.").empty());
  Require(CEditorExportFormats::SuffixOf("").empty());
}

void TestApplyDefaultSuffix()
{
  // The case this exists for: Qt's own dialog appends nothing, so a user who
  // picked the binary container and typed a bare name would otherwise get
  // JSON in a file with no extension.
  Require(CEditorExportFormats::ApplyDefaultSuffix(
              "/tracks/mytrack", "glTF Binary (*.glb)", EXPORT_GLTF)
          == "/tracks/mytrack.glb");
  Require(CEditorExportFormats::ApplyDefaultSuffix(
              "/tracks/mytrack", "glTF JSON (*.gltf)", EXPORT_GLTF)
          == "/tracks/mytrack.gltf");
  Require(CEditorExportFormats::ApplyDefaultSuffix(
              "C:\\tracks\\mytrack", "OBJ Files (*.obj)", EXPORT_OBJ)
          == "C:\\tracks\\mytrack.obj");

  // A name the user did give an extension is left exactly as typed, including
  // one that disagrees with the selected filter: that is their choice, and for
  // glTF it is how you ask for the other container.
  Require(CEditorExportFormats::ApplyDefaultSuffix(
              "/tracks/mytrack.gltf", "glTF Binary (*.glb)", EXPORT_GLTF)
          == "/tracks/mytrack.gltf");
  Require(CEditorExportFormats::ApplyDefaultSuffix(
              "/tracks/mytrack.obj", "OBJ Files (*.obj)", EXPORT_OBJ)
          == "/tracks/mytrack.obj");

  // No usable filter falls back to the format's own default rather than
  // writing a suffix-less file.
  Require(CEditorExportFormats::ApplyDefaultSuffix(
              "/tracks/mytrack", "All Files (*.*)", EXPORT_GLTF)
          == "/tracks/mytrack.glb");
  Require(CEditorExportFormats::ApplyDefaultSuffix("/tracks/mytrack", "",
                                                   EXPORT_OBJ)
          == "/tracks/mytrack.obj");

  // A directory containing a dot does not count as the file having one.
  Require(CEditorExportFormats::ApplyDefaultSuffix(
              "C:\\my.tracks\\export", "OBJ Files (*.obj)", EXPORT_OBJ)
          == "C:\\my.tracks\\export.obj");

  Require(CEditorExportFormats::ApplyDefaultSuffix(
              "", "OBJ Files (*.obj)", EXPORT_OBJ).empty());
}

void TestGltfContainerChoice()
{
  Require(CEditorExportFormats::IsBinaryGltf("track.glb"));
  Require(CEditorExportFormats::IsBinaryGltf("C:\\a\\track.GLB"));
  Require(!CEditorExportFormats::IsBinaryGltf("track.gltf"));
  Require(!CEditorExportFormats::IsBinaryGltf("track"));

  // End to end: what the dialog returns, through the suffix rule, decides the
  // container. This is the chain that was broken before E4-S5.
  const std::string sTyped = "/tracks/mytrack";
  Require(CEditorExportFormats::IsBinaryGltf(
      CEditorExportFormats::ApplyDefaultSuffix(
          sTyped, "glTF Binary (*.glb)", EXPORT_GLTF)));
  Require(!CEditorExportFormats::IsBinaryGltf(
      CEditorExportFormats::ApplyDefaultSuffix(
          sTyped, "glTF JSON (*.gltf)", EXPORT_GLTF)));
}
}

int main()
{
  TestEveryFormatIsAlwaysOffered();
  TestSuffixFromFilter();
  TestSuffixOf();
  TestApplyDefaultSuffix();
  TestGltfContainerChoice();
  std::puts("editor export format tests passed");
  return 0;
}
