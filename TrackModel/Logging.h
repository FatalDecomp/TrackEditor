#ifndef _WHIPLIB_LOGGING_H
#define _WHIPLIB_LOGGING_H
//-------------------------------------------------------------------------------------------------

namespace Logging
{
  typedef void (*WhipLibLoggingCallback)(const char *szText, int iLen);
  extern void SetWhipLibLoggingCallback(WhipLibLoggingCallback pfnLoggingCallback);
  extern void LogMessage(const char *szText, ...);
}

//-------------------------------------------------------------------------------------------------
#endif
