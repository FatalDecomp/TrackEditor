#ifndef _TRACKEDITOR_PALETTE_H
#define _TRACKEDITOR_PALETTE_H
//-------------------------------------------------------------------------------------------------
#include <cstdint>
#include <string>
#include "glm.hpp"
//-------------------------------------------------------------------------------------------------
#define PALETTE_SIZE 256
//-------------------------------------------------------------------------------------------------

class CPalette
{
public:
  CPalette();
  ~CPalette();

  void ClearData();
  bool LoadPalette(const std::string &sFilename);
  bool IsLoaded() const { return m_bLoaded; };

  glm::vec<3, std::uint8_t> m_paletteAy[PALETTE_SIZE];

private:
  bool m_bLoaded;
};

//-------------------------------------------------------------------------------------------------
#endif
