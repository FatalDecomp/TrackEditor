#include "Logging.h"
#include <stdarg.h>
#include <stdio.h>
//-------------------------------------------------------------------------------------------------
#if defined(_DEBUG) && defined(IS_WINDOWS)
#define new new(_CLIENT_BLOCK, __FILE__, __LINE__)
#endif
//-------------------------------------------------------------------------------------------------

static Logging::WhipLibLoggingCallback g_pfnLoggingCallback = NULL;

//-------------------------------------------------------------------------------------------------

void Logging::SetWhipLibLoggingCallback(Logging::WhipLibLoggingCallback pfnLoggingCallback)
{
  g_pfnLoggingCallback = pfnLoggingCallback;
}

//-------------------------------------------------------------------------------------------------

void Logging::LogMessage(const char *szText, ...)
{
  if (g_pfnLoggingCallback) {
    va_list args;
    va_start(args, szText);
    char szMsg[2048];
    int iLen = vsnprintf(szMsg, sizeof(szMsg) - 1, szText, args);
    if (iLen >= 0)
      szMsg[iLen] = '\0';
    va_end(args);

    (*g_pfnLoggingCallback)(szMsg, iLen);
  }
}

//-------------------------------------------------------------------------------------------------