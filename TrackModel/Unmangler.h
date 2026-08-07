#ifndef _WHIPLIB_UNMANGLER_H
#define _WHIPLIB_UNMANGLER_H
//-------------------------------------------------------------------------------------------------
#include <stdint.h>
#include <vector>
//-------------------------------------------------------------------------------------------------
#define MAX_MANGLED_LENGTH 67108864 // arbitrary 64 MB max, realistic maximum is much smaller
//-------------------------------------------------------------------------------------------------

namespace Unmangler
{
  extern int GetUnmangledLength(const uint8_t *pSource, int iSourceLen);
  extern bool UnmangleFile(const uint8_t *pSource, int iSourceLen, uint8_t *pDestination, int iDestLength);
  extern void MangleFile(const std::vector<uint8_t> &source, std::vector<uint8_t> &destination);
}

//-------------------------------------------------------------------------------------------------
#endif