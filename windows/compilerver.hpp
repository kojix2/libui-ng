#ifndef __LIBUI_COMPILERVER__
#define __LIBUI_COMPILERVER__

// Visual Studio (Microsoft's compilers)
// VS2013 is needed for va_copy().
#ifdef _MSC_VER
#if _MSC_VER < 1800
#error Visual Studio 2013 or higher is required to build libui.
#endif
#endif

#ifdef __MINGW32__
// Silence warning: base class 'struct IUnknown' has accessible non-virtual destructor
// as MinGW does not process COM interfaces correctly
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#endif

// libui uses UTF-16 strings with the Windows API and requires a 16-bit wchar_t.
#include <limits.h>
#if WCHAR_MAX > 0xFFFF
#error unexpected: wchar_t larger than 16-bit on a Windows ABI build; contact andlabs with your build setup information
#endif

#endif
