// 27 april 2015
#include "uipriv_windows.hpp"

#define windowClass L"libui_uiWindowClass"

struct uiWindow {
	uiWindowsControl c;
	HWND hwnd;
	HMENU menubar;
	uiControl *child;
	BOOL shownOnce;
	int visible;
	int margined;
	int resizeable;
	BOOL hasMenubar;
	BOOL changingSize;
	int fullscreen;
	WINDOWPLACEMENT fsPrevPlacement;
	int borderless;
	int focused;

	int (*onClosing)(uiWindow *, void *);
	void *onClosingData;
	void (*onContentSizeChanged)(uiWindow *, void *);
	void *onContentSizeChangedData;
	void (*onFocusChanged)(uiWindow *, void *);
	void *onFocusChangedData;
	void (*onPositionChanged)(uiWindow *, void *);
	void *onPositionChangedData;
	BOOL changingPosition;
};

static std::set<uiWindow *> windows;

static BOOL isMenuCommand(WPARAM wParam, LPARAM lParam)
{
	if (lParam != 0)
		return FALSE;

	// IsDialogMessage() will also generate IDOK and IDCANCEL when pressing
	// Enter and Escape (respectively) on some controls, like EDIT controls.
	// These reserved dialog command IDs are not menu item IDs.
	if (HIWORD(wParam) != 0 || LOWORD(wParam) <= IDCANCEL)
		return FALSE;

	return TRUE;
}

// from https://msdn.microsoft.com/en-us/library/windows/desktop/dn742486.aspx#sizingandspacing
#define windowMargin 7

static void windowMargins(uiWindow *w, int *mx, int *my)
{
	uiWindowsSizing sizing;

	*mx = 0;
	*my = 0;
	if (!w->margined)
		return;
	uiWindowsGetSizing(w->hwnd, &sizing);
	*mx = windowMargin;
	*my = windowMargin;
	uiWindowsSizingDlgUnitsToPixels(&sizing, mx, my);
}

static void windowRelayout(uiWindow *w)
{
	int x, y, width, height;
	RECT r;
	int mx, my;
	HWND child;

	if (w->child == NULL)
		return;
	x = 0;
	y = 0;
	uiWindowsEnsureGetClientRect(w->hwnd, &r);
	width = r.right - r.left;
	height = r.bottom - r.top;
	windowMargins(w, &mx, &my);
	x += mx;
	y += my;
	width -= 2 * mx;
	height -= 2 * my;
	child = (HWND) uiControlHandle(w->child);
	uiWindowsEnsureMoveWindowDuringResize(child, x, y, width, height);
}

static void updateFrame(uiWindow *w)
{
	if (SetWindowPos(w->hwnd, NULL,
		0, 0, 0, 0,
		SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER) == 0)
		logLastError(L"error updating window frame");
}

static void updateWindowStyle(uiWindow *w)
{
	DWORD style;

	style = getStyle(w->hwnd) & ~WS_OVERLAPPEDWINDOW;
	if (!w->fullscreen && !w->borderless) {
		style |= WS_OVERLAPPEDWINDOW;
		if (!w->resizeable)
			style &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
	}
	setStyle(w->hwnd, style);
}

