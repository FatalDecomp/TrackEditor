#ifndef _WHIPLIB_TEXTURE_H
#define _WHIPLIB_TEXTURE_H
//-------------------------------------------------------------------------------------------------
#include <vector>
#include <string>
#include "glm.hpp"
#include "Types.h"
//-------------------------------------------------------------------------------------------------
#define TILE_WIDTH 64
#define TILE_HEIGHT TILE_WIDTH
//-------------------------------------------------------------------------------------------------
#define SURFACE_FLAG_WALL_31       0x80000000
#define SURFACE_FLAG_BOUNCE_30     0x40000000
#define SURFACE_FLAG_ECHO          0x20000000
#define SURFACE_FLAG_28            0x10000000 //todo
#define SURFACE_FLAG_27            0x08000000 //todo
#define SURFACE_FLAG_26            0x04000000 //todo
#define SURFACE_FLAG_PIT           0x02000000
#define SURFACE_FLAG_YELLOW_MAP    0x01000000
#define SURFACE_FLAG_23            0x00800000 //todo
#define SURFACE_FLAG_WALL_22       0x00400000
#define SURFACE_FLAG_TRANSPARENT   0x00200000
#define SURFACE_FLAG_BOUNCE_20     0x00100000
#define SURFACE_FLAG_NON_MAGNETIC  0x00080000
#define SURFACE_FLAG_FLIP_VERT     0x00040000
#define SURFACE_FLAG_SKIP_RENDER   0x00020000
#define SURFACE_FLAG_TEXTURE_PAIR  0x00010000
#define SURFACE_FLAG_ANMS_LIVERY   0x00008000
#define SURFACE_FLAG_CONCAVE       0x00004000
#define SURFACE_FLAG_FLIP_BACKFACE 0x00002000
#define SURFACE_FLAG_FLIP_HORIZ    0x00001000
#define SURFACE_FLAG_BACK          0x00000800
#define SURFACE_FLAG_PARTIAL_TRANS 0x00000400
#define SURFACE_FLAG_ANMS_LOOKUP   0x00000200
#define SURFACE_FLAG_APPLY_TEXTURE 0x00000100
#define SURFACE_MASK_FLAGS         0xFFFFFF00
#define SURFACE_MASK_TEXTURE_INDEX 0x000000FF
//-------------------------------------------------------------------------------------------------
class CPalette;
struct tVertex;
//-------------------------------------------------------------------------------------------------
struct tTile
{
  glm::vec<4, uint8> data[TILE_WIDTH][TILE_HEIGHT];
};
//-------------------------------------------------------------------------------------------------

class CTexture
{
public:
  CTexture();
  ~CTexture();

  void ClearData();
  bool LoadTexture(const std::string &sFilename, CPalette *pPalette);
  void Bind(uint32 uiSlot = 0) const;
  void Unbind() const;
  void GetTextureCoordinates(uint32 uiSurfaceType,
                             tVertex &topLeft, tVertex &topRight, tVertex &bottomLeft, tVertex &bottomRight);
  glm::vec2 GetColorCenterCoordinates(uint32 uiColor);
  uint8 *GenerateBitmapData(int &iSize);
  bool ExportToPngFile(const std::string &sFilename);
  int GetNumTiles();

  tTile *m_pTileAy;

  static glm::vec4 RandomColor();
  static glm::vec4 ColorBytesToFloat(const glm::vec3 &color);

private:
  bool ProcessTextureData(const uint8 *pData, size_t length);
  void FlipTileLines(tTile *pSource, tTile *pDest, int iNumTiles);
  void ApplyTexCoords(glm::vec2 &topLeft,
                      glm::vec2 &topRight,
                      glm::vec2 &bottomLeft,
                      glm::vec2 &bottomRight,
                      uint32 uiTexIndex, uint32 uiTexIncVal,
                      bool bFlipHoriz, bool bFlipVert);
  void ApplyColor(glm::vec2 &topLeft,
                  glm::vec2 &topRight,
                  glm::vec2 &bottomLeft,
                  glm::vec2 &bottomRight,
                  uint32 uiTexIndex);
  void ApplyTransparency(glm::vec2 &topLeft,
                         glm::vec2 &topRight,
                         glm::vec2 &bottomLeft,
                         glm::vec2 &bottomRight,
                         uint32 uiTexIndex);
  glm::vec<4, uint8> GetTranspColor(int iTranspIndex);

  uint32 m_uiId;
  CPalette *m_pPalette; //not owned by this class
  int m_iNumTiles;
};

//-------------------------------------------------------------------------------------------------
#endif