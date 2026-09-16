// 11 april 2015
#include "uipriv_windows.hpp"
#include <float.h>
#include <limits.h>

struct uiLabel {
	uiWindowsControl c;
	HWND hwnd;
	HFONT customFont;
	double fontSize;
	double defaultFontSize;
};

static void uiLabelDestroy(uiControl *c)
{
	uiLabel *l = uiLabel(c);
	HFONT customFont = l->customFont;

	uiWindowsEnsureDestroyWindow(l->hwnd);
	if (customFont != NULL && DeleteObject(customFont) == 0)
		logLastError(L"error deleting label font");
	uiFreeControl(c);
}

uiWindowsControlAllDefaultsExceptDestroy(uiLabel)

static void uiLabelMinimumSize(uiWindowsControl *c, int *width, int *height)
{
	uiLabel *l = uiLabel(c);
	HFONT font;

	font = l->customFont;
	if (font == NULL)
		font = hMessageFont;
	uiprivWindowTextSize(l->hwnd, font, width, height);
}

char *uiLabelText(uiLabel *l)
{
	return uiWindowsWindowText(l->hwnd);
}

void uiLabelSetText(uiLabel *l, const char *text)
{
	uiWindowsSetWindowText(l->hwnd, text);
	// changing the text might necessitate a change in the label's size
	uiWindowsControlMinimumSizeChanged(uiWindowsControl(l));
}

static double labelFontSize(HWND hwnd, HFONT font)
{
	HDC dc;
	HFONT prevfont;
	TEXTMETRICW metrics;
	int dpi;
	double size = 0;

	dc = GetDC(hwnd);
	if (dc == NULL) {
		logLastError(L"error getting DC for label font size");
		return 0;
	}
	prevfont = (HFONT) SelectObject(dc, font);
	if (prevfont == NULL) {
		logLastError(L"error selecting label font");
		ReleaseDC(hwnd, dc);
		return 0;
	}
	ZeroMemory(&metrics, sizeof (TEXTMETRICW));
	dpi = GetDeviceCaps(dc, LOGPIXELSY);
	if (GetTextMetricsW(dc, &metrics) == 0)
		logLastError(L"error getting label font metrics");
	else if (dpi == 0)
		logLastError(L"error getting vertical display resolution");
	else
		size = (metrics.tmHeight - metrics.tmInternalLeading) * 72.0 / dpi;
	if (SelectObject(dc, prevfont) != font)
		logLastError(L"error restoring previous font into device context");
	if (ReleaseDC(hwnd, dc) == 0)
		logLastError(L"error releasing DC");
	return size;
}

static HFONT labelFontForSize(uiLabel *l, double size)
{
	LOGFONTW font;
	HDC dc;
	int dpi;
	double height;

	if (GetObjectW(hMessageFont, sizeof (LOGFONTW), &font) != sizeof (LOGFONTW)) {
		logLastError(L"error getting default label font description");
		return NULL;
	}
	dc = GetDC(l->hwnd);
	if (dc == NULL) {
		logLastError(L"error getting DC for label font");
		return NULL;
	}
	dpi = GetDeviceCaps(dc, LOGPIXELSY);
	if (ReleaseDC(l->hwnd, dc) == 0)
		logLastError(L"error releasing DC");
	if (dpi == 0) {
		logLastError(L"error getting vertical display resolution");
		return NULL;
	}
	height = size * dpi / 72.0;
	if (height > LONG_MAX) {
		uiprivUserBug("uiLabelSetFontSize() size cannot be represented by GDI.");
		return NULL;
	}
	font.lfHeight = -(LONG) (height + 0.5);
	if (font.lfHeight == 0)
		font.lfHeight = -1;
	font.lfWidth = 0;
	return CreateFontIndirectW(&font);
}

double uiLabelFontSize(uiLabel *l)
{
	return l->fontSize;
}

void uiLabelSetFontSize(uiLabel *l, double size)
{
	HFONT font;
	HFONT oldFont;

	if (!(size > 0) || size > DBL_MAX) {
		uiprivUserBug("uiLabelSetFontSize() size must be finite and positive.");
		return;
	}
	font = labelFontForSize(l, size);
	if (font == NULL) {
		logLastError(L"error creating label font");
		return;
	}
	oldFont = l->customFont;
	SendMessageW(l->hwnd, WM_SETFONT, (WPARAM) font, (LPARAM) TRUE);
	l->customFont = font;
	l->fontSize = size;
	if (oldFont != NULL && DeleteObject(oldFont) == 0)
		logLastError(L"error deleting previous label font");
	uiWindowsControlMinimumSizeChanged(uiWindowsControl(l));
}

void uiLabelResetFontSize(uiLabel *l)
{
	HFONT oldFont;

	oldFont = l->customFont;
	SendMessageW(l->hwnd, WM_SETFONT, (WPARAM) hMessageFont, (LPARAM) TRUE);
	l->customFont = NULL;
	l->fontSize = l->defaultFontSize;
	if (oldFont != NULL && DeleteObject(oldFont) == 0)
		logLastError(L"error deleting label font");
	uiWindowsControlMinimumSizeChanged(uiWindowsControl(l));
}

uiLabel *uiNewLabel(const char *text)
{
	uiLabel *l;
	WCHAR *wtext;

	uiWindowsNewControl(uiLabel, l);

	wtext = toUTF16(text);
	l->hwnd = uiWindowsEnsureCreateControlHWND(0,
		L"static", wtext,
		// SS_LEFTNOWORDWRAP clips text past the end; SS_NOPREFIX avoids accelerator translation
		// controls are vertically aligned to the top by default (thanks Xeek in irc.freenode.net/#winapi)
		SS_LEFTNOWORDWRAP | SS_NOPREFIX,
		hInstance, NULL,
		TRUE);
	uiprivFree(wtext);
	l->customFont = NULL;
	l->fontSize = labelFontSize(l->hwnd, hMessageFont);
	l->defaultFontSize = l->fontSize;

	return l;
}
