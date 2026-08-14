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

// E4-S4. The exported atlas has to be addressable by the transforms ROLLER
// publishes on its materials, or every canonical UV lands somewhere else.
// This reproduces ROLLER's own arithmetic (editor_surface.c ed_build_material,
// itself derived from polytex.c's row = index >> 2, col = index & 3) as the
// specification, and checks the atlas satisfies it.
void TestCanonicalExportAtlas()
{
  CTemporaryAssetDirectory directory;

  // The palette file holds 6-bit VGA values that the loader shifts left by
  // two, so entries stay under 64 to keep every index a distinct colour.
  // Index i therefore decodes to red 4i.
  std::vector<std::uint8_t> palette(PALETTE_SIZE * 3, 0);
  for (int i = 0; i < 64; ++i)
    palette[static_cast<size_t>(i) * 3 + 0] = static_cast<std::uint8_t>(i);
  WriteBytes(directory.Path() / "PALETTE.PAL", palette);

  // Six tiles, so the last row is half empty and the padding is exercised.
  const int iContentTiles = 6;
  const size_t iTileBytes = static_cast<size_t>(TILE_WIDTH * TILE_HEIGHT);
  std::vector<std::uint8_t> texture(iTileBytes * iContentTiles);
  for (int i = 0; i < iContentTiles; ++i) {
    std::fill(texture.begin() + static_cast<size_t>(i) * iTileBytes,
              texture.begin() + static_cast<size_t>(i + 1) * iTileBytes,
              static_cast<std::uint8_t>(i + 1));
  }
  // Orientation markers inside tile 0. The source is row-major, so linear
  // index 1 is the pixel to the right of the origin and index TILE_WIDTH is
  // the one below it. Indices 10-12 are clear of the 1-6 the tiles use.
  texture[0] = 10;
  texture[1] = 11;
  texture[TILE_WIDTH] = 12;
  WriteBytes(directory.Path() / "MAIN.DRH", texture);
  WriteBytes(directory.Path() / "SIGNS.DRH",
             MangleAsLiterals(std::vector<std::uint8_t>(iTileBytes, 1)));

  CTrackAssets assets;
  Require(assets.LoadFromDocument(directory.Path().string(), "MAIN.DRH",
                                  "SIGNS.DRH"));
  const CTexture *pTexture = assets.GetMainTexture();
  Require(pTexture->GetNumTiles() == iContentTiles);

  int iWidth = 0;
  int iHeight = 0;
  int iSize = 0;
  std::uint8_t *pAtlas = pTexture->GenerateExportAtlas(iWidth, iHeight, iSize);
  Require(pAtlas != nullptr);

  const int iTilesPerRow = CTexture::GetExportTilesPerRow();
  Require(iTilesPerRow == 4);
  Require(iWidth == EXPORT_ATLAS_WIDTH);
  // Six tiles at four per row is two rows, not six.
  Require(iHeight == TILE_HEIGHT * 2);
  Require(iSize == 4 * iWidth * iHeight);

  const auto Pixel = [&](int x, int y) {
    return pAtlas + (static_cast<size_t>(y) * iWidth + static_cast<size_t>(x))
        * 4u;
  };

  // Every tile sits where ROLLER's transform says it does. Resolving the
  // material-local origin (0,0) through scale/bias must land on that tile's
  // top-left pixel, and (1,1) just inside its opposite corner.
  for (int i = 0; i < iContentTiles; ++i) {
    const float fScaleU = static_cast<float>(TILE_WIDTH)
        / static_cast<float>(iWidth);
    const float fScaleV = static_cast<float>(TILE_HEIGHT)
        / static_cast<float>(iHeight);
    const float fBiasU = static_cast<float>((i % iTilesPerRow) * TILE_WIDTH)
        / static_cast<float>(iWidth);
    const float fBiasV = static_cast<float>((i / iTilesPerRow) * TILE_HEIGHT)
        / static_cast<float>(iHeight);

    const int iOriginX = static_cast<int>(fBiasU * iWidth);
    const int iOriginY = static_cast<int>(fBiasV * iHeight);
    const int iFarX = static_cast<int>((fBiasU + fScaleU) * iWidth) - 1;
    const int iFarY = static_cast<int>((fBiasV + fScaleV) * iHeight) - 1;

    // Tile i was filled with palette index i + 1, except for tile 0's three
    // marker pixels, so probe a corner that is never a marker.
    const std::uint8_t byExpected = static_cast<std::uint8_t>((i + 1) * 4);
    Require(Pixel(iFarX, iFarY)[0] == byExpected);
    Require(Pixel(iOriginX, iFarY)[0] == byExpected);
    Require(Pixel(iFarX, iOriginY)[0] == byExpected);
    Require(Pixel(iFarX, iFarY)[3] == 255);
  }

  // Tile 0 is upright: no transpose and no vertical flip. Its origin marker is
  // at the atlas origin, the next source pixel is to its right, and the pixel
  // one source row on is below it.
  Require(Pixel(0, 0)[0] == 10 * 4);
  Require(Pixel(1, 0)[0] == 11 * 4);
  Require(Pixel(0, 1)[0] == 12 * 4);

  // The two padding slots in the last row are transparent, never a repeat of
  // an earlier tile.
  for (int i = iContentTiles; i < 2 * iTilesPerRow; ++i) {
    const int iPadX = (i % iTilesPerRow) * TILE_WIDTH;
    const int iPadY = (i / iTilesPerRow) * TILE_HEIGHT;
    Require(Pixel(iPadX, iPadY)[3] == 0);
    Require(Pixel(iPadX + TILE_WIDTH - 1, iPadY + TILE_HEIGHT - 1)[3] == 0);
  }

  delete[] pAtlas;

  // The legacy single-column bitmap is untouched: WhipLib's C API publishes
  // it and WhipLib::TextureMapping computes UVs against it.
  int iLegacySize = 0;
  std::uint8_t *pLegacy = pTexture->GenerateBitmapData(iLegacySize);
  Require(pLegacy != nullptr);
  Require(iLegacySize
          == 4 * TILE_WIDTH * TILE_HEIGHT * pTexture->GetAtlasTileCount());
  delete[] pLegacy;

  // And the PNG on disk carries the canonical dimensions.
  const std::filesystem::path mainPng = directory.Path() / "canonical.png";
  const std::filesystem::path signPng = directory.Path() / "canonical_BLD.png";
  Require(assets.ExportTextures(mainPng.string(), signPng.string()));
  Require(ReadPngDimension(mainPng, 16)
          == static_cast<std::uint32_t>(EXPORT_ATLAS_WIDTH));
  Require(ReadPngDimension(mainPng, 20)
          == static_cast<std::uint32_t>(TILE_HEIGHT * 2));
}

