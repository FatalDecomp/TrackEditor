#include "TextureMapping.h"
#include "Texture.h"
#include "Vertex.h"
//-------------------------------------------------------------------------------------------------
namespace
{
void ApplyTexCoords(glm::vec2 &topLeft,
                    glm::vec2 &topRight,
                    glm::vec2 &bottomLeft,
                    glm::vec2 &bottomRight,
                    std::uint32_t uiTexIndex,
                    std::uint32_t uiTexIncVal,
                    bool bFlipHoriz,
                    bool bFlipVert,
                    int iAtlasTileCount)
{
  const float fTileCount = static_cast<float>(iAtlasTileCount);
  if (!bFlipHoriz && !bFlipVert) {
    topLeft = glm::vec2(1.0f, static_cast<float>(uiTexIndex) / fTileCount);
    topRight = glm::vec2(1.0f, static_cast<float>(uiTexIndex + uiTexIncVal) / fTileCount);
    bottomLeft = glm::vec2(0.0f, static_cast<float>(uiTexIndex) / fTileCount);
    bottomRight = glm::vec2(0.0f, static_cast<float>(uiTexIndex + uiTexIncVal) / fTileCount);
  } else if (bFlipHoriz && !bFlipVert) {
    topLeft = glm::vec2(1.0f, static_cast<float>(uiTexIndex + uiTexIncVal) / fTileCount);
    topRight = glm::vec2(1.0f, static_cast<float>(uiTexIndex) / fTileCount);
    bottomLeft = glm::vec2(0.0f, static_cast<float>(uiTexIndex + uiTexIncVal) / fTileCount);
    bottomRight = glm::vec2(0.0f, static_cast<float>(uiTexIndex) / fTileCount);
  } else if (!bFlipHoriz && bFlipVert) {
    topLeft = glm::vec2(0.0f, static_cast<float>(uiTexIndex) / fTileCount);
    topRight = glm::vec2(0.0f, static_cast<float>(uiTexIndex + uiTexIncVal) / fTileCount);
    bottomLeft = glm::vec2(1.0f, static_cast<float>(uiTexIndex) / fTileCount);
    bottomRight = glm::vec2(1.0f, static_cast<float>(uiTexIndex + uiTexIncVal) / fTileCount);
  } else {
    topLeft = glm::vec2(0.0f, static_cast<float>(uiTexIndex + uiTexIncVal) / fTileCount);
    topRight = glm::vec2(0.0f, static_cast<float>(uiTexIndex) / fTileCount);
    bottomLeft = glm::vec2(1.0f, static_cast<float>(uiTexIndex + uiTexIncVal) / fTileCount);
    bottomRight = glm::vec2(1.0f, static_cast<float>(uiTexIndex) / fTileCount);
  }
}

void ApplyColor(glm::vec2 &topLeft,
                glm::vec2 &topRight,
                glm::vec2 &bottomLeft,
                glm::vec2 &bottomRight,
                std::uint32_t uiColor,
                int iAtlasTileCount,
                int iAtlasIndex)
{
  const int iPaletteX = static_cast<int>(uiColor / 16);
  const int iPaletteY = static_cast<int>(uiColor % 16);
  const float fAtlasHeight = static_cast<float>(iAtlasTileCount * TILE_HEIGHT);

  topLeft = glm::vec2(1.0f - static_cast<float>(iPaletteX * 4 + 1) / TILE_WIDTH,
                      static_cast<float>(iAtlasIndex * TILE_HEIGHT + iPaletteY * 4 + 1) / fAtlasHeight);
  topRight = glm::vec2(1.0f - static_cast<float>(iPaletteX * 4 + 1) / TILE_WIDTH,
                       static_cast<float>(iAtlasIndex * TILE_HEIGHT + iPaletteY * 4 + 3) / fAtlasHeight);
  bottomLeft = glm::vec2(1.0f - static_cast<float>(iPaletteX * 4 + 3) / TILE_WIDTH,
                         static_cast<float>(iAtlasIndex * TILE_HEIGHT + iPaletteY * 4 + 1) / fAtlasHeight);
  bottomRight = glm::vec2(1.0f - static_cast<float>(iPaletteX * 4 + 3) / TILE_WIDTH,
                          static_cast<float>(iAtlasIndex * TILE_HEIGHT + iPaletteY * 4 + 3) / fAtlasHeight);
}
}

//-------------------------------------------------------------------------------------------------

void TextureMapping::GetTextureCoordinates(const CTexture &Texture,
                                           std::uint32_t uiSurfaceType,
                                           tVertex &topLeft,
                                           tVertex &topRight,
                                           tVertex &bottomLeft,
                                           tVertex &bottomRight)
{
  const int iAtlasTileCount = Texture.GetAtlasTileCount();
  if (iAtlasTileCount <= 0)
    return;

  const bool bPair = (uiSurfaceType & SURFACE_FLAG_TEXTURE_PAIR) != 0;
  const bool bFlipVert = (uiSurfaceType & SURFACE_FLAG_FLIP_VERT) != 0;
  const bool bFlipHoriz = (uiSurfaceType & SURFACE_FLAG_FLIP_HORIZ) != 0;
  const bool bTransparent = (uiSurfaceType & SURFACE_FLAG_TRANSPARENT) != 0;
  const bool bApplyTexture = (uiSurfaceType & SURFACE_FLAG_APPLY_TEXTURE) != 0;
  const std::uint32_t uiTexIndex = uiSurfaceType & SURFACE_MASK_TEXTURE_INDEX;

  if (bApplyTexture) {
    ApplyTexCoords(topLeft.texCoords, topRight.texCoords,
                   bottomLeft.texCoords, bottomRight.texCoords,
                   uiTexIndex, bPair ? 2u : 1u,
                   bFlipHoriz, bFlipVert, iAtlasTileCount);
  } else {
    const int iAtlasIndex = bTransparent
        ? iAtlasTileCount - 1
        : iAtlasTileCount - 2;
    ApplyColor(topLeft.texCoords, topRight.texCoords,
               bottomLeft.texCoords, bottomRight.texCoords,
               uiTexIndex, iAtlasTileCount, iAtlasIndex);
  }
}

//-------------------------------------------------------------------------------------------------

glm::vec2 TextureMapping::GetColorCenterCoordinates(const CTexture &Texture,
                                                    std::uint32_t uiColor)
{
  glm::vec2 topLeft(0.0f);
  glm::vec2 topRight(0.0f);
  glm::vec2 bottomLeft(0.0f);
  glm::vec2 bottomRight(0.0f);
  const int iAtlasTileCount = Texture.GetAtlasTileCount();
  if (iAtlasTileCount <= 0)
    return topLeft;

  ApplyColor(topLeft, topRight, bottomLeft, bottomRight,
             uiColor, iAtlasTileCount, iAtlasTileCount - 2);
  return topLeft;
}
