#ifndef _TRACKEDITOR_PALETTE_H
#define _TRACKEDITOR_PALETTE_H
//-------------------------------------------------------------------------------------------------
#include <cstdint>
#include <string>
//-------------------------------------------------------------------------------------------------
#define PALETTE_SIZE 256
//-------------------------------------------------------------------------------------------------
struct tPaletteColor
{
  std::uint8_t r;
  std::uint8_t g;
  std::uint8_t b;
};
//-------------------------------------------------------------------------------------------------

class CPalette
{
public:
  CPalette();
  ~CPalette();

  void ClearData();
  bool LoadPalette(const std::string &sFilename);
  bool IsLoaded() const { return m_bLoaded; };

  tPaletteColor m_paletteAy[PALETTE_SIZE];

private:
  bool m_bLoaded;
};

//-------------------------------------------------------------------------------------------------
#endif
