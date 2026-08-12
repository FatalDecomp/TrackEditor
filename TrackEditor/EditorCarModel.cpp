#include "EditorCarModel.h"

#if defined(IS_WINDOWS)
#undef IS_WINDOWS
#endif
extern "C"
{
#include "carplans.h"
#include "editor_surface.h"
}

#include <cstring>

namespace
{
constexpr uint32_t kCarTextureSet = 1u;
constexpr uint32_t kMaterialCapacity = 256u;
constexpr uint32_t kIndicesPerQuad = 6u;

struct tCarDescription
{
  const char *szName;
  const char *szTexture;
};

// CarDesigns is ordered by the public CAR_DESIGN_* numbering. Designs 8..11
// intentionally reuse one of the first eight physical plans with a different
// livery, while F1WACK and DEATH use their own texture banks.
const tCarDescription g_aCars[] = {
  { "AUTO",     "xauto.bm" },
  { "DESILVA",  "xdesilva.bm" },
  { "PULSE",    "xpulse.bm" },
  { "GLOBAL",   "xglobal.bm" },
  { "MILLION",  "xmillion.bm" },
  { "MISSION",  "xmission.bm" },
  { "ZIZIN",    "xzizin.bm" },
  { "REISE",    "xreise.bm" },
  { "SUICYCO",  "xzizin.bm" },
  { "MAYTE",    "xauto.bm" },
  { "2X4B523P", "xpulse.bm" },
  { "TINKLE",   "xreise.bm" },
  { "F1WACK",   "red28.bm" },
  { "DEATH",    "death.bm" }
};

static_assert(sizeof(g_aCars) / sizeof(g_aCars[0]) ==
              ROLLER_ED_TEST_CAR_DESIGN_COUNT);

uint32_t ResolveFrontSurface(const tCarDesign &Design,
                             const tPolygon &Polygon)
{
  uint32_t uiSurface = Polygon.uiTex;
  if ((uiSurface & CAR_FLAG_ANMS_LOOKUP) != 0
      && Design.pAnms != reinterpret_cast<tAnimation *>(-1)) {
    const tAnimation &Animation = Design.pAnms[static_cast<uint8_t>(uiSurface)];
    if (Animation.uiCount != 0u)
      uiSurface = Animation.framesAy[0];
  }
  return uiSurface;
}

struct tCollectContext
{
  tEdCarGeometry *pGeometry;
};

void CollectSurface(const tEdSurfaceEmission *pSurface, void *pUserData)
{
  if (!pSurface || !pUserData
      || pSurface->uiVertexCount != ED_SURFACE_VERTEX_COUNT)
    return;

  tCollectContext *pContext = static_cast<tCollectContext *>(pUserData);
  tEdCarGeometry &Geometry = *pContext->pGeometry;
  const uint32_t uiBaseVertex = static_cast<uint32_t>(Geometry.Vertices.size());
  const uint32_t uiBaseIndex = static_cast<uint32_t>(Geometry.Indices.size());

  for (uint32_t i = 0; i < ED_SURFACE_VERTEX_COUNT; ++i) {
    tEdVertex Vertex = {};
    std::memcpy(Vertex.fPosition, pSurface->aVertices[i].fPosition,
                sizeof(Vertex.fPosition));
    std::memcpy(Vertex.fNormal, pSurface->aVertices[i].fNormal,
                sizeof(Vertex.fNormal));
    std::memcpy(Vertex.fUV, pSurface->aVertices[i].fMaterialUV,
                sizeof(Vertex.fUV));
    Geometry.Vertices.push_back(Vertex);
  }

  const uint32_t auiIndices[kIndicesPerQuad] = {
    uiBaseVertex + 0u, uiBaseVertex + 1u, uiBaseVertex + 2u,
    uiBaseVertex + 0u, uiBaseVertex + 2u, uiBaseVertex + 3u
  };
  Geometry.Indices.insert(Geometry.Indices.end(), auiIndices,
                          auiIndices + kIndicesPerQuad);

  tEdPrimitive Primitive = {};
  Primitive.uiFirstIndex = uiBaseIndex;
  Primitive.uiIndexCount = kIndicesPerQuad;
  Primitive.uiChunkId = ROLLER_ED_INVALID_CHUNK_ID;
  Primitive.uiFrontMaterialId = pSurface->uiFrontMaterialId;
  Primitive.uiBackMaterialId = pSurface->uiBackMaterialId;
  // The single-object export path ignores track grouping identity. These two
  // authored values keep the shared emitter's validation vocabulary intact.
  Primitive.unSurfaceClass = ROLLER_ED_SURFACE_CLASS_BUILDING;
  Primitive.unContentClass = ROLLER_ED_CONTENT_AUTHORED_SCENERY;
  if ((pSurface->unFlags & ROLLER_ED_SURFACE_FLAG_ALPHA) != 0)
    Primitive.unFlags |= ROLLER_ED_PRIMITIVE_FLAG_ALPHA_BLEND;
  if ((pSurface->unFlags & ROLLER_ED_SURFACE_FLAG_TWO_SIDED) != 0)
    Primitive.unFlags |= ROLLER_ED_PRIMITIVE_FLAG_TWO_SIDED;
  Primitive.byTopology = ROLLER_ED_TOPOLOGY_TRIANGLE_LIST;
  Geometry.Primitives.push_back(Primitive);
}
}