static LRESULT CALLBACK windowWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LONG_PTR ww;
	uiWindow *w;
	CREATESTRUCTW *cs = (CREATESTRUCTW *) lParam;
	WINDOWPOS *wp = (WINDOWPOS *) lParam;
	MINMAXINFO *mmi = (MINMAXINFO *) lParam;
	int width, height;
	LRESULT lResult;

	ww = GetWindowLongPtrW(hwnd, GWLP_USERDATA);
	if (ww == 0) {
		if (uMsg == WM_CREATE)
			SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR) (cs->lpCreateParams));
		// fall through to DefWindowProc() anyway
		return DefWindowProcW(hwnd, uMsg, wParam, lParam);
	}
	w = uiWindow((void *) ww);
	if (handleParentMessages(hwnd, uMsg, wParam, lParam, &lResult) != FALSE)
		return lResult;
	switch (uMsg) {
	case WM_COMMAND:
		if (!isMenuCommand(wParam, lParam))
			break;
		runMenuEvent(LOWORD(wParam), uiWindow(w));
		return 0;
	case WM_WINDOWPOSCHANGED:
		if (!uiprivUserCallbackEnter(uiControl(w)))
			return 0;
		if ((wp->flags & SWP_NOMOVE) == 0)
			if (w->onPositionChanged != NULL)
				if (!w->changingPosition) {
					(*(w->onPositionChanged))(w, w->onPositionChangedData);
					if (uiprivControlDestroyPending(uiControl(w))) {
						uiprivUserCallbackLeave();
						return 0;
					}
				}
		if ((wp->flags & SWP_NOSIZE) != 0) {
			uiprivUserCallbackLeave();
			break;
		}
		// WM_WINDOWPOSCHANGED can arrive before window construction is complete.
		if (w->onContentSizeChanged != NULL)
			if (!w->changingSize) {
				(*(w->onContentSizeChanged))(w, w->onContentSizeChangedData);
				if (uiprivControlDestroyPending(uiControl(w))) {
					uiprivUserCallbackLeave();
					return 0;
				}
			}
		windowRelayout(w);
		uiprivUserCallbackLeave();
		return 0;
	case WM_GETMINMAXINFO:
		// ensure the user cannot resize the window smaller than its minimum size
		lResult = DefWindowProcW(hwnd, uMsg, wParam, lParam);
		uiWindowsControlMinimumSize(uiWindowsControl(w), &width, &height);
		// width and height are in client coordinates; ptMinTrackSize is in window coordinates
		clientSizeToWindowSize(w->hwnd, &width, &height, w->hasMenubar);
		mmi->ptMinTrackSize.x = width;
		mmi->ptMinTrackSize.y = height;
		return lResult;
	case WM_ACTIVATE:
		if (LOWORD(wParam) == WA_INACTIVE)
			w->focused = 0;
		else
			w->focused = 1;
		if (!uiprivUserCallbackEnter(uiControl(w)))
			return 0;
		w->onFocusChanged(w, w->onFocusChangedData);
		uiprivUserCallbackLeave();
		return 0;
	case WM_CLOSE:
		if (!uiprivUserCallbackEnter(uiControl(w)))
			return 0;
		if ((*(w->onClosing))(w, w->onClosingData))
			uiControlDestroy(uiControl(w));
		uiprivUserCallbackLeave();
		return 0;		// we destroyed it already
	}
	return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

ATOM registerWindowClass(HICON hDefaultIcon, HCURSOR hDefaultCursor)
{
	WNDCLASSW wc;

	ZeroMemory(&wc, sizeof (WNDCLASSW));
	wc.lpszClassName = windowClass;
	wc.lpfnWndProc = windowWndProc;
	wc.hInstance = hInstance;
	wc.hIcon = hDefaultIcon;
	wc.hCursor = hDefaultCursor;
	wc.hbrBackground = (HBRUSH) (COLOR_BTNFACE + 1);
	return RegisterClassW(&wc);
}

void unregisterWindowClass(void)
{
	if (UnregisterClassW(windowClass, hInstance) == 0)
		logLastError(L"error unregistering uiWindow window class");
}

static int defaultOnClosing(uiWindow *w, void *data)
{
	return 0;
}

static void defaultOnPositionContentSizeChanged(uiWindow *w, void *data)
{
	// do nothing
}

static void defaultOnFocusChanged(uiWindow *w, void *data)
{
	// do nothing
}

static void uiWindowDestroy(uiControl *c)
{
	uiWindow *w = uiWindow(c);

	// first hide ourselves
	ShowWindow(w->hwnd, SW_HIDE);
	// now destroy the child
	if (w->child != NULL) {
		uiControlSetParent(w->child, NULL);
		uiControlDestroy(w->child);
	}
	// now free the menubar, if any
	if (w->menubar != NULL)
		freeMenubar(w->menubar);
	// and finally free ourselves
	windows.erase(w);
	uiWindowsEnsureDestroyWindow(w->hwnd);
	uiFreeControl(uiControl(w));
}

uiWindowsControlDefaultHandle(uiWindow)

