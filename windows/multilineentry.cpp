// 8 april 2015
#include <float.h>
#include <limits.h>
#include "uipriv_windows.hpp"

// TODO there's alpha darkening of text going on in read-only ones; something is up in our parent logic

struct uiMultilineEntry {
	uiWindowsControl c;
	HWND hwnd;
	HFONT customFont;
	double fontSize;
	double defaultFontSize;
	void (*onChanged)(uiMultilineEntry *, void *);
	void *onChangedData;
	BOOL inhibitChanged;
};

static BOOL onWM_COMMAND(uiControl *c, HWND hwnd, WORD code, LRESULT *lResult)
{
	uiMultilineEntry *e = uiMultilineEntry(c);

	if (code != EN_CHANGE)
		return FALSE;
	if (e->inhibitChanged)
		return FALSE;
	if (!uiprivUserCallbackEnter(uiControl(e))) {
		*lResult = 0;
		return TRUE;
	}
	(*(e->onChanged))(e, e->onChangedData);
	*lResult = 0;
	uiprivUserCallbackLeave();
	return TRUE;
}

static void uiMultilineEntryDestroy(uiControl *c)
{
	uiMultilineEntry *e = uiMultilineEntry(c);
	HFONT customFont;

	customFont = e->customFont;
	uiWindowsUnregisterWM_COMMANDHandler(e->hwnd);
	uiprivDestroyTooltip(c);
	uiWindowsEnsureDestroyWindow(e->hwnd);
	if (customFont != NULL && DeleteObject(customFont) == 0)
		logLastError(L"error deleting multiline entry font");
	uiFreeControl(uiControl(e));
}

uiWindowsControlAllDefaultsExceptDestroy(uiMultilineEntry)

// from http://msdn.microsoft.com/en-us/library/windows/desktop/dn742486.aspx#sizingandspacing
#define entryWidth 107 /* this is actually the shorter progress bar width, but Microsoft only indicates as wide as necessary */
// LONGTERM change this for multiline text boxes (longterm because how?)
#define entryHeight 14

static void uiMultilineEntryMinimumSize(uiWindowsControl *c, int *width, int *height)
{
	uiMultilineEntry *e = uiMultilineEntry(c);
	uiWindowsSizing sizing;
	int x, y;

	x = entryWidth;
	y = entryHeight;
	uiWindowsGetSizing(e->hwnd, &sizing);
	uiWindowsSizingDlgUnitsToPixels(&sizing, &x, &y);
	*width = x;
	*height = y;
}

static void defaultOnChanged(uiMultilineEntry *e, void *data)
{
	// do nothing
}

char *uiMultilineEntryText(uiMultilineEntry *e)
{
	char *out;

	out = uiWindowsWindowText(e->hwnd);
	CRLFtoLF(out);
	return out;
}

void uiMultilineEntrySetText(uiMultilineEntry *e, const char *text)
{
	char *crlf;

	// doing this raises an EN_CHANGED
	e->inhibitChanged = TRUE;
	crlf = LFtoCRLF(text);
	uiWindowsSetWindowText(e->hwnd, crlf);
	uiprivFree(crlf);
	e->inhibitChanged = FALSE;
	// don't queue the control for resize; entry sizes are independent of their contents
}

void uiMultilineEntryAppend(uiMultilineEntry *e, const char *text)
{
	DWORD selStart, selEnd;
	LRESULT textLen;
	char *crlf;
	WCHAR *wtext;

	// doing this raises an EN_CHANGED
	e->inhibitChanged = TRUE;

	// Save current selection
	SendMessageW(e->hwnd, EM_GETSEL, (WPARAM) &selStart, (LPARAM) &selEnd);
	// Append by replacing an empty selection at the end of the input
	textLen = SendMessageW(e->hwnd, WM_GETTEXTLENGTH, 0, 0);
	Edit_SetSel(e->hwnd, textLen, textLen);
	crlf = LFtoCRLF(text);
	wtext = toUTF16(crlf);
	uiprivFree(crlf);
	Edit_ReplaceSel(e->hwnd, wtext);
	uiprivFree(wtext);
	// Restore selection
	Edit_SetSel(e->hwnd, selStart, selEnd);

	e->inhibitChanged = FALSE;
}

void uiMultilineEntryOnChanged(uiMultilineEntry *e, void (*f)(uiMultilineEntry *, void *), void *data)
{
	e->onChanged = f;
	e->onChangedData = data;
}

static double multilineEntryFontSizeFromHFONT(HWND hwnd, HFONT font)
{
	HDC dc;
	HGDIOBJ oldFont;
	TEXTMETRICW metrics;
	int dpi;
	double size;

	dc = GetDC(hwnd);
	if (dc == NULL) {
		logLastError(L"error getting device context for multiline entry font");
		return 0;
	}
	oldFont = SelectObject(dc, font);
	if (oldFont == NULL || oldFont == HGDI_ERROR) {
		logLastError(L"error selecting multiline entry font");
		ReleaseDC(hwnd, dc);
		return 0;
	}
	if (GetTextMetricsW(dc, &metrics) == 0) {
		logLastError(L"error getting multiline entry font metrics");
		SelectObject(dc, oldFont);
		ReleaseDC(hwnd, dc);
		return 0;
	}
	dpi = GetDeviceCaps(dc, LOGPIXELSY);
	if (dpi <= 0) {
		logLastError(L"error getting vertical DPI for multiline entry font");
		SelectObject(dc, oldFont);
		ReleaseDC(hwnd, dc);
		return 0;
	}
	size = ((double) metrics.tmHeight - metrics.tmInternalLeading) * 72.0 / dpi;
	SelectObject(dc, oldFont);
	ReleaseDC(hwnd, dc);
	return size;
}

static HFONT multilineEntryFontForSize(uiMultilineEntry *e, double size)
{
	HFONT baseFont;
	LOGFONTW lf;
	HDC dc;
	double height;
	int dpi;
	HFONT font;

	baseFont = (HFONT) SendMessageW(e->hwnd, WM_GETFONT, 0, 0);
	if (baseFont == NULL)
		baseFont = hMessageFont;
	if (GetObjectW(baseFont, sizeof (LOGFONTW), &lf) == 0) {
		logLastError(L"error getting multiline entry font description");
		return NULL;
	}

	dc = GetDC(e->hwnd);
	if (dc == NULL) {
		logLastError(L"error getting device context for multiline entry font");
		return NULL;
	}
	dpi = GetDeviceCaps(dc, LOGPIXELSY);
	ReleaseDC(e->hwnd, dc);
	if (dpi <= 0) {
		logLastError(L"error getting vertical DPI for multiline entry font");
		return NULL;
	}
	height = size * dpi / 72.0;
	if (height > (double) LONG_MAX - 0.5) {
		uiprivUserBug(
			"uiMultilineEntrySetFontSize() size cannot be represented by GDI.");
		return NULL;
	}

	lf.lfHeight = -(LONG) (height + 0.5);
	if (lf.lfHeight == 0)
		lf.lfHeight = -1;
	lf.lfWidth = 0;
	font = CreateFontIndirectW(&lf);
	if (font == NULL)
		logLastError(L"error creating multiline entry font");
	return font;
}

static void setMultilineEntryFontSize(uiMultilineEntry *e, double size)
{
	HFONT font;
	HFONT oldFont;

	font = multilineEntryFontForSize(e, size);
	if (font == NULL)
		return;
	oldFont = e->customFont;
	SendMessageW(e->hwnd, WM_SETFONT, (WPARAM) font, (LPARAM) TRUE);
	e->customFont = font;
	e->fontSize = size;
	if (oldFont != NULL && DeleteObject(oldFont) == 0)
		logLastError(L"error deleting previous multiline entry font");
}

double uiMultilineEntryFontSize(uiMultilineEntry *e)
{
	return e->fontSize;
}

void uiMultilineEntrySetFontSize(uiMultilineEntry *e, double size)
{
	if (!(size > 0) || size > DBL_MAX) {
		uiprivUserBug(
			"uiMultilineEntrySetFontSize() size must be finite and positive.");
		return;
	}

	setMultilineEntryFontSize(e, size);
}

void uiMultilineEntryResetFontSize(uiMultilineEntry *e)
{
	setMultilineEntryFontSize(e, e->defaultFontSize);
}

int uiMultilineEntryReadOnly(uiMultilineEntry *e)
{
	return (getStyle(e->hwnd) & ES_READONLY) != 0;
}

void uiMultilineEntrySetReadOnly(uiMultilineEntry *e, int readonly)
{
	if (Edit_SetReadOnly(e->hwnd, readonly) == 0)
		logLastError(L"error setting uiMultilineEntry read-only state");
}

static uiMultilineEntry *finishMultilineEntry(DWORD style)
{
	uiMultilineEntry *e;
	HFONT font;

	uiWindowsNewControl(uiMultilineEntry, e);

	e->hwnd = uiWindowsEnsureCreateControlHWND(WS_EX_CLIENTEDGE,
		L"edit", L"",
		ES_AUTOVSCROLL | ES_LEFT | ES_MULTILINE | ES_NOHIDESEL | ES_WANTRETURN | WS_TABSTOP | WS_VSCROLL | style,
		hInstance, NULL,
		TRUE);
	font = (HFONT) SendMessageW(e->hwnd, WM_GETFONT, 0, 0);
	if (font == NULL)
		font = hMessageFont;
	e->customFont = NULL;
	e->fontSize = multilineEntryFontSizeFromHFONT(e->hwnd, font);
	e->defaultFontSize = e->fontSize;

	uiWindowsRegisterWM_COMMANDHandler(e->hwnd, onWM_COMMAND, uiControl(e));
	uiMultilineEntryOnChanged(e, defaultOnChanged, NULL);

	return e;
}

uiMultilineEntry *uiNewMultilineEntry(void)
{
	return finishMultilineEntry(0);
}

uiMultilineEntry *uiNewNonWrappingMultilineEntry(void)
{
	return finishMultilineEntry(WS_HSCROLL | ES_AUTOHSCROLL);
}
