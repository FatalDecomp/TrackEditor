#include "TrackAssets.h"
#include "Palette.h"
#include "Texture.h"
#include <filesystem>
#include <utility>
//-------------------------------------------------------------------------------------------------

namespace
{
std::string ResolveDocumentAsset(const std::string &sDocumentAssetRoot,
                                 const std::string &sFilename)
{
  if (sDocumentAssetRoot.empty() || sFilename.empty())
    return std::string();

  return (std::filesystem::path(sDocumentAssetRoot) / sFilename)
      .lexically_normal().string();
}
}

//-------------------------------------------------------------------------------------------------

CTrackAssets::CTrackAssets() = default;
CTrackAssets::~CTrackAssets() = default;

//-------------------------------------------------------------------------------------------------

void CTrackAssets::Clear()
{
  m_pSignTexture.reset();
  m_pMainTexture.reset();
  m_pPalette.reset();
  m_sLastLoadedPalette.clear();
  m_sLastLoadedTexture.clear();
  m_sLastLoadedBuilding.clear();
}

//-------------------------------------------------------------------------------------------------

bool CTrackAssets::LoadFromDocument(const std::string &sDocumentAssetRoot,
                                    const std::string &sTextureFile,
                                    const std::string &sBuildingFile)
{
  const std::string sPalettePath =
      ResolveDocumentAsset(sDocumentAssetRoot, "PALETTE.PAL");
  const std::string sTexturePath =
      ResolveDocumentAsset(sDocumentAssetRoot, sTextureFile);
  const std::string sBuildingPath =
      ResolveDocumentAsset(sDocumentAssetRoot, sBuildingFile);

  if (sPalettePath.empty() || sTexturePath.empty() || sBuildingPath.empty())
    return false;

  if (IsLoaded()
      && m_sLastLoadedPalette == sPalettePath
      && m_sLastLoadedTexture == sTexturePath
      && m_sLastLoadedBuilding == sBuildingPath) {
    return true;
  }

  std::unique_ptr<CPalette> pPalette = std::make_unique<CPalette>();
  if (!pPalette->LoadPalette(sPalettePath))
    return false;

  std::unique_ptr<CTexture> pMainTexture = std::make_unique<CTexture>();
  if (!pMainTexture->LoadTexture(sTexturePath, pPalette.get()))
    return false;

  std::unique_ptr<CTexture> pSignTexture = std::make_unique<CTexture>();
  if (!pSignTexture->LoadTexture(sBuildingPath, pPalette.get()))
    return false;

  m_pSignTexture = std::move(pSignTexture);
  m_pMainTexture = std::move(pMainTexture);
  m_pPalette = std::move(pPalette);
  m_sLastLoadedPalette = sPalettePath;
  m_sLastLoadedTexture = sTexturePath;
  m_sLastLoadedBuilding = sBuildingPath;
  return true;
}

//-------------------------------------------------------------------------------------------------

bool CTrackAssets::ExportTextures(const std::string &sMainTexturePng,
                                  const std::string &sSignTexturePng) const
{
  if (!IsLoaded())
    return false;

  return m_pMainTexture->ExportToPngFile(sMainTexturePng)
      && m_pSignTexture->ExportToPngFile(sSignTexturePng);
}

//-------------------------------------------------------------------------------------------------

bool CTrackAssets::IsLoaded() const
{
  return m_pPalette && m_pPalette->IsLoaded()
      && m_pMainTexture && m_pMainTexture->IsLoaded()
      && m_pSignTexture && m_pSignTexture->IsLoaded();
}

//-------------------------------------------------------------------------------------------------
