#include "EditorExportFormat.h"

#include <algorithm>
#include <cctype>

namespace
{
// Every format here is always offered. The menu in MainWindow.ui carries one
// action per entry, and tests/test_e4_s5_export_ui.py checks the two agree.
const tEdExportFormat g_aFormats[] = {
  { EXPORT_OBJ, "OBJ Files (*.obj)", "obj" },
  // The first clause is the default the dialog opens on, so a user who does
  // nothing but type a name gets the self-contained container.
  { EXPORT_GLTF, "glTF Binary (*.glb);;glTF JSON (*.gltf)", "glb" }
};
const uint32_t g_uiFormatCount =
    static_cast<uint32_t>(sizeof(g_aFormats) / sizeof(g_aFormats[0]));

std::string ToLower(const std::string &sValue)
{
  std::string sResult(sValue);
  std::transform(sResult.begin(), sResult.end(), sResult.begin(),
                 [](unsigned char c) {
                   return static_cast<char>(std::tolower(c));
                 });
  return sResult;
}

size_t LastSeparator(const std::string &sPath)
{
  const size_t uiSlash = sPath.find_last_of('/');
  const size_t uiBackslash = sPath.find_last_of('\\');
  if (uiSlash == std::string::npos)
    return uiBackslash;
  if (uiBackslash == std::string::npos)
    return uiSlash;
  return std::max(uiSlash, uiBackslash);
}
}

uint32_t CEditorExportFormats::Count()
{
  return g_uiFormatCount;
}

const tEdExportFormat &CEditorExportFormats::At(uint32_t uiIndex)
{
  return g_aFormats[uiIndex < g_uiFormatCount ? uiIndex : 0u];
}

const tEdExportFormat &CEditorExportFormats::For(eExportType eType)
{
  for (uint32_t i = 0; i < g_uiFormatCount; ++i) {
    if (g_aFormats[i].eType == eType)
      return g_aFormats[i];
  }
  return g_aFormats[0];
}

std::string CEditorExportFormats::SuffixFromFilter(const std::string &sFilter)
{
  // Qt hands back the single clause the user had selected, so only the first
  // "(*.ext)" in the string is relevant.
  const size_t uiOpen = sFilter.find("(*.");
  if (uiOpen == std::string::npos)
    return std::string();
  const size_t uiStart = uiOpen + 3;
  const size_t uiEnd = sFilter.find_first_of(") ;", uiStart);
  if (uiEnd == std::string::npos || uiEnd <= uiStart)
    return std::string();
  const std::string sSuffix = ToLower(sFilter.substr(uiStart, uiEnd - uiStart));
  // "*.*" is a wildcard, not an extension to append.
  return sSuffix == "*" ? std::string() : sSuffix;
}

std::string CEditorExportFormats::SuffixOf(const std::string &sPath)
{
  const size_t uiSeparator = LastSeparator(sPath);
  const size_t uiNameStart =
      uiSeparator == std::string::npos ? 0u : uiSeparator + 1u;
  const size_t uiDot = sPath.find_last_of('.');
  if (uiDot == std::string::npos || uiDot < uiNameStart
      || uiDot + 1u >= sPath.size()) {
    return std::string();
  }
  return ToLower(sPath.substr(uiDot + 1u));
}

std::string CEditorExportFormats::ApplyDefaultSuffix(
    const std::string &sPath, const std::string &sSelectedFilter,
    eExportType eType)
{
  if (sPath.empty() || !SuffixOf(sPath).empty())
    return sPath;

  std::string sSuffix = SuffixFromFilter(sSelectedFilter);
  if (sSuffix.empty())
    sSuffix = For(eType).szDefaultSuffix;
  if (sSuffix.empty())
    return sPath;
  return sPath + "." + sSuffix;
}

bool CEditorExportFormats::IsBinaryGltf(const std::string &sPath)
{
  return SuffixOf(sPath) == "glb";
}