uiControl *uiWindowParent(uiControl *c)
{
	return NULL;
}

void uiWindowSetParent(uiControl *c, uiControl *parent)
{
	uiUserBugCannotSetParentOnToplevel("uiWindow");
}

static int uiWindowToplevel(uiControl *c)
{
	return 1;
}

static int uiWindowVisible(uiControl *c)
{
	uiWindow *w = uiWindow(c);

	return w->visible;
}

static void uiWindowShow(uiControl *c)
{
	uiWindow *w = uiWindow(c);

	w->visible = 1;
	// just in case the window's minimum size wasn't recalculated already
	ensureMinimumWindowSize(w);
	if (w->shownOnce) {
		ShowWindow(w->hwnd, SW_SHOW);
		return;
	}
	w->shownOnce = TRUE;
	// make sure the child is the correct size
	uiWindowsControlMinimumSizeChanged(uiWindowsControl(w));
	ShowWindow(w->hwnd, nCmdShow);
	if (UpdateWindow(w->hwnd) == 0)
		logLastError(L"error calling UpdateWindow() after showing uiWindow for the first time");
}

static void uiWindowHide(uiControl *c)
{
	uiWindow *w = uiWindow(c);

	w->visible = 0;
	ShowWindow(w->hwnd, SW_HIDE);
}

// TODO decide whether disabling a window should disable its frame or only its contents
uiWindowsControlDefaultEnabled(uiWindow)
uiWindowsControlDefaultEnable(uiWindow)
uiWindowsControlDefaultDisable(uiWindow)
uiWindowsControlDefaultSyncEnableState(uiWindow)
uiWindowsControlDefaultSetParentHWND(uiWindow)

static void uiWindowMinimumSize(uiWindowsControl *c, int *width, int *height)
{
	uiWindow *w = uiWindow(c);
	int mx, my;

	*width = 0;
	*height = 0;
	if (w->child != NULL)
		uiWindowsControlMinimumSize(uiWindowsControl(w->child), width, height);
	windowMargins(w, &mx, &my);
	*width += 2 * mx;
	*height += 2 * my;
}

static void uiWindowMinimumSizeChanged(uiWindowsControl *c)
{
	uiWindow *w = uiWindow(c);

	if (uiWindowsControlTooSmall(uiWindowsControl(w))) {
		// TODO share the minimum window size calculation with WM_GETMINMAXINFO
		ensureMinimumWindowSize(w);
		return;
	}
	// otherwise we only need to re-layout everything
	windowRelayout(w);
}

static void uiWindowLayoutRect(uiWindowsControl *c, RECT *r)
{
	uiWindow *w = uiWindow(c);

	// the layout rect is the client rect in this case
	uiWindowsEnsureGetClientRect(w->hwnd, r);
}

uiWindowsControlDefaultAssignControlIDZOrder(uiWindow)

uiWindowsControlRelayoutOnChildVisibilityChanged(uiWindow)

char *uiWindowTitle(uiWindow *w)
{
	return uiWindowsWindowText(w->hwnd);
}

void uiWindowSetTitle(uiWindow *w, const char *title)
{
	uiWindowsSetWindowText(w->hwnd, title);
	// don't queue resize; the caption isn't part of what affects layout and sizing of the client area (it'll be ellipsized if too long)
}

// this is used for both fullscreening and centering
// see also https://blogs.msdn.microsoft.com/oldnewthing/20100412-00/?p=14353 and https://blogs.msdn.microsoft.com/oldnewthing/20050505-04/?p=35703
static void windowMonitorRect(HWND hwnd, RECT *r)
{
	HMONITOR monitor;
	MONITORINFO mi;

	monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY);
	ZeroMemory(&mi, sizeof (MONITORINFO));
	mi.cbSize = sizeof (MONITORINFO);
	if (GetMonitorInfoW(monitor, &mi) == 0) {
		logLastError(L"error getting window monitor rect");
		// default to SM_CXSCREEN x SM_CYSCREEN to be safe
		r->left = 0;
		r->top = 0;
		r->right = GetSystemMetrics(SM_CXSCREEN);
		r->bottom = GetSystemMetrics(SM_CYSCREEN);
		return;
	}
	*r = mi.rcMonitor;
}

