#ifndef _WHIPLIB_TEXTURE_MAPPING_H
#define _WHIPLIB_TEXTURE_MAPPING_H
//-------------------------------------------------------------------------------------------------
#include "glm.hpp"
#include <cstdint>
//-------------------------------------------------------------------------------------------------
class CTexture;
struct tVertex;
//-------------------------------------------------------------------------------------------------
namespace TextureMapping
{
  void GetTextureCoordinates(const CTexture &Texture,
                             std::uint32_t uiSurfaceType,
                             tVertex &topLeft, tVertex &topRight,
                             tVertex &bottomLeft, tVertex &bottomRight);
  glm::vec2 GetColorCenterCoordinates(const CTexture &Texture,
                                      std::uint32_t uiColor);
}
//-------------------------------------------------------------------------------------------------
#endif
