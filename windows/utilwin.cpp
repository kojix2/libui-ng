// 14 may 2015
#include "uipriv_windows.hpp"
#include <cstdlib>

// The utility window is a special window that performs certain tasks internal to libui.
// It is not a message-only window, and it is always hidden and disabled.
// Its roles:
// - It is the initial parent of all controls. When a control loses its parent, it also becomes that control's parent.
// - It handles WM_QUERYENDSESSION requests.
// - It handles WM_WININICHANGE and forwards the message to any child windows that request it.
// - It handles executing functions queued to run by uiQueueMain().
// This cannot be a message-only window because it is also the initial parent
// of ordinary child controls before they are attached elsewhere.

#define utilWindowClass L"libui_utilWindowClass"

HWND utilWindow;

// Timer IDs for uiTimer() are allocated pointers, so 1 is reserved for
// retrying a deferred-destruction notification when PostMessageW() fails.
static const UINT_PTR destroyFlushRetryTimerID = 1;
static uintptr_t destroyFlushRetryID;

#ifdef _UI_STATIC
static BOOL failNextDestroyPostForTests;

extern "C" void uiprivWindowsFailNextDestroyPostForTests(void)
{
	failNextDestroyPostForTests = TRUE;
}

extern "C" void uiprivWindowsDeliverDestroyRetryTimerForTests(void)
{
	SendMessageW(utilWindow, WM_TIMER, destroyFlushRetryTimerID, 0);
}
#endif

static LRESULT CALLBACK utilWindowWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	void (*qf)(void *);
	LRESULT lResult;
	uiprivTimer *timer;
	int repeat;

	if (handleParentMessages(hwnd, uMsg, wParam, lParam, &lResult) != FALSE)
		return lResult;
	switch (uMsg) {
	case WM_QUERYENDSESSION:
		// TODO block handler (TODO figure out if this meant the Vista-style block handler or not)
		if (uiprivShouldQuit()) {
			uiQuit();
			return TRUE;
		}
		return FALSE;
	case WM_WININICHANGE:
		issueWM_WININICHANGE(wParam, lParam);
		return 0;
	case msgQueued:
		qf = (void (*)(void *)) wParam;
		uiprivUserCallbackEnter(NULL);
		(*qf)((void *) lParam);
		uiprivUserCallbackLeave();
		return 0;
	case msgControlDestroyFlush:
		uiprivControlDestroyFlush((uintptr_t) wParam);
		return 0;
	case WM_TIMER:
		if (wParam == destroyFlushRetryTimerID) {
			uintptr_t id = destroyFlushRetryID;
			if (id == 0)
				return 0;
			destroyFlushRetryID = 0;
			if (KillTimer(utilWindow, destroyFlushRetryTimerID) == 0)
				logLastError(L"error stopping deferred destruction retry timer");
			uiprivControlDestroyFlush(id);
			return 0;
		}
		timer = (uiprivTimer *) wParam;
		if (!uiprivTimerBeginCallback(timer))
			return 0;
		uiprivUserCallbackEnter(NULL);
		repeat = (*(timer->f))(timer->data);
		uiprivUserCallbackLeave();
		uiprivTimerEndCallback(timer);
		if (!repeat) {
			if (KillTimer(utilWindow, (UINT_PTR) timer) == 0)
				logLastError(L"error calling KillTimer() to end uiTimer() procedure");
			uiprivFreeTimer(timer);
		}
		return 0;
	}
	return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

void uiprivScheduleControlDestroyFlush(uintptr_t id)
{
	BOOL posted;

#ifdef _UI_STATIC
	if (failNextDestroyPostForTests) {
		failNextDestroyPostForTests = FALSE;
		SetLastError(ERROR_NOT_ENOUGH_QUOTA);
		posted = FALSE;
	} else
#endif
		posted = PostMessageW(utilWindow, msgControlDestroyFlush, (WPARAM) id, 0);
	if (!posted) {
		logLastError(L"error queueing deferred control destruction");
		// WM_TIMER is synthesized by the message loop rather than using a
		// posted-message slot. It can recover from a full posted queue.
		destroyFlushRetryID = id;
		if (SetTimer(utilWindow, destroyFlushRetryTimerID,
			USER_TIMER_MINIMUM, NULL) == 0) {
			logLastError(L"error scheduling deferred destruction retry timer");
			std::abort();
		}
	}
}

void uiprivFlushFailedControlDestroyPost(void)
{
	uintptr_t id = destroyFlushRetryID;

	if (id == 0)
		return;
	if (uiprivControlDestroyFlushAtMainLoopBoundary() &&
		destroyFlushRetryID == id) {
		destroyFlushRetryID = 0;
		if (KillTimer(utilWindow, destroyFlushRetryTimerID) == 0)
			logLastError(L"error stopping deferred destruction retry timer");
	}
}

const char *initUtilWindow(HICON hDefaultIcon, HCURSOR hDefaultCursor)
{
	WNDCLASSW wc;

	ZeroMemory(&wc, sizeof (WNDCLASSW));
	wc.lpszClassName = utilWindowClass;
	wc.lpfnWndProc = utilWindowWndProc;
	wc.hInstance = hInstance;
	wc.hIcon = hDefaultIcon;
	wc.hCursor = hDefaultCursor;
	wc.hbrBackground = (HBRUSH) (COLOR_BTNFACE + 1);
	if (RegisterClass(&wc) == 0)
		// see init.cpp for an explanation of the =s
		return "=registering utility window class";

	utilWindow = CreateWindowExW(0,
		utilWindowClass, L"libui utility window",
		WS_OVERLAPPEDWINDOW,
		0, 0, 100, 100,
		NULL, NULL, hInstance, NULL);
	if (utilWindow == NULL)
		return "=creating utility window";
	// and just to be safe
	EnableWindow(utilWindow, FALSE);

	return NULL;
}

void uninitUtilWindow(void)
{
	if (destroyFlushRetryID != 0) {
		destroyFlushRetryID = 0;
		if (KillTimer(utilWindow, destroyFlushRetryTimerID) == 0)
			logLastError(L"error stopping deferred destruction retry timer");
	}
	if (DestroyWindow(utilWindow) == 0)
		logLastError(L"error destroying utility window");
	if (UnregisterClass(utilWindowClass, hInstance) == 0)
		logLastError(L"error unregistering utility window class");
}