void uiWindowPosition(uiWindow *w, int *x, int *y)
{
	RECT r;

	uiWindowsEnsureGetWindowRect(w->hwnd, &r);
	*x = r.left;
	*y = r.top;
}

void uiWindowSetPosition(uiWindow *w, int x, int y)
{
	w->changingPosition = TRUE;
	if (SetWindowPos(w->hwnd, NULL, x, y, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_NOZORDER) == 0)
		logLastError(L"error setting window position");
	w->changingPosition = FALSE;
}

void uiWindowOnPositionChanged(uiWindow *w, void (*f)(uiWindow *, void *), void *data)
{
	w->onPositionChanged = f;
	w->onPositionChangedData = data;
}

void uiWindowContentSize(uiWindow *w, int *width, int *height)
{
	RECT r;

	uiWindowsEnsureGetClientRect(w->hwnd, &r);
	*width = r.right - r.left;
	*height = r.bottom - r.top;
}

// TODO decide whether programmatic sizes should be clamped to the control minimum
void uiWindowSetContentSize(uiWindow *w, int width, int height)
{
	w->changingSize = TRUE;
	clientSizeToWindowSize(w->hwnd, &width, &height, w->hasMenubar);
	if (SetWindowPos(w->hwnd, NULL, 0, 0, width, height, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER) == 0)
		logLastError(L"error resizing window");
	w->changingSize = FALSE;
}

int uiWindowFullscreen(uiWindow *w)
{
	return w->fullscreen;
}

void uiWindowSetFullscreen(uiWindow *w, int fullscreen)
{
	RECT r;

	if (w->fullscreen && fullscreen)
		return;
	if (!w->fullscreen && !fullscreen)
		return;
	w->fullscreen = fullscreen;
	w->changingSize = TRUE;
	if (w->fullscreen) {
		ZeroMemory(&(w->fsPrevPlacement), sizeof (WINDOWPLACEMENT));
		w->fsPrevPlacement.length = sizeof (WINDOWPLACEMENT);
		if (GetWindowPlacement(w->hwnd, &(w->fsPrevPlacement)) == 0)
			logLastError(L"error getting old window placement");
		windowMonitorRect(w->hwnd, &r);
		updateWindowStyle(w);
		if (SetWindowPos(w->hwnd, HWND_TOP,
			r.left, r.top,
			r.right - r.left, r.bottom - r.top,
			SWP_FRAMECHANGED | SWP_NOOWNERZORDER) == 0)
			logLastError(L"error making window fullscreen");
	} else {
		updateWindowStyle(w);
		if (SetWindowPlacement(w->hwnd, &(w->fsPrevPlacement)) == 0)
			logLastError(L"error leaving fullscreen");
		if (SetWindowPos(w->hwnd, NULL,
			0, 0, 0, 0,
			SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER) == 0)
			logLastError(L"error restoring window border after fullscreen");
	}
	w->changingSize = FALSE;
}

void uiWindowOnContentSizeChanged(uiWindow *w, void (*f)(uiWindow *, void *), void *data)
{
	w->onContentSizeChanged = f;
	w->onContentSizeChangedData = data;
}

void uiWindowOnClosing(uiWindow *w, int (*f)(uiWindow *, void *), void *data)
{
	w->onClosing = f;
	w->onClosingData = data;
}

void uiWindowOnFocusChanged(uiWindow *w, void (*f)(uiWindow *, void *), void *data)
{
	w->onFocusChanged = f;
	w->onFocusChangedData = data;
}

int uiWindowFocused(uiWindow *w)
{
	return w->focused;
}

int uiWindowBorderless(uiWindow *w)
{
	return w->borderless;
}

// TODO window should move to the old client position and should not have the extra space the borders left behind
void uiWindowSetBorderless(uiWindow *w, int borderless)
{
	w->borderless = borderless;
	updateWindowStyle(w);
	updateFrame(w);
}

