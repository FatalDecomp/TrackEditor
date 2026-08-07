#ifndef _TRACKEDITOR_TEXTURE_H
#define _TRACKEDITOR_TEXTURE_H
//-------------------------------------------------------------------------------------------------
#include <cstddef>
#include <cstdint>
#include <string>
//-------------------------------------------------------------------------------------------------
#define TILE_WIDTH 64
#define TILE_HEIGHT TILE_WIDTH
// E4-S4. ROLLER addresses texture tiles in a 256-pixel-wide atlas -- polytex.c
// resolves a tile as row = index >> 2, col = index & 3 with a 256-byte stride
// - and the canonical UVs the exporters emit are expressed against exactly
// that atlas. The exported PNG therefore has to use it too, which is why this
// is a fixed 256 rather than a multiple of TILE_WIDTH chosen here.
#define EXPORT_ATLAS_WIDTH 256
//-------------------------------------------------------------------------------------------------
#define SURFACE_FLAG_WALL_31       0x80000000
#define SURFACE_FLAG_BOUNCE        0x40000000
#define SURFACE_FLAG_ECHO          0x20000000
#define SURFACE_FLAG_AI_MAX_SPEED  0x10000000
#define SURFACE_FLAG_NO_SPAWN      0x08000000
#define SURFACE_FLAG_PIT_BOX       0x04000000
#define SURFACE_FLAG_PIT           0x02000000
#define SURFACE_FLAG_PIT_ZONE      0x01000000
#define SURFACE_FLAG_AI_FAST_STRAT 0x00800000
#define SURFACE_FLAG_WALL_22       0x00400000
#define SURFACE_FLAG_TRANSPARENT   0x00200000
#define SURFACE_FLAG_FALL_OFF      0x00100000
#define SURFACE_FLAG_NON_MAGNETIC  0x00080000
#define SURFACE_FLAG_FLIP_VERT     0x00040000
#define SURFACE_FLAG_SKIP_RENDER   0x00020000
#define SURFACE_FLAG_TEXTURE_PAIR  0x00010000
#define SURFACE_FLAG_PREVENT_JUMP  0x00008000
#define SURFACE_FLAG_CONCAVE       0x00004000
#define SURFACE_FLAG_FLIP_BACKFACE 0x00002000
#define SURFACE_FLAG_FLIP_HORIZ    0x00001000
#define SURFACE_FLAG_BACK          0x00000800
#define SURFACE_FLAG_PARTIAL_TRANS 0x00000400
#define SURFACE_FLAG_NO_EXTRAS     0x00000200
#define SURFACE_FLAG_APPLY_TEXTURE 0x00000100
#define SURFACE_MASK_FLAGS         0xFFFFFF00
#define SURFACE_MASK_TEXTURE_INDEX 0x000000FF
//-------------------------------------------------------------------------------------------------
#define CAR_FLAG_ANMS_LIVERY       0x00008000
#define CAR_FLAG_ANMS_LOOKUP       0x00000200
//-------------------------------------------------------------------------------------------------
class CPalette;
//-------------------------------------------------------------------------------------------------
struct tTextureColor
{
  std::uint8_t r;
  std::uint8_t g;
  std::uint8_t b;
  std::uint8_t a;
};

struct tTile
{
  tTextureColor data[TILE_WIDTH][TILE_HEIGHT];
};
//-------------------------------------------------------------------------------------------------

class CTexture
{
public:
  CTexture();
  ~CTexture();

  void ClearData();
  bool LoadTexture(const std::string &sFilename, CPalette *pPalette);
  // The legacy single-column bitmap: TILE_WIDTH wide, TILE_HEIGHT per tile,
  // every tile including the synthetic palette and transparency ones. Both of
  // its consumers went with WhipLib, so nothing in the build calls this now -
  // only the track-assets unit test does. Kept as the legacy on-disk layout;
  // the exported PNG uses GenerateExportAtlas instead.
  std::uint8_t *GenerateBitmapData(int &iSize) const;

  // E4-S4. The canonical export atlas: EXPORT_ATLAS_WIDTH wide, four tiles per
  // row, row-major in tile-index order, content tiles only, top row first and
  // no transpose - the layout ROLLER's material transforms address. Tiles past
  // the content count pad the last row transparently.
  static int GetExportTilesPerRow();
  int GetExportAtlasHeight() const;
  std::uint8_t *GenerateExportAtlas(int &iWidth, int &iHeight,
                                    int &iSize) const;

  bool ExportToPngFile(const std::string &sFilename) const;
  int GetNumTiles() const;
  int GetAtlasTileCount() const { return m_iNumTiles; }
  bool IsLoaded() const { return m_pTileAy && m_iNumTiles > 2; }

  tTile *m_pTileAy;

private:
  bool ProcessTextureData(const std::uint8_t *pData, size_t length);
  void FlipTileLines(const tTile *pSource, tTile *pDest, int iNumTiles) const;
  tTextureColor GetTranspColor(int iTranspIndex);

  CPalette *m_pPalette; //not owned by this class
  int m_iNumTiles;
};

//-------------------------------------------------------------------------------------------------
#endif
