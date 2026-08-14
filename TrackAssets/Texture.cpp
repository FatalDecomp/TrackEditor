#include "Texture.h"
#include "Palette.h"
#include <cstdint>
#include <fstream>
#include <cstring>
#include "Unmangler.h"
#include <vector>
#include "Logging.h"
//-------------------------------------------------------------------------------------------------
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
//-------------------------------------------------------------------------------------------------
#if defined(_DEBUG) && defined(IS_WINDOWS)
#define new new(_CLIENT_BLOCK, __FILE__, __LINE__)
#endif
//-------------------------------------------------------------------------------------------------

#define NUM_PALETTE_TILES 1
#define NUM_TRANSPARENT_TILES 1

//-------------------------------------------------------------------------------------------------

CTexture::CTexture()
  : m_pTileAy(NULL)
  , m_pPalette(NULL)
  , m_iNumTiles(0)
{
}

//-------------------------------------------------------------------------------------------------

CTexture::~CTexture()
{
  ClearData();
}

//-------------------------------------------------------------------------------------------------

void CTexture::ClearData()
{
  m_iNumTiles = 0;
  if (m_pTileAy) {
    delete[] m_pTileAy;
    m_pTileAy = NULL;
  }
  m_pPalette = NULL;
}

//-------------------------------------------------------------------------------------------------

bool CTexture::LoadTexture(const std::string &sFilename, CPalette *pPalette)
{
  ClearData();
  m_pPalette = pPalette;

  if (sFilename.empty()) {
    Logging::LogMessage("Texture filename is empty");
    return false;
  }

  //open file
  std::ifstream file(sFilename.c_str(), std::ios::binary);
  if (!file.is_open()) {
    Logging::LogMessage("Failed to open texture: %s", sFilename.c_str());
    return false;
  }

  file.seekg(0, file.end);
  size_t length = file.tellg();
  file.seekg(0, file.beg);
  if (length <= 0) {
    Logging::LogMessage("Texture file %s is empty", sFilename.c_str());
    return false;
  }

  //read file
  std::vector<std::uint8_t> data(length);
  file.read(reinterpret_cast<char *>(data.data()), static_cast<std::streamsize>(length));
  if (!file)
    return false;

  bool bSuccess = false;
  //unmangle
  int iUnmangledLength = Unmangler::GetUnmangledLength(data.data(), (int)length);
  const int iPixelsPerTile = TILE_WIDTH * TILE_HEIGHT;
  if (iUnmangledLength > 0
      && iUnmangledLength < MAX_MANGLED_LENGTH
      && iUnmangledLength % iPixelsPerTile == 0) {
    Logging::LogMessage("Texture file %s is mangled", sFilename.c_str());
    std::vector<std::uint8_t> unmangledData(static_cast<size_t>(iUnmangledLength));
    bSuccess = Unmangler::UnmangleFile(data.data(), (int)length,
                                       unmangledData.data(), iUnmangledLength);
    Logging::LogMessage("%s texture file %s", bSuccess ? "Unmangled" : "Failed to unmangle", sFilename.c_str());

    if (bSuccess)
      bSuccess = ProcessTextureData(unmangledData.data(), unmangledData.size());
  } else {
    bSuccess = ProcessTextureData(data.data(), length);
  }

  file.close();

  Logging::LogMessage("%s texture: %s", bSuccess ? "Loaded" : "Failed to load", sFilename.c_str());

  return bSuccess;
}

//-------------------------------------------------------------------------------------------------

std::uint8_t *CTexture::GenerateBitmapData(int &iSize) const
{
  iSize = (4 * TILE_WIDTH * TILE_HEIGHT * m_iNumTiles);

  std::uint8_t *pData = new std::uint8_t[iSize];

  tTile *pTilesFlipped = new tTile[m_iNumTiles];
  FlipTileLines(m_pTileAy, pTilesFlipped, m_iNumTiles);

  int iOffset = 0;// fileHeader.bfOffBits;
  for (int i = 0; i < m_iNumTiles; ++i) {
    for (int x = 0; x < TILE_WIDTH; ++x) {
      for (int y = 0; y < TILE_HEIGHT; ++y) {
        pData[iOffset++] = pTilesFlipped[i].data[x][y].r;
        pData[iOffset++] = pTilesFlipped[i].data[x][y].g;
        pData[iOffset++] = pTilesFlipped[i].data[x][y].b;
        pData[iOffset++] = pTilesFlipped[i].data[x][y].a;
      }
    }
  }
  delete[] pTilesFlipped;

  return pData;
}

//-------------------------------------------------------------------------------------------------

int CTexture::GetExportTilesPerRow()
{
  return EXPORT_ATLAS_WIDTH / TILE_WIDTH;
}

