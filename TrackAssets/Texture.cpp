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
  : m_iNumTiles(0)
  , m_pTileAy(NULL)
  , m_pPalette(NULL)
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

bool CTexture::ExportToPngFile(const std::string &sFilename) const
{
  if (!IsLoaded() || sFilename.empty())
    return false;

  int iBmpSize;
  std::uint8_t *pBmpData = GenerateBitmapData(iBmpSize);
  const bool bSuccess = stbi_write_png(sFilename.c_str(), TILE_WIDTH,
                                      TILE_HEIGHT * m_iNumTiles, 4,
                                      pBmpData, TILE_WIDTH * 4) != 0;
  delete[] pBmpData;
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
      if (PALETTE_SIZE > byPaletteIndex) {
        pTile->data[j % TILE_WIDTH][j / TILE_WIDTH] =
            tTextureColor{m_pPalette->m_paletteAy[byPaletteIndex].r,
                          m_pPalette->m_paletteAy[byPaletteIndex].g,
                          m_pPalette->m_paletteAy[byPaletteIndex].b,
                          static_cast<std::uint8_t>(byPaletteIndex ? 255 : 0)};
      } else {
        assert(0);
        Logging::LogMessage("Error loading texture: palette index out of bounds");
        return false;
      }
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
