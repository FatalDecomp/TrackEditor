#include "Texture.h"
#include "Palette.h"
#include <fstream>
#include <sstream>
#include "Unmangler.h"
#include <GL/glew.h>
#include "OpenGLDebug.h"
#include "Vertex.h"
#include "ShapeFactory.h"
#include <fstream>
#include "Logging.h"
#if defined(IS_WINDOWS)
#include <Windows.h>
#endif
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
  : m_uiId(0)
  , m_iNumTiles(0)
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
  GLCALL(glDeleteTextures(1, &m_uiId));

  m_uiId = 0;
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
  if (m_uiId == 0) {
    GLCALL(glGenTextures(1, &m_uiId));
    GLCALL(glBindTexture(GL_TEXTURE_2D, m_uiId));
    GLCALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
    GLCALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
    GLCALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    GLCALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
  }

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
  char *szBuf = new char[length];
  memset(szBuf, 0, length);
  file.read(szBuf, length);

  bool bSuccess = false;
  //unmangle
  int iUnmangledLength = Unmangler::GetUnmangledLength((uint8 *)szBuf, (int)length);
  if (iUnmangledLength > 0 && iUnmangledLength < MAX_MANGLED_LENGTH) {
    Logging::LogMessage("Texture file %s is mangled", sFilename.c_str());
    uint8 *szUnmangledData = new uint8[iUnmangledLength];
    bSuccess = Unmangler::UnmangleFile((uint8 *)szBuf, (int)length, szUnmangledData, iUnmangledLength);
    Logging::LogMessage("%s texture file %s", bSuccess ? "Unmangled" : "Failed to unmangle", sFilename.c_str());

    if (bSuccess)
      bSuccess = ProcessTextureData(szUnmangledData, (size_t)iUnmangledLength);

    delete[] szUnmangledData;
  } else {
    bSuccess = ProcessTextureData((uint8 *)szBuf, length);
  }

  delete[] szBuf;
  file.close();

  Logging::LogMessage("%s texture: %s", bSuccess ? "Loaded" : "Failed to load", sFilename.c_str());

  return bSuccess;
}

//-------------------------------------------------------------------------------------------------

void CTexture::Bind(uint32 uiSlot) const
{
  GLCALL(glActiveTexture(GL_TEXTURE0 + uiSlot));
  GLCALL(glBindTexture(GL_TEXTURE_2D, m_uiId));
}

//-------------------------------------------------------------------------------------------------

void CTexture::Unbind() const
{
  GLCALL(glBindTexture(GL_TEXTURE_2D, 0));
}

//-------------------------------------------------------------------------------------------------

void CTexture::GetTextureCoordinates(uint32 uiSurfaceType,
                                     tVertex &topLeft, tVertex &topRight, tVertex &bottomLeft, tVertex &bottomRight)
{
  if (!m_pPalette) {
    return;
  }

  bool bPair = uiSurfaceType & SURFACE_FLAG_TEXTURE_PAIR;// &&uiSurfaceType &SURFACE_FLAG_PAIR_NEXT_TEX;
  bool bFlipVert = uiSurfaceType & SURFACE_FLAG_FLIP_VERT;
  bool bFlipHoriz = uiSurfaceType & SURFACE_FLAG_FLIP_HORIZ;
  bool bTransparent = uiSurfaceType & SURFACE_FLAG_TRANSPARENT;
  bool bPartialTrans = uiSurfaceType & SURFACE_FLAG_PARTIAL_TRANS;
  bool bApplyTexture = uiSurfaceType & SURFACE_FLAG_APPLY_TEXTURE;
  uint32 uiTexIndex = uiSurfaceType & SURFACE_MASK_TEXTURE_INDEX;
  uint32 uiTexIncVal = bPair ? 2 : 1;

  if (bApplyTexture) {
    ApplyTexCoords(topLeft.texCoords,
                   topRight.texCoords,
                   bottomLeft.texCoords,
                   bottomRight.texCoords,
                   uiTexIndex, uiTexIncVal, bFlipHoriz, bFlipVert);
  } else if (bTransparent) {
    ApplyTransparency(topLeft.texCoords,
                      topRight.texCoords,
                      bottomLeft.texCoords,
                      bottomRight.texCoords,
                      uiTexIndex);
  } else {
    ApplyColor(topLeft.texCoords,
               topRight.texCoords,
               bottomLeft.texCoords,
               bottomRight.texCoords,
               uiTexIndex);
  }
}

