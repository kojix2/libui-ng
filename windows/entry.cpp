// 8 april 2015
#include "uipriv_windows.hpp"

struct uiEntry {
	uiWindowsControl c;
	HWND hwnd;
	void (*onChanged)(uiEntry *, void *);
	void *onChangedData;
	BOOL inhibitChanged;
};

static BOOL onWM_COMMAND(uiControl *c, HWND hwnd, WORD code, LRESULT *lResult)
{
	uiEntry *e = uiEntry(c);

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

static void uiEntryDestroy(uiControl *c)
{
	uiEntry *e = uiEntry(c);

	uiWindowsUnregisterWM_COMMANDHandler(e->hwnd);
	uiWindowsEnsureDestroyWindow(e->hwnd);
	uiFreeControl(uiControl(e));
}

uiWindowsControlAllDefaultsExceptDestroy(uiEntry)

// from http://msdn.microsoft.com/en-us/library/windows/desktop/dn742486.aspx#sizingandspacing
#define entryWidth 107 /* this is actually the shorter progress bar width, but Microsoft only indicates as wide as necessary */
#define entryHeight 14

static void uiEntryMinimumSize(uiWindowsControl *c, int *width, int *height)
{
	uiEntry *e = uiEntry(c);
	uiWindowsSizing sizing;
	int x, y;

	x = entryWidth;
	y = entryHeight;
	uiWindowsGetSizing(e->hwnd, &sizing);
	uiWindowsSizingDlgUnitsToPixels(&sizing, &x, &y);
	*width = x;
	*height = y;
}

static void defaultOnChanged(uiEntry *e, void *data)
{
	// do nothing
}

char *uiEntryText(uiEntry *e)
{
	return uiWindowsWindowText(e->hwnd);
}

void uiEntrySetText(uiEntry *e, const char *text)
{
	// doing this raises an EN_CHANGED
	e->inhibitChanged = TRUE;
	uiWindowsSetWindowText(e->hwnd, text);
	// Only set the cursor if the entry has focus to avoid weird scrolling upon window
	// creation. Cursor placement is otherwise determined by mouse position upon click.
	if (GetFocus() == e->hwnd)
		Edit_SetSel(e->hwnd, (WPARAM) -1, (LPARAM) -1);
	e->inhibitChanged = FALSE;
	// don't queue the control for resize; entry sizes are independent of their contents
}

void uiEntryOnChanged(uiEntry *e, void (*f)(uiEntry *, void *), void *data)
{
	e->onChanged = f;
	e->onChangedData = data;
}

int uiEntryReadOnly(uiEntry *e)
{
	return (getStyle(e->hwnd) & ES_READONLY) != 0;
}

void uiEntrySetReadOnly(uiEntry *e, int readonly)
{
	if (Edit_SetReadOnly(e->hwnd, readonly) == 0)
		logLastError(L"error setting uiEntry read-only state");
}

static uiEntry *finishNewEntry(DWORD style)
{
	uiEntry *e;

	uiWindowsNewControl(uiEntry, e);

	e->hwnd = uiWindowsEnsureCreateControlHWND(WS_EX_CLIENTEDGE,
		L"edit", L"",
		style | ES_AUTOHSCROLL | ES_LEFT | ES_NOHIDESEL | WS_TABSTOP,
		hInstance, NULL,
		TRUE);

	uiWindowsRegisterWM_COMMANDHandler(e->hwnd, onWM_COMMAND, uiControl(e));
	uiEntryOnChanged(e, defaultOnChanged, NULL);

	return e;
}

uiEntry *uiNewEntry(void)
{
	return finishNewEntry(0);
}

uiEntry *uiNewPasswordEntry(void)
{
	return finishNewEntry(ES_PASSWORD);
}

// reserved left margin equals the control's client height, so the icon area is always a square
static int searchEntryIconAreaWidth(HWND hwnd)
{
	RECT rc;

	GetClientRect(hwnd, &rc);
	return rc.bottom - rc.top;
}

static void searchEntryDrawIcon(HWND hwnd, HDC dc)
{
	HPEN pen, oldpen;
	HGDIOBJ oldbrush;
	int area, r, cx, cy, penWidth;
	int hx1, hy1, hx2, hy2;
	BOOL releaseDC;

	area = searchEntryIconAreaWidth(hwnd);
	// proportions follow Feather icons' "search" glyph (circle cx=11 cy=11 r=8,
	// handle from (16.65,16.65) to (21,21), viewBox 24x24) scaled to the icon box
	r = (area * 8) / 24;
	if (r < 3)
		return;
	cx = (area * 11) / 24;
	cy = cx;
	hx1 = (area * 1665) / 2400;
	hy1 = hx1;
	hx2 = (area * 21) / 24;
	hy2 = hx2;
	penWidth = area / 16;
	if (penWidth < 1)
		penWidth = 1;

	releaseDC = FALSE;
	if (dc == NULL) {
		dc = GetDC(hwnd);
		if (dc == NULL)
			return;
		releaseDC = TRUE;
	}
	pen = CreatePen(PS_SOLID, penWidth, GetSysColor(COLOR_GRAYTEXT));
	oldpen = (HPEN) SelectObject(dc, pen);
	oldbrush = SelectObject(dc, GetStockObject(HOLLOW_BRUSH));

	// magnifying glass: a circle with a short diagonal handle
	Ellipse(dc, cx - r, cy - r, cx + r, cy + r);
	MoveToEx(dc, hx1, hy1, NULL);
	LineTo(dc, hx2, hy2);

	SelectObject(dc, oldbrush);
	SelectObject(dc, oldpen);
	DeleteObject(pen);
	if (releaseDC)
		ReleaseDC(hwnd, dc);
}

static LRESULT CALLBACK searchEntrySubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	LRESULT lResult;

	if (uMsg == WM_NCDESTROY) {
		if (RemoveWindowSubclass(hwnd, searchEntrySubclassProc, uIdSubclass) == FALSE)
			logLastError(L"RemoveWindowSubclass()");
		return DefSubclassProc(hwnd, uMsg, wParam, lParam);
	}

	lResult = DefSubclassProc(hwnd, uMsg, wParam, lParam);
	if (uMsg == WM_SIZE)
		SendMessageW(hwnd, EM_SETMARGINS, EC_LEFTMARGIN, MAKELPARAM(searchEntryIconAreaWidth(hwnd), 0));
	// The edit control draws typed text directly instead of always going through WM_PAINT,
	// so redraw the icon after every message rather than only on WM_PAINT. WM_PRINTCLIENT
	// supplies the target DC explicitly; drawing to the window DC there would omit the icon
	// from the printed/offscreen result and unnecessarily alter the visible window. The
	// default WM_PRINT handler delegates client drawing through WM_PRINTCLIENT.
	if (uMsg == WM_PRINTCLIENT)
		searchEntryDrawIcon(hwnd, (HDC) wParam);
	else if (uMsg != WM_PRINT)
		searchEntryDrawIcon(hwnd, NULL);
	return lResult;
}

uiEntry *uiNewSearchEntry(void)
{
	uiEntry *e;

	e = finishNewEntry(0);

	// "SearchBoxEditComposited" is meant for edits hosted on an Explorer-themed
	// toolbar; applied to a plain edit it strips the border/background instead.
	if (SetWindowSubclass(e->hwnd, searchEntrySubclassProc, 0, 0) == FALSE) {
		logLastError(L"SetWindowSubclass()");
		// Leave this as an ordinary entry rather than reserving an empty icon area.
	} else
		SendMessageW(e->hwnd, EM_SETMARGINS, EC_LEFTMARGIN, MAKELPARAM(searchEntryIconAreaWidth(e->hwnd), 0));

	return e;
}
