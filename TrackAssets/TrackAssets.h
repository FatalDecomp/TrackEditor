#ifndef _TRACKEDITOR_TRACKASSETS_H
#define _TRACKEDITOR_TRACKASSETS_H
//-------------------------------------------------------------------------------------------------
#include <memory>
#include <string>
//-------------------------------------------------------------------------------------------------
class CPalette;
class CTexture;
//-------------------------------------------------------------------------------------------------

class CTrackAssets
{
public:
  CTrackAssets();
  ~CTrackAssets();

  CTrackAssets(const CTrackAssets &) = delete;
  CTrackAssets &operator=(const CTrackAssets &) = delete;

  void Clear();
  bool LoadFromDocument(const std::string &sDocumentAssetRoot,
                        const std::string &sTextureFile,
                        const std::string &sBuildingFile,
                        const std::string &sFallbackAssetRoot = {});
  bool ExportTextures(const std::string &sMainTexturePng,
                      const std::string &sSignTexturePng) const;
  bool IsLoaded() const;

  CPalette *GetPalette() { return m_pPalette.get(); }
  const CPalette *GetPalette() const { return m_pPalette.get(); }
  CTexture *GetMainTexture() { return m_pMainTexture.get(); }
  const CTexture *GetMainTexture() const { return m_pMainTexture.get(); }
  CTexture *GetSignTexture() { return m_pSignTexture.get(); }
  const CTexture *GetSignTexture() const { return m_pSignTexture.get(); }

private:
  std::unique_ptr<CPalette> m_pPalette;
  std::unique_ptr<CTexture> m_pMainTexture;
  std::unique_ptr<CTexture> m_pSignTexture;
  std::string m_sLastLoadedPalette;
  std::string m_sLastLoadedTexture;
  std::string m_sLastLoadedBuilding;
};

//-------------------------------------------------------------------------------------------------
#endif
