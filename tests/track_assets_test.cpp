#include "Palette.h"
#include "Texture.h"
#include "TrackAssets.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace
{
void RequireImpl(bool bCondition, const char *szExpression, int iLine)
{
  if (!bCondition) {
    std::fprintf(stderr, "requirement failed at line %d: %s\n", iLine, szExpression);
    std::abort();
  }
}
#define Require(condition) RequireImpl((condition), #condition, __LINE__)

class CTemporaryAssetDirectory
{
public:
  CTemporaryAssetDirectory()
  {
    const auto ullSuffix = static_cast<unsigned long long>(
        std::chrono::steady_clock::now().time_since_epoch().count());
    m_path = std::filesystem::temp_directory_path()
        / ("trackeditor-e3-s5b-" + std::to_string(ullSuffix));
    Require(std::filesystem::create_directories(m_path));
  }

  ~CTemporaryAssetDirectory()
  {
    std::error_code Error;
    std::filesystem::remove_all(m_path, Error);
  }

  const std::filesystem::path &Path() const { return m_path; }

private:
  std::filesystem::path m_path;
};

void WriteBytes(const std::filesystem::path &path,
                const std::vector<std::uint8_t> &data)
{
  std::ofstream file(path, std::ios::binary);
  Require(file.is_open());
  file.write(reinterpret_cast<const char *>(data.data()),
             static_cast<std::streamsize>(data.size()));
  Require(file.good());
}

std::vector<std::uint8_t> MangleAsLiterals(const std::vector<std::uint8_t> &source)
{
  const std::uint32_t uiSourceSize = static_cast<std::uint32_t>(source.size());
  std::vector<std::uint8_t> mangled(4);
  for (int i = 0; i < 4; ++i)
    mangled[static_cast<size_t>(i)] =
        static_cast<std::uint8_t>(uiSourceSize >> (i * 8));

  for (size_t iOffset = 0; iOffset < source.size();) {
    const size_t iLiteralCount = std::min<size_t>(0x3f, source.size() - iOffset);
    mangled.push_back(static_cast<std::uint8_t>(iLiteralCount));
    mangled.insert(mangled.end(), source.begin() + iOffset,
                   source.begin() + iOffset + iLiteralCount);
    iOffset += iLiteralCount;
  }
  return mangled;
}

std::uint32_t ReadPngDimension(const std::filesystem::path &path, size_t iOffset)
{
  std::ifstream file(path, std::ios::binary);
  Require(file.is_open());
  std::array<std::uint8_t, 24> header = {};
  file.read(reinterpret_cast<char *>(header.data()),
            static_cast<std::streamsize>(header.size()));
  Require(file.good());
  const std::array<std::uint8_t, 8> pngSignature = {
      0x89, 'P', 'N', 'G', 0x0d, 0x0a, 0x1a, 0x0a};
  for (size_t i = 0; i < pngSignature.size(); ++i)
    Require(header[i] == pngSignature[i]);

  return (static_cast<std::uint32_t>(header[iOffset]) << 24)
      | (static_cast<std::uint32_t>(header[iOffset + 1]) << 16)
      | (static_cast<std::uint32_t>(header[iOffset + 2]) << 8)
      | static_cast<std::uint32_t>(header[iOffset + 3]);
}
}

int main()
{
  CTemporaryAssetDirectory directory;

  std::vector<std::uint8_t> palette(PALETTE_SIZE * 3, 0);
  palette[3] = 10;
  palette[4] = 20;
  palette[5] = 30;
  palette[6] = 5;
  palette[7] = 15;
  palette[8] = 25;
  WriteBytes(directory.Path() / "PALETTE.PAL", palette);

  const size_t iTileBytes = static_cast<size_t>(TILE_WIDTH * TILE_HEIGHT);
  std::vector<std::uint8_t> mainTexture(iTileBytes, 1);
  WriteBytes(directory.Path() / "MAIN.DRH", mainTexture);
  WriteBytes(directory.Path() / "SIGNS.DRH",
             MangleAsLiterals(std::vector<std::uint8_t>(iTileBytes, 2)));

  CTrackAssets assets;
  Require(assets.LoadFromDocument(directory.Path().string(), "MAIN.DRH", "SIGNS.DRH"));
  Require(assets.IsLoaded());
  Require(assets.GetPalette());
  Require(assets.GetMainTexture());
  Require(assets.GetSignTexture());
  Require(assets.GetMainTexture()->GetNumTiles() == 1);
  Require(assets.GetSignTexture()->GetNumTiles() == 1);

  const auto &mainPixel = assets.GetMainTexture()->m_pTileAy[0].data[0][0];
  Require(mainPixel.r == 40);
  Require(mainPixel.g == 80);
  Require(mainPixel.b == 120);
  Require(mainPixel.a == 255);

  const auto &signPixel = assets.GetSignTexture()->m_pTileAy[0].data[0][0];
  Require(signPixel.r == 20);
  Require(signPixel.g == 60);
  Require(signPixel.b == 100);
  Require(signPixel.a == 255);

  const std::filesystem::path mainPng = directory.Path() / "track.png";
  const std::filesystem::path signPng = directory.Path() / "track_BLD.png";
  Require(assets.ExportTextures(mainPng.string(), signPng.string()));
  Require(ReadPngDimension(mainPng, 16) == TILE_WIDTH);
  Require(ReadPngDimension(mainPng, 20) == TILE_HEIGHT * 3);
  Require(ReadPngDimension(signPng, 16) == TILE_WIDTH);
  Require(ReadPngDimension(signPng, 20) == TILE_HEIGHT * 3);

  CTexture *pLastGoodMain = assets.GetMainTexture();
  Require(!assets.LoadFromDocument(directory.Path().string(), "MISSING.DRH", "SIGNS.DRH"));
  Require(assets.GetMainTexture() == pLastGoodMain);
  Require(assets.IsLoaded());

  Require(assets.LoadFromDocument(directory.Path().string(), "MAIN.DRH", "SIGNS.DRH"));
  assets.Clear();
  Require(!assets.IsLoaded());
  Require(!assets.ExportTextures(mainPng.string(), signPng.string()));
  return 0;
}