void uiWindowSetChild(uiWindow *w, uiControl *child)
{
	if (w->child != NULL) {
		uiControlSetParent(w->child, NULL);
		uiWindowsControlSetParentHWND(uiWindowsControl(w->child), NULL);
	}
	w->child = child;
	if (w->child != NULL) {
		uiControlSetParent(w->child, uiControl(w));
		uiWindowsControlSetParentHWND(uiWindowsControl(w->child), w->hwnd);
		uiWindowsControlAssignSoleControlIDZOrder(uiWindowsControl(w->child));
		windowRelayout(w);
	}
}

int uiWindowMargined(uiWindow *w)
{
	return w->margined;
}

void uiWindowSetMargined(uiWindow *w, int margined)
{
	w->margined = margined;
	windowRelayout(w);
}

int uiWindowResizeable(uiWindow *w)
{
	return w->resizeable;
}

void uiWindowSetResizeable(uiWindow *w, int resizeable)
{
	w->resizeable = resizeable;
	updateWindowStyle(w);
	updateFrame(w);
}

static void setClientSize(uiWindow *w, int width, int height, BOOL hasMenubar)
{
	clientSizeToWindowSize(w->hwnd, &width, &height, hasMenubar);
	if (SetWindowPos(w->hwnd, NULL, 0, 0, width, height, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER) == 0)
		logLastError(L"error resizing window");
}

uiWindow *uiNewWindow(const char *title, int width, int height, int hasMenubar)
{
	uiWindow *w;
	WCHAR *wtitle;
	BOOL hasMenubarBOOL;

	uiWindowsNewControl(uiWindow, w);

	w->resizeable = TRUE;
	hasMenubarBOOL = FALSE;
	if (hasMenubar)
		hasMenubarBOOL = TRUE;
	w->hasMenubar = hasMenubarBOOL;

#define style WS_OVERLAPPEDWINDOW
#define exstyle 0

	wtitle = toUTF16(title);
	w->hwnd = CreateWindowExW(exstyle,
		windowClass, wtitle,
		style,
		CW_USEDEFAULT, CW_USEDEFAULT,
		// use the raw width and height for now
		// this will get CW_USEDEFAULT (hopefully) predicting well
		// even if it doesn't, we're adjusting it later
		width, height,
		NULL, NULL, hInstance, w);
	if (w->hwnd == NULL)
		logLastError(L"error creating window");
	uiprivFree(wtitle);

	if (hasMenubar) {
		w->menubar = makeMenubar();
		if (w->menubar != NULL && SetMenu(w->hwnd, w->menubar) == 0)
			logLastError(L"error giving menu to window");
	}

	// and use the proper size
	setClientSize(w, width, height, hasMenubarBOOL);

	uiWindowOnClosing(w, defaultOnClosing, NULL);
	uiWindowOnContentSizeChanged(w, defaultOnPositionContentSizeChanged, NULL);
	uiWindowOnFocusChanged(w, defaultOnFocusChanged, NULL);
	uiWindowOnPositionChanged(w, defaultOnPositionContentSizeChanged, NULL);

	windows.insert(w);
	return w;
}

// this cannot queue a resize because it's called by the resize handler
void ensureMinimumWindowSize(uiWindow *w)
{
	int width, height;
	RECT r;

	uiWindowsControlMinimumSize(uiWindowsControl(w), &width, &height);
	uiWindowsEnsureGetClientRect(w->hwnd, &r);
	if (width < (r.right - r.left))		// preserve width if larger
		width = r.right - r.left;
	if (height < (r.bottom - r.top))		// preserve height if larger
		height = r.bottom - r.top;
	clientSizeToWindowSize(w->hwnd, &width, &height, w->hasMenubar);
	if (SetWindowPos(w->hwnd, NULL, 0, 0, width, height, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER) == 0)
		logLastError(L"error resizing window");
}

void disableAllWindowsExcept(uiWindow *which)
{
	for (auto &w : windows) {
		if (w == which)
			continue;
		EnableWindow(w->hwnd, FALSE);
	}
}

void enableAllWindowsExcept(uiWindow *which)
{
	for (auto &w : windows) {
		if (w == which)
			continue;
		if (!uiControlEnabled(uiControl(w)))
			continue;
		EnableWindow(w->hwnd, TRUE);
	}
}