//-------------------------------------------------------------------------------------------------

glm::vec2 CTexture::GetColorCenterCoordinates(uint32 uiColor)
{
  int iPaletteIndex = m_iNumTiles - NUM_PALETTE_TILES - NUM_TRANSPARENT_TILES;

  int iPaletteX = uiColor / 16;
  int iPaletteY = uiColor % 16;

  return glm::vec2(1.0f - (float)(iPaletteX * 4 + 1) / TILE_WIDTH, ((float)iPaletteIndex * TILE_HEIGHT + iPaletteY * 4 + 1) / (float)(m_iNumTiles * TILE_HEIGHT));
}

//-------------------------------------------------------------------------------------------------

uint8 *CTexture::GenerateBitmapData(int &iSize)
{
  iSize = (4 * TILE_WIDTH * TILE_HEIGHT * m_iNumTiles);

  uint8 *pData = new uint8[iSize];

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

bool CTexture::ExportToPngFile(const std::string &sFilename)
{
  int iBmpSize;
  uint8 *pBmpData = GenerateBitmapData(iBmpSize);
  stbi_write_png(sFilename.c_str(), TILE_WIDTH, TILE_HEIGHT * m_iNumTiles, 4, pBmpData, TILE_WIDTH * 4);
  delete[] pBmpData;
  return true;
}

//-------------------------------------------------------------------------------------------------

int CTexture::GetNumTiles()
{
  return m_iNumTiles - NUM_PALETTE_TILES - NUM_TRANSPARENT_TILES;
}

//-------------------------------------------------------------------------------------------------

glm::vec4 CTexture::RandomColor()
{
  glm::vec4 ret;
  ret.r = rand() / (float)RAND_MAX;
  ret.b = rand() / (float)RAND_MAX;
  ret.g = rand() / (float)RAND_MAX;
  ret.a = 1.0f;
  return ret;
}

//-------------------------------------------------------------------------------------------------

glm::vec4 CTexture::ColorBytesToFloat(const glm::vec3 &color)
{
  glm::vec4 ret;
  ret.r = (float)color.r / 255.0f;
  ret.g = (float)color.g / 255.0f;
  ret.b = (float)color.b / 255.0f;
  ret.a = 1.0f;
  return ret;
}

//-------------------------------------------------------------------------------------------------

bool CTexture::ProcessTextureData(const uint8 *pData, size_t length)
{
  if (!m_pPalette) {
    assert(0);
    return false;
  }

  int iPixelsPerTile = TILE_WIDTH * TILE_HEIGHT;
  int iNumTexTiles = (int)length / iPixelsPerTile;
  m_iNumTiles = iNumTexTiles + NUM_PALETTE_TILES + NUM_TRANSPARENT_TILES;
  m_pTileAy = new tTile[m_iNumTiles];
  for (int i = 0; i < iNumTexTiles; ++i) {
    tTile *pTile = &m_pTileAy[i];
    for (int j = 0; j < iPixelsPerTile; ++j) {
      uint8 byPaletteIndex = pData[i * iPixelsPerTile + j];
      if (PALETTE_SIZE > byPaletteIndex) {
        pTile->data[j % TILE_WIDTH][j / TILE_WIDTH] = glm::vec<4, uint8>(m_pPalette->m_paletteAy[byPaletteIndex].r,
                                                                         m_pPalette->m_paletteAy[byPaletteIndex].g,
                                                                         m_pPalette->m_paletteAy[byPaletteIndex].b,
                                                                         byPaletteIndex ? 255 : 0);
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
            glm::vec<4, uint8>(m_pPalette->m_paletteAy[iPaletteIndex].r,
                               m_pPalette->m_paletteAy[iPaletteIndex].g,
                               m_pPalette->m_paletteAy[iPaletteIndex].b,
                               255);
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

  tTile *pTilesFlipped = new tTile[m_iNumTiles];
  FlipTileLines(m_pTileAy, pTilesFlipped, m_iNumTiles);
  int iLength = TILE_WIDTH;
  int iHeight = TILE_HEIGHT * m_iNumTiles;
  GLCALL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
                      iLength, iHeight, 0,
                      GL_RGBA, GL_UNSIGNED_BYTE, pTilesFlipped));
  GLCALL(glBindTexture(GL_TEXTURE_2D, 0));
  delete[] pTilesFlipped;
  return true;
}

//-------------------------------------------------------------------------------------------------

void CTexture::FlipTileLines(tTile *pSource, tTile *pDest, int iNumTiles)
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

void CTexture::ApplyTexCoords(glm::vec2 &topLeft,
                              glm::vec2 &topRight,
                              glm::vec2 &bottomLeft,
                              glm::vec2 &bottomRight,
                              uint32 uiTexIndex, uint32 uiTexIncVal,
                              bool bFlipHoriz, bool bFlipVert)
{
  if (!bFlipHoriz && !bFlipVert) {
    topLeft = glm::vec2(1.0f, (float)uiTexIndex / (float)m_iNumTiles);
    topRight = glm::vec2(1.0f, (float)(uiTexIndex + uiTexIncVal) / (float)m_iNumTiles);
    bottomLeft = glm::vec2(0.0f, (float)uiTexIndex / (float)m_iNumTiles);
    bottomRight = glm::vec2(0.0f, (float)(uiTexIndex + uiTexIncVal) / (float)m_iNumTiles);
  } else if (bFlipHoriz && !bFlipVert) {
    topLeft = glm::vec2(1.0f, (float)(uiTexIndex + uiTexIncVal) / (float)m_iNumTiles);
    topRight = glm::vec2(1.0f, (float)uiTexIndex / (float)m_iNumTiles);
    bottomLeft = glm::vec2(0.0f, (float)(uiTexIndex + uiTexIncVal) / (float)m_iNumTiles);
    bottomRight = glm::vec2(0.0f, (float)uiTexIndex / (float)m_iNumTiles);
  } else if (!bFlipHoriz && bFlipVert) {
    topLeft = glm::vec2(0.0f, (float)uiTexIndex / (float)m_iNumTiles);
    topRight = glm::vec2(0.0f, (float)(uiTexIndex + uiTexIncVal) / (float)m_iNumTiles);
    bottomLeft = glm::vec2(1.0f, (float)uiTexIndex / (float)m_iNumTiles);
    bottomRight = glm::vec2(1.0f, (float)(uiTexIndex + uiTexIncVal) / (float)m_iNumTiles);
  } else if (bFlipHoriz && bFlipVert) {
    topLeft = glm::vec2(0.0f, (float)(uiTexIndex + uiTexIncVal) / (float)m_iNumTiles);
    topRight = glm::vec2(0.0f, (float)uiTexIndex / (float)m_iNumTiles);
    bottomLeft = glm::vec2(1.0f, (float)(uiTexIndex + uiTexIncVal) / (float)m_iNumTiles);
    bottomRight = glm::vec2(1.0f, (float)uiTexIndex / (float)m_iNumTiles);
  }
}

//-------------------------------------------------------------------------------------------------

void CTexture::ApplyColor(glm::vec2 &topLeft,
                          glm::vec2 &topRight,
                          glm::vec2 &bottomLeft,
                          glm::vec2 &bottomRight,
                          uint32 uiTexIndex)
{
  int iPaletteIndex = m_iNumTiles - NUM_PALETTE_TILES - NUM_TRANSPARENT_TILES;

  int iPaletteX = uiTexIndex / 16;
  int iPaletteY = uiTexIndex % 16;

  topLeft     = glm::vec2(1.0f - (float)(iPaletteX * 4 + 1) / TILE_WIDTH, ((float)iPaletteIndex * TILE_HEIGHT + iPaletteY * 4 + 1) / (float)(m_iNumTiles * TILE_HEIGHT));
  topRight    = glm::vec2(1.0f - (float)(iPaletteX * 4 + 1) / TILE_WIDTH, ((float)iPaletteIndex * TILE_HEIGHT + iPaletteY * 4 + 3) / (float)(m_iNumTiles * TILE_HEIGHT));
  bottomLeft  = glm::vec2(1.0f - (float)(iPaletteX * 4 + 3) / TILE_WIDTH, ((float)iPaletteIndex * TILE_HEIGHT + iPaletteY * 4 + 1) / (float)(m_iNumTiles * TILE_HEIGHT));
  bottomRight = glm::vec2(1.0f - (float)(iPaletteX * 4 + 3) / TILE_WIDTH, ((float)iPaletteIndex * TILE_HEIGHT + iPaletteY * 4 + 3) / (float)(m_iNumTiles * TILE_HEIGHT));
}

//-------------------------------------------------------------------------------------------------

void CTexture::ApplyTransparency(glm::vec2 &topLeft,
                                 glm::vec2 &topRight,
                                 glm::vec2 &bottomLeft,
                                 glm::vec2 &bottomRight,
                                 uint32 uiTexIndex)
{
  int iTranspIndex = m_iNumTiles - NUM_TRANSPARENT_TILES;

  int iTranspX = uiTexIndex / 16;
  int iTranspY = uiTexIndex % 16;

  topLeft     = glm::vec2(1.0f - (float)(iTranspX * 4 + 1) / TILE_WIDTH, ((float)iTranspIndex * TILE_HEIGHT + iTranspY * 4 + 1) / (float)(m_iNumTiles * TILE_HEIGHT));
  topRight    = glm::vec2(1.0f - (float)(iTranspX * 4 + 1) / TILE_WIDTH, ((float)iTranspIndex * TILE_HEIGHT + iTranspY * 4 + 3) / (float)(m_iNumTiles * TILE_HEIGHT));
  bottomLeft  = glm::vec2(1.0f - (float)(iTranspX * 4 + 3) / TILE_WIDTH, ((float)iTranspIndex * TILE_HEIGHT + iTranspY * 4 + 1) / (float)(m_iNumTiles * TILE_HEIGHT));
  bottomRight = glm::vec2(1.0f - (float)(iTranspX * 4 + 3) / TILE_WIDTH, ((float)iTranspIndex * TILE_HEIGHT + iTranspY * 4 + 3) / (float)(m_iNumTiles * TILE_HEIGHT));
}

//-------------------------------------------------------------------------------------------------

glm::vec<4, uint8> CTexture::GetTranspColor(int iTranspIndex)
{
  glm::vec<4, uint8> color(0, 0, 0, 0);
  switch (iTranspIndex) {
    case 0:
      color = glm::vec<4, uint8>(0, 0, 0, 255);
      break;
    case 1:
      color = glm::vec<4, uint8>(0, 0, 0, 64);
      break;
    case 2:
      color = glm::vec<4, uint8>(0, 0, 0, 128);
      break;
    case 3:
      color = glm::vec<4, uint8>(0, 0, 0, 192);
      break;
    case 4:
      color = glm::vec<4, uint8>(0, 0, 255, 64);
      break;
  }
  return color;
}

//-------------------------------------------------------------------------------------------------