//-------------------------------------------------------------------------------------------------

int CTexture::GetExportAtlasHeight() const
{
  const int iTiles = GetNumTiles();
  const int iTilesPerRow = GetExportTilesPerRow();
  // ROLLER gives an empty bank one row rather than a zero-height atlas.
  const int iRows = iTiles > 0
      ? (iTiles + iTilesPerRow - 1) / iTilesPerRow
      : 1;
  return iRows * TILE_HEIGHT;
}

//-------------------------------------------------------------------------------------------------

std::uint8_t *CTexture::GenerateExportAtlas(int &iWidth, int &iHeight,
                                            int &iSize) const
{
  iWidth = EXPORT_ATLAS_WIDTH;
  iHeight = GetExportAtlasHeight();
  iSize = 4 * iWidth * iHeight;

  std::uint8_t *pData = new std::uint8_t[iSize];
  // A tile count that does not fill its last row leaves padding, and the
  // canonical UVs never address it; transparent black is the honest value.
  std::memset(pData, 0, static_cast<size_t>(iSize));

  // Content tiles only. m_iNumTiles also counts the synthetic palette and
  // transparency tiles the editor appends for its own pickers; ROLLER's
  // uiTileCount does not, and the atlas has to agree with ROLLER or every UV
  // lands a row out.
  const int iTiles = GetNumTiles();
  const int iTilesPerRow = GetExportTilesPerRow();
  for (int i = 0; i < iTiles; ++i) {
    const int iTileX = (i % iTilesPerRow) * TILE_WIDTH;
    const int iTileY = (i / iTilesPerRow) * TILE_HEIGHT;
    for (int y = 0; y < TILE_HEIGHT; ++y) {
      for (int x = 0; x < TILE_WIDTH; ++x) {
        // data is indexed [column][row]; writing it straight into a row-major
        // image is what keeps the tile upright. The legacy column bitmap walks
        // x outermost, which transposes each tile, and flips the rows on top
        // of that - both were invisible while it computed its own UVs to
        // match, and both are wrong for a canonical UV.
        const tTextureColor &Colour = m_pTileAy[i].data[x][y];
        std::uint8_t *pPixel =
            pData + (static_cast<size_t>(iTileY + y) * iWidth
                     + static_cast<size_t>(iTileX + x)) * 4u;
        pPixel[0] = Colour.r;
        pPixel[1] = Colour.g;
        pPixel[2] = Colour.b;
        pPixel[3] = Colour.a;
      }
    }
  }
  return pData;
}

//-------------------------------------------------------------------------------------------------

bool CTexture::GeneratePairRightTile(int iTileIndex, tTile &TileOut) const
{
  const int iTiles = GetNumTiles();
  if (!m_pTileAy || iTileIndex < 0 || iTileIndex >= iTiles - 1)
    return false;

  const int iTilesPerRow = GetExportTilesPerRow();
  if (iTileIndex % iTilesPerRow != iTilesPerRow - 1) {
    TileOut = m_pTileAy[iTileIndex + 1];
    return true;
  }

  // ROLLER's wall sampler starts from tile N's pointer and reads a 128-pixel
  // row through a 256-pixel-wide atlas. At the last column, the right half of
  // output row Y therefore begins at column zero of atlas row Y + 1. For the
  // first 63 scanlines that is the first tile in N's own tile row, shifted by
  // one scanline; the final scanline crosses into the following tile row.
  const int iFirstTileInRow = iTileIndex - (iTileIndex % iTilesPerRow);
  for (int y = 0; y < TILE_HEIGHT - 1; ++y) {
    for (int x = 0; x < TILE_WIDTH; ++x)
      TileOut.data[x][y] = m_pTileAy[iFirstTileInRow].data[x][y + 1];
  }
  for (int x = 0; x < TILE_WIDTH; ++x)
    TileOut.data[x][TILE_HEIGHT - 1] = m_pTileAy[iTileIndex + 1].data[x][0];

  return true;
}

//-------------------------------------------------------------------------------------------------

bool CTexture::ExportToPngFile(const std::string &sFilename) const
{
  if (!IsLoaded() || sFilename.empty())
    return false;

  int iWidth = 0;
  int iHeight = 0;
  int iSize = 0;
  std::uint8_t *pAtlas = GenerateExportAtlas(iWidth, iHeight, iSize);
  const bool bSuccess = stbi_write_png(sFilename.c_str(), iWidth, iHeight, 4,
                                       pAtlas, iWidth * 4) != 0;
  delete[] pAtlas;
  return bSuccess;
}

//-------------------------------------------------------------------------------------------------

