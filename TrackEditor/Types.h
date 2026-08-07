#ifndef _WHIPLIB_TYPES_H
#define _WHIPLIB_TYPES_H
//-------------------------------------------------------------------------------------------------
#include <stddef.h>
#include <string>
//-------------------------------------------------------------------------------------------------
#if defined(IS_WINDOWS)
typedef signed char      int8;
typedef signed short     int16;
typedef signed int       int32;
typedef signed __int64   int64;
typedef unsigned char    uint8;
typedef unsigned short   uint16;
typedef unsigned int     uint32;
typedef unsigned __int64 uint64;
#else
typedef signed char        int8;
typedef signed short       int16;
typedef signed int         int32;
typedef signed long long   int64;
typedef unsigned char      uint8;
typedef unsigned short     uint16;
typedef unsigned int       uint32;
typedef unsigned long long uint64;
#endif
//-------------------------------------------------------------------------------------------------

enum class eShapeSection
{
  CENTER = 0,
  LSHOULDER,
  RSHOULDER,
  LWALL,
  RWALL,
  ROOF,
  OWALLFLOOR,
  LLOWALL,
  RLOWALL,
  LUOWALL,
  RUOWALL,
  SELECTED,
  AILINE1, //visualized AI lines positioned above track
  AILINE2,
  AILINE3,
  AILINE4,
  CARLINE1, //actual AI lines at track surface level
  CARLINE2,
  CARLINE3,
  CARLINE4,
  EXPORT,
  CENTERLINE
};

//-------------------------------------------------------------------------------------------------

enum class eWhipModel
{
  CAR_F1WACK = 0,
  CAR_XAUTO,
  CAR_XDESILVA,
  CAR_XPULSE,
  CAR_XGLOBAL,
  CAR_XMILLION,
  CAR_XMISSION,
  CAR_XZIZIN,
  CAR_XREISE,
  CAR_YAUTO,
  CAR_YDESILVA,
  CAR_YPULSE,
  CAR_YGLOBAL,
  CAR_YMILLION,
  CAR_YMISSION,
  CAR_YZIZIN,
  CAR_YREISE,
  CAR_DEATH,
  SIGN_TOWER,
  SIGN_TOWER2,
  SIGN_SIGN01,
  SIGN_SIGN02,
  SIGN_BUILD,
  SIGN_BUILD1,
  SIGN_BUILD2,
  SIGN_BUILD3,
  SIGN_HEELBAR,
  SIGN_BALLOON,
  SIGN_BALLOON2,
  SIGN_TREE,
  SIGN_ADVERT,
  SIGN_ADVERT2,
  SIGN_ADVERT3,
  SIGN_QUADBLD,
  SIGN_BLD0
};

//-------------------------------------------------------------------------------------------------

static std::string g_transparencyAy[5] = {
  "Invisible",
  "Light Tint",
  "Medium Tint",
  "Dark Tint",
  "Light Blue",
};
static int g_transparencyAyCount = 5;

//-------------------------------------------------------------------------------------------------

#define STUNT_FLAG_LSHOULDER  0x01
#define STUNT_FLAG_LWALL      0x02
#define STUNT_FLAG_LLANE      0x04
#define STUNT_FLAG_RLANE      0x08
#define STUNT_FLAG_RSHOULDER  0x10
#define STUNT_FLAG_RWALL      0x20

//-------------------------------------------------------------------------------------------------
#endif
