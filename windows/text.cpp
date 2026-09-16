// 9 april 2015
#include "uipriv_windows.hpp"
#include <limits.h>

WCHAR *windowTextAndLen(HWND hwnd, LRESULT *len)
{
	int n;
	int copied;
	WCHAR *text;

	n = (int) SendMessageW(hwnd, WM_GETTEXTLENGTH, 0, 0);
	if (len != NULL)
			*len = n;
	// WM_GETTEXTLENGTH does not include the null terminator
	text = (WCHAR *) uiprivAlloc((n + 1) * sizeof (WCHAR), "WCHAR[]");
	SetLastError(ERROR_SUCCESS);
	copied = GetWindowTextW(hwnd, text, n + 1);
	if (copied == 0 && GetLastError() != ERROR_SUCCESS) {
		logLastError(L"error getting window text");
		// on error, return an empty string to be safe
		*text = L'\0';
		if (len != NULL)
			*len = 0;
	} else if (len != NULL) {
		*len = copied;
	}
	return text;
}

WCHAR *windowText(HWND hwnd)
{
	return windowTextAndLen(hwnd, NULL);
}

void setWindowText(HWND hwnd, WCHAR *wtext)
{
	if (SetWindowTextW(hwnd, wtext) == 0)
		logLastError(L"error setting window text");
}

void uiFreeText(char *text)
{
	uiprivFree(text);
}

// via http://msdn.microsoft.com/en-us/library/windows/desktop/dn742486.aspx#sizingandspacing
#define labelHeight 8

int uiWindowsWindowTextHeight(HWND hwnd)
{
	LRESULT len;
	WCHAR* text, *start;
	int lineCount = 1;

	text = windowTextAndLen(hwnd, &len);
	for (start = text; start != text + len; start++)
		if (*start == L'\n')
			lineCount++;

	uiprivFree(text);
	return lineCount * labelHeight;
}

void uiprivWindowTextSize(HWND hwnd, HFONT font, int *width, int *height)
{
	LRESULT len;
	WCHAR *text, *start, *end, *textEnd;
	HDC dc;
	HFONT prevfont;
	SIZE size;
	TEXTMETRICW metrics;
	int lineCount;

	int maxWidth = 0;
	int totalHeight = 0;

	if (width != NULL)
		*width = 0;
	if (height != NULL)
		*height = 0;

	text = windowTextAndLen(hwnd, &len);
	dc = GetDC(hwnd);
	if (dc == NULL) {
		logLastError(L"error getting DC");
		goto noTextOrError;
	}
	prevfont = (HFONT) SelectObject(dc, font);
	if (prevfont == NULL) {
		logLastError(L"error loading control font into device context");
		ReleaseDC(hwnd, dc);
		goto noTextOrError;
	}

	ZeroMemory(&metrics, sizeof (TEXTMETRICW));
	if (GetTextMetricsW(dc, &metrics) == 0)
		logLastError(L"error getting control font metrics");
	else {
		lineCount = 1;
		for (start = text; start != text + len; start++)
			if (*start == L'\n')
				lineCount++;
		if (metrics.tmHeight > 0 && lineCount > INT_MAX / metrics.tmHeight)
			totalHeight = INT_MAX;
		else
			totalHeight = lineCount * metrics.tmHeight;
	}

	// Calculate the maximum width of all lines, including text before and
	// after empty lines. Empty lines have zero width but still contribute to
	// the height above.
	textEnd = text + len;
	start = text;
	for (;;) {
		end = start;
		while (end != textEnd && *end != L'\n')
			end++;
		if (end != start &&
			GetTextExtentPoint32W(dc, start, (int) (end - start), &size) == 0)
			logLastError(L"error getting text extent point");
		else if (end != start && size.cx > maxWidth)
			maxWidth = size.cx;
		if (end == textEnd)
			break;
		start = end + 1;
	}

	if (SelectObject(dc, prevfont) != font)
		logLastError(L"error restoring previous font into device context");
	if (ReleaseDC(hwnd, dc) == 0)
		logLastError(L"error releasing DC");

	uiprivFree(text);
	if (width != NULL)
		*width = maxWidth;
	if (height != NULL)
		*height = totalHeight;
	return;

noTextOrError:
	uiprivFree(text);
}

int uiWindowsWindowTextWidth(HWND hwnd)
{
	int width;

	uiprivWindowTextSize(hwnd, hMessageFont, &width, NULL);
	return width;
}

char *uiWindowsWindowText(HWND hwnd)
{
	WCHAR *wtext;
	char *text;

	wtext = windowText(hwnd);
	text = toUTF8(wtext);
	uiprivFree(wtext);
	return text;
}

void uiWindowsSetWindowText(HWND hwnd, const char *text)
{
	WCHAR *wtext;

	wtext = toUTF16(text);
	setWindowText(hwnd, wtext);
	uiprivFree(wtext);
}

int uiprivStricmp(const char *a, const char *b)
{
	WCHAR *wa, *wb;
	int ret;

	wa = toUTF16(a);
	wb = toUTF16(b);
	ret = CompareStringOrdinal(wa, -1, wb, -1, TRUE);
	if (ret == 0) {
		logLastError(L"error comparing Unicode strings");
		ret = _wcsicmp(wa, wb);
	} else
		ret -= CSTR_EQUAL;
	uiprivFree(wb);
	uiprivFree(wa);
	return ret;
}