void tEdCarGeometry::Clear()
{
  Vertices.clear();
  Indices.clear();
  Primitives.clear();
  Materials.clear();
}

tEdExportGeometry tEdCarGeometry::View() const
{
  tEdExportGeometry View;
  View.pVertices = Vertices.empty() ? nullptr : Vertices.data();
  View.uiVertexCount = static_cast<uint32_t>(Vertices.size());
  View.puiIndices = Indices.empty() ? nullptr : Indices.data();
  View.uiIndexCount = static_cast<uint32_t>(Indices.size());
  View.pPrimitives = Primitives.empty() ? nullptr : Primitives.data();
  View.uiPrimitiveCount = static_cast<uint32_t>(Primitives.size());
  View.pMaterials = Materials.empty() ? nullptr : Materials.data();
  View.uiMaterialCount = static_cast<uint32_t>(Materials.size());
  return View;
}

uint32_t CEditorCarModel::Count()
{
  return static_cast<uint32_t>(sizeof(g_aCars) / sizeof(g_aCars[0]));
}

const char *CEditorCarModel::Name(uint32_t uiDesign)
{
  return uiDesign < Count() ? g_aCars[uiDesign].szName : nullptr;
}

const char *CEditorCarModel::TextureFileName(uint32_t uiDesign)
{
  return uiDesign < Count() ? g_aCars[uiDesign].szTexture : nullptr;
}

bool CEditorCarModel::Build(uint32_t uiDesign, uint32_t uiTextureTileCount,
                            tEdCarGeometry &GeometryOut,
                            std::string &sError)
{
  GeometryOut.Clear();
  sError.clear();
  if (uiDesign >= Count() || uiTextureTileCount == 0u) {
    sError = "the car design or texture tile count is invalid";
    return false;
  }

  const tCarDesign &Design = CarDesigns[uiDesign];
  if (!Design.pCoords || !Design.pPols || Design.byNumCoords == 0u
      || Design.byNumPols == 0u) {
    sError = "the car design has no geometry";
    return false;
  }

  const uint32_t uiRows = (uiTextureTileCount + 3u) / 4u;
  const tEdTextureAtlas Atlas = {
    kCarTextureSet, 256u, uiRows * 64u, 64u, uiTextureTileCount
  };
  GeometryOut.Materials.resize(kMaterialCapacity);
  tEdMaterialTable Materials;
  if (!ed_material_table_init(&Materials, GeometryOut.Materials.data(),
                              kMaterialCapacity, Atlas)) {
    GeometryOut.Clear();
    sError = "the car texture atlas is invalid";
    return false;
  }

  GeometryOut.Vertices.reserve(static_cast<size_t>(Design.byNumPols) * 4u);
  GeometryOut.Indices.reserve(static_cast<size_t>(Design.byNumPols) * 6u);
  GeometryOut.Primitives.reserve(Design.byNumPols);
  tCollectContext Context = { &GeometryOut };

  for (uint32_t i = 0; i < Design.byNumPols; ++i) {
    const tPolygon &Polygon = Design.pPols[i];
    float afVertices[ED_SURFACE_VERTEX_COUNT][3];
    for (uint32_t v = 0; v < ED_SURFACE_VERTEX_COUNT; ++v) {
      if (Polygon.verts[v] >= Design.byNumCoords) {
        GeometryOut.Clear();
        sError = "a car polygon references a missing coordinate";
        return false;
      }
      const tVec3 &Point = Design.pCoords[Polygon.verts[v]];
      afVertices[v][0] = Point.fX;
      afVertices[v][1] = Point.fY;
      afVertices[v][2] = Point.fZ;
    }

    const uint32_t uiSurface = ResolveFrontSurface(Design, Polygon);
    tEdSurfaceInfo Info = {};
    Info.uiChunkId = ROLLER_ED_INVALID_CHUNK_ID;
    Info.uiRenderFlags = uiSurface;
    Info.uiBackSurfaceFlags = Design.pBacks
        ? Design.pBacks[i] : ED_MATERIAL_ID_NONE;
    Info.uiTextureSet = kCarTextureSet;
    Info.bPairTextureEnabled =
        (uiSurface & SURFACE_FLAG_TEXTURE_PAIR) != 0;
    Info.unSurfaceClass = ROLLER_ED_SURFACE_CLASS_BUILDING;
    Info.unContentClass = ROLLER_ED_CONTENT_AUTHORED_SCENERY;
    Info.byTopology = ROLLER_ED_TOPOLOGY_QUAD;
    Info.byRenderUVLayout = static_cast<uint8_t>(Info.bPairTextureEnabled
        ? ROLLER_ED_RENDER_UV_PAIR_HORIZONTAL
        : ROLLER_ED_RENDER_UV_TILE);

    if (!ed_emit_surface(afVertices, &Info, &Materials, CollectSurface,
                         &Context)) {
      GeometryOut.Clear();
      sError = "car polygon " + std::to_string(i)
          + " could not be converted with the selected texture bank";
      return false;
    }
  }

  GeometryOut.Materials.resize(Materials.uiCount);
  if (GeometryOut.Primitives.empty() || GeometryOut.Materials.empty()) {
    GeometryOut.Clear();
    sError = "the car design produced no exportable geometry";
    return false;
  }
  return true;
}
