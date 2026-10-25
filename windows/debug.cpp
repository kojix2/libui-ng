// 25 february 2015
#include "uipriv_windows.hpp"
#include <cstdlib>
#include <strsafe.h>

static void printDebug(const WCHAR *msg)
{
	if (IsDebuggerPresent())
		OutputDebugStringW(msg);
	else
		fputws(msg, stderr);
}

HRESULT _logLastError(debugargs, const WCHAR *s)
{
	DWORD le;
	WCHAR msg[2048];
	WCHAR formatted[512];

	le = GetLastError();

	// Logging can run on a uiQueueMain() producer thread. Keep it independent
	// of libui's unsynchronized allocation tracker.
	if (FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, le, 0, formatted, ARRAYSIZE(formatted), NULL) == 0)
		StringCchCopyW(formatted, ARRAYSIZE(formatted), L"(message unavailable)\n");
	StringCchPrintfW(msg, ARRAYSIZE(msg),
		L"[libui] %s:%s:%s() %s: GetLastError() == %lu %s",
		file, line, func,
		s, le, formatted);
	printDebug(msg);

	SetLastError(le);
	// a function does not have to set a last error
	// if the last error we get is actually 0, then HRESULT_FROM_WIN32(0) will return S_OK (0 cast to an HRESULT, since 0 <= 0), which we don't want
	// prevent this by returning E_FAIL
	if (le == 0)
		return E_FAIL;
	return HRESULT_FROM_WIN32(le);
}

HRESULT _logHRESULT(debugargs, const WCHAR *s, HRESULT hr)
{
	DWORD le;
	WCHAR msg[2048];
	WCHAR formatted[512];

	le = GetLastError();
	if (FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, hr, 0, formatted, ARRAYSIZE(formatted), NULL) == 0)
		StringCchCopyW(formatted, ARRAYSIZE(formatted), L"(message unavailable)\n");
	StringCchPrintfW(msg, ARRAYSIZE(msg),
		L"[libui] %s:%s:%s() %s: HRESULT == 0x%08lX %s",
		file, line, func,
		s, (unsigned long) hr, formatted);
	printDebug(msg);

	SetLastError(le);
	return hr;
}

#ifdef _UI_STATIC
extern "C" HRESULT uiprivWindowsLogLastErrorForTests(DWORD error, DWORD *after)
{
	SetLastError(error);
	HRESULT hr = _logLastError(L"test", L"1", L"test", L"injected failure");
	*after = GetLastError();
	return hr;
}

extern "C" HRESULT uiprivWindowsLogHRESULTForTests(HRESULT error, DWORD *after)
{
	SetLastError(ERROR_ACCESS_DENIED);
	HRESULT hr = _logHRESULT(L"test", L"1", L"test", L"injected failure", error);
	*after = GetLastError();
	return hr;
}
#endif

void uiprivRealBug(const char *file, const char *line, const char *func, const char *prefix, const char *format, va_list ap)
{
	va_list ap2;
	char *msg;
	size_t n;
	WCHAR *final;

	va_copy(ap2, ap);
	n = _vscprintf(format, ap2);
	va_end(ap2);
	n++;		// terminating '\0'

	msg = (char *) uiprivAlloc(n * sizeof (char), "char[]");
	// includes terminating '\0' according to example in https://msdn.microsoft.com/en-us/library/xa1a1a6z.aspx
	vsprintf_s(msg, n, format, ap);

	final = strf(L"[libui] %hs:%hs:%hs() %hs%hs\n", file, line, func, prefix, msg);
	uiprivFree(msg);
	printDebug(final);
	uiprivFree(final);

	// An explicit API or implementation invariant violation is fatal. A
	// debugger may inspect it first; continuing must not return to bad state.
	if (IsDebuggerPresent())
		DebugBreak();
	std::abort();
}
