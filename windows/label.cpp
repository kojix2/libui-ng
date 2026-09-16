// 11 april 2015
#include "uipriv_windows.hpp"
#include <float.h>

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
	char *previous = uiWindowsWindowText(l->hwnd);
	int unchanged = strcmp(previous, text) == 0;
	uiFreeText(previous);
	if (unchanged)
		return;
	int oldWidth, oldHeight, width, height;
	uiWindowsControlMinimumSize(uiWindowsControl(l), &oldWidth, &oldHeight);
	uiWindowsSetWindowText(l->hwnd, text);
	uiWindowsControlMinimumSize(uiWindowsControl(l), &width, &height);
	if (width != oldWidth || height != oldHeight)
		uiWindowsControlMinimumSizeChanged(uiWindowsControl(l));
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
	font = uiprivControlFontForSize(l->hwnd, hMessageFont, size);
	if (font == NULL)
		return;
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
	l->fontSize = uiprivControlFontSize(l->hwnd, hMessageFont);
	l->defaultFontSize = l->fontSize;

	return l;
}