int CTexture::GetNumTiles() const
{
  return m_iNumTiles - NUM_PALETTE_TILES - NUM_TRANSPARENT_TILES;
}

//-------------------------------------------------------------------------------------------------

bool CTexture::ProcessTextureData(const std::uint8_t *pData, size_t length)
{
  if (!m_pPalette) {
    assert(0);
    return false;
  }

  const int iPixelsPerTile = TILE_WIDTH * TILE_HEIGHT;
  if (length == 0 || length % static_cast<size_t>(iPixelsPerTile) != 0) {
    Logging::LogMessage("Error loading texture: invalid texture byte count");
    return false;
  }
  int iNumTexTiles = (int)length / iPixelsPerTile;
  m_iNumTiles = iNumTexTiles + NUM_PALETTE_TILES + NUM_TRANSPARENT_TILES;
  m_pTileAy = new tTile[m_iNumTiles];
  for (int i = 0; i < iNumTexTiles; ++i) {
    tTile *pTile = &m_pTileAy[i];
    for (int j = 0; j < iPixelsPerTile; ++j) {
      std::uint8_t byPaletteIndex = pData[i * iPixelsPerTile + j];
      // The palette covers the whole byte range, so the out-of-bounds branch
      // that used to sit here was unreachable and read as always-true to the
      // compiler. The assertion is now the static one.
      static_assert(PALETTE_SIZE == 256,
                    "a palette index is a byte, so PALETTE_SIZE must cover 0..255");
      pTile->data[j % TILE_WIDTH][j / TILE_WIDTH] =
          tTextureColor{m_pPalette->m_paletteAy[byPaletteIndex].r,
                        m_pPalette->m_paletteAy[byPaletteIndex].g,
                        m_pPalette->m_paletteAy[byPaletteIndex].b,
                        static_cast<std::uint8_t>(byPaletteIndex ? 255 : 0)};
    }
  }

  //generate palette tile
  tTile *pPaletteTile = &m_pTileAy[iNumTexTiles];
  int iRunner = 0;
  for (int i = 0; i < 16; ++i) {
    for (int j = 0; j < 16; ++j) {
      for (int k = 0; k < 16; ++k) {
        int iPixelNum = iRunner++;
        int iTileX = iPixelNum % TILE_WIDTH;
        int iTileY = iPixelNum / TILE_WIDTH;
        int iPaletteIndex = iTileX / 4 + i * 16;
        if (PALETTE_SIZE > iPaletteIndex) {
          pPaletteTile->data[iTileX][iTileY] =
              tTextureColor{m_pPalette->m_paletteAy[iPaletteIndex].r,
                            m_pPalette->m_paletteAy[iPaletteIndex].g,
                            m_pPalette->m_paletteAy[iPaletteIndex].b,
                            255};
        }
      }
    }
  }

  //generate transparent color tile
  tTile *pTranspTile = &m_pTileAy[iNumTexTiles + NUM_PALETTE_TILES];
  iRunner = 0;
  for (int i = 0; i < 16; ++i) {
    for (int j = 0; j < 16; ++j) {
      for (int k = 0; k < 16; ++k) {
        int iPixelNum = iRunner++;
        int iTileX = iPixelNum % TILE_WIDTH;
        int iTileY = iPixelNum / TILE_WIDTH;
        int iTranspIndex = iTileX / 4 + i * 16;

        pTranspTile->data[iTileX][iTileY] = GetTranspColor(iTranspIndex);
      }
    }
  }

  return true;
}

//-------------------------------------------------------------------------------------------------

void CTexture::FlipTileLines(const tTile *pSource, tTile *pDest, int iNumTiles) const
{
  for (int i = 0; i < iNumTiles; ++i) {
    for (int x = 0; x < TILE_WIDTH; ++x) {
      for (int y = 0; y < TILE_HEIGHT; ++y) {
        pDest[i].data[x][y] = pSource[i].data[x][TILE_HEIGHT - (y + 1)];
      }
    }
  }
}

//-------------------------------------------------------------------------------------------------

tTextureColor CTexture::GetTranspColor(int iTranspIndex)
{
  tTextureColor color{0, 0, 0, 0};
  switch (iTranspIndex) {
    case 0:
      color = tTextureColor{0, 0, 0, 255};
      break;
    case 1:
      color = tTextureColor{0, 0, 0, 64};
      break;
    case 2:
      color = tTextureColor{0, 0, 0, 128};
      break;
    case 3:
      color = tTextureColor{0, 0, 0, 192};
      break;
    case 4:
      color = tTextureColor{0, 0, 255, 64};
      break;
  }
  return color;
}

//-------------------------------------------------------------------------------------------------