void TestRendererPairRightTile()
{
  CTemporaryAssetDirectory directory;

  std::vector<std::uint8_t> palette(PALETTE_SIZE * 3, 0);
  for (int i = 0; i < 64; ++i)
    palette[static_cast<size_t>(i) * 3] = static_cast<std::uint8_t>(i);
  WriteBytes(directory.Path() / "PALETTE.PAL", palette);

  const int iContentTiles = 6;
  const size_t iTileBytes = static_cast<size_t>(TILE_WIDTH * TILE_HEIGHT);
  std::vector<std::uint8_t> texture(iTileBytes * iContentTiles, 0);
  for (int i = 0; i < iContentTiles; ++i) {
    for (int y = 0; y < TILE_HEIGHT; ++y) {
      const std::uint8_t byRowMarker =
          static_cast<std::uint8_t>(i * 10 + y);
      std::fill(texture.begin() + static_cast<size_t>(i) * iTileBytes
                    + static_cast<size_t>(y) * TILE_WIDTH,
                texture.begin() + static_cast<size_t>(i) * iTileBytes
                    + static_cast<size_t>(y + 1) * TILE_WIDTH,
                byRowMarker);
    }
  }
  WriteBytes(directory.Path() / "MAIN.DRH", texture);
  WriteBytes(directory.Path() / "SIGNS.DRH",
             MangleAsLiterals(std::vector<std::uint8_t>(iTileBytes, 1)));

  CTrackAssets assets;
  Require(assets.LoadFromDocument(directory.Path().string(), "MAIN.DRH",
                                  "SIGNS.DRH"));
  const CTexture *pTexture = assets.GetMainTexture();
  Require(pTexture != nullptr);

  // A normal pair takes its right half directly from the next logical tile.
  tTile PairRight = {};
  Require(pTexture->GeneratePairRightTile(1, PairRight));
  for (int y = 0; y < TILE_HEIGHT; ++y) {
    Require(PairRight.data[0][y].r
            == pTexture->m_pTileAy[2].data[0][y].r);
  }

  // Tile 3 ends the first four-wide atlas row. ROLLER does not jump to all of
  // tile 4: its flat 128-pixel read wraps to tile 0 one scanline down, then
  // reaches tile 4 only for the final output scanline.
  Require(pTexture->GeneratePairRightTile(3, PairRight));
  for (int y = 0; y < TILE_HEIGHT - 1; ++y) {
    Require(PairRight.data[0][y].r
            == pTexture->m_pTileAy[0].data[0][y + 1].r);
    Require(PairRight.data[TILE_WIDTH - 1][y].r
            == pTexture->m_pTileAy[0].data[TILE_WIDTH - 1][y + 1].r);
  }
  Require(PairRight.data[0][TILE_HEIGHT - 1].r
          == pTexture->m_pTileAy[4].data[0][0].r);
  Require(PairRight.data[TILE_WIDTH - 1][TILE_HEIGHT - 1].r
          == pTexture->m_pTileAy[4].data[TILE_WIDTH - 1][0].r);

  // The renderer builds no pair for the final content tile, and invalid
  // requests leave the caller's output untouched.
  const tTextureColor Sentinel{1, 2, 3, 4};
  PairRight.data[0][0] = Sentinel;
  Require(!pTexture->GeneratePairRightTile(-1, PairRight));
  Require(PairRight.data[0][0].r == Sentinel.r);
  Require(!pTexture->GeneratePairRightTile(iContentTiles - 1, PairRight));
  Require(PairRight.data[0][0].r == Sentinel.r);
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
  // E4-S4: the exported PNG is the canonical atlas, not the legacy column.
  // One content tile occupies one row of a four-wide atlas; the two synthetic
  // palette/transparency tiles the editor appends are not exported.
  Require(ReadPngDimension(mainPng, 16) == EXPORT_ATLAS_WIDTH);
  Require(ReadPngDimension(mainPng, 20) == TILE_HEIGHT);
  Require(ReadPngDimension(signPng, 16) == EXPORT_ATLAS_WIDTH);
  Require(ReadPngDimension(signPng, 20) == TILE_HEIGHT);

  CTexture *pLastGoodMain = assets.GetMainTexture();
  Require(!assets.LoadFromDocument(directory.Path().string(), "MISSING.DRH", "SIGNS.DRH"));
  Require(assets.GetMainTexture() == pLastGoodMain);
  Require(assets.IsLoaded());

  Require(assets.LoadFromDocument(directory.Path().string(), "MAIN.DRH", "SIGNS.DRH"));
  assets.Clear();
  Require(!assets.IsLoaded());
  Require(!assets.ExportTextures(mainPng.string(), signPng.string()));

  TestCanonicalExportAtlas();
  TestRendererPairRightTile();
  return 0;
}
