#ifndef _WHIPLIB_SIGNTYPE_H
#define _WHIPLIB_SIGNTYPE_H
//-------------------------------------------------------------------------------------------------
#include "Types.h"
//-------------------------------------------------------------------------------------------------

struct tSignType
{
  eWhipModel modelType;
  std::string sDescription;
  bool bCanHaveTexture;
  bool bBillboarded;
};

static tSignType g_signAy[17] = {
  { eWhipModel::SIGN_TOWER,   "TOWER"    , true , false},
  { eWhipModel::SIGN_TOWER2,  "TOWER 2"  , false, false},
  { eWhipModel::SIGN_SIGN01,  "SIGN 1"   , true , false},
  { eWhipModel::SIGN_SIGN02,  "SIGN 2"   , true , false},
  { eWhipModel::SIGN_BUILD,   "BUILD"    , false, false},
  { eWhipModel::SIGN_BUILD1,  "BUILD 1"  , false, false},
  { eWhipModel::SIGN_BUILD2,  "BUILD 2"  , false, false},
  { eWhipModel::SIGN_BUILD3,  "BUILD 3"  , true , false},
  { eWhipModel::SIGN_HEELBAR, "HEELBAR"  , false, false},
  { eWhipModel::SIGN_BALLOON, "BALLOON"  , true , true },
  { eWhipModel::SIGN_TREE,    "TREE"     , false, true },
  { eWhipModel::SIGN_ADVERT,  "ADVERT"   , true , false},
  { eWhipModel::SIGN_ADVERT2, "ADVERT 2" , true , false},
  { eWhipModel::SIGN_QUADBLD, "QUAD BLD" , false, false},
  { eWhipModel::SIGN_BLD0,    "BLD 0"    , false, false},
  { eWhipModel::SIGN_BALLOON2,"BIG BALL" , true , true },
  { eWhipModel::SIGN_ADVERT3, "BIG AD"   , true , false},
};
static int g_signAyCount = sizeof(g_signAy) / sizeof(g_signAy[0]);

//-------------------------------------------------------------------------------------------------

// Header-local helpers: not every translation unit that needs the table needs
// both lookups. g_signAy is deliberately left as a per-TU copy.
[[maybe_unused]] static const tSignType &GetSignTypeFromModel(eWhipModel model)
{
  for (int i = 0; i < g_signAyCount; ++i) {
    if (g_signAy[i].modelType == model)
      return g_signAy[i];
  }
  assert(0);
  return g_signAy[0];
}

//-------------------------------------------------------------------------------------------------

[[maybe_unused]] static bool IsSign(eWhipModel model)
{
  for (int i = 0; i < g_signAyCount; ++i) {
    if (g_signAy[i].modelType == model)
      return true;
  }
  return false;
}

//-------------------------------------------------------------------------------------------------
#endif