// 16 august 2015
#include "uipriv_windows.hpp"

// choose a value distinct from uiWindowSignature
#define uiWindowsControlSignature 0x4D53576E

static int isContainerControl(uiControl *c)
{
	switch (c->TypeSignature) {
	case uiBoxSignature:
	case uiFormSignature:
	case uiGridSignature:
	case uiGroupSignature:
	case uiTabSignature:
		return 1;
	}
	return 0;
}

void uiWindowsControlSyncEnableState(uiWindowsControl *c, int enabled)
{
	(*(c->SyncEnableState))(c, enabled);
}

void uiWindowsControlSetParentHWND(uiWindowsControl *c, HWND parent)
{
	(*(c->SetParentHWND))(c, parent);
}

void uiWindowsControlMinimumSize(uiWindowsControl *c, int *width, int *height)
{
	(*(c->MinimumSize))(c, width, height);
	// Preserve a container's structural child minimum.
	uiprivControlMinimumSizeApply(uiControl(c), width, height,
		isContainerControl(uiControl(c)));
}

static void notifyMinimumSizeChanged(uiWindowsControl *c)
{
	uiControl *parent;

	// Relayout from the root so smaller minima redistribute existing space.
	parent = uiControlParent(uiControl(c));
	if (parent != NULL)
		notifyMinimumSizeChanged(uiWindowsControl(parent));
	(*(c->MinimumSizeChanged))(c);
}

void uiWindowsControlMinimumSizeChanged(uiWindowsControl *c)
{
	notifyMinimumSizeChanged(c);
}

// TODO get rid of this
void uiWindowsControlLayoutRect(uiWindowsControl *c, RECT *r)
{
	(*(c->LayoutRect))(c, r);
}

void uiWindowsControlAssignControlIDZOrder(uiWindowsControl *c, LONG_PTR *controlID, HWND *insertAfter)
{
	(*(c->AssignControlIDZOrder))(c, controlID, insertAfter);
}

void uiWindowsControlChildVisibilityChanged(uiWindowsControl *c)
{
	(*(c->ChildVisibilityChanged))(c);
}

HWND uiWindowsEnsureCreateControlHWND(DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle, HINSTANCE instance, LPVOID lpParam, BOOL useStandardControlFont)
{
	HWND hwnd;

	// don't let using the arrow keys in a uiRadioButtons leave the radio buttons
	if ((dwStyle & WS_TABSTOP) != 0)
		dwStyle |= WS_GROUP;
	hwnd = CreateWindowExW(dwExStyle,
		lpClassName, lpWindowName,
		dwStyle | WS_CHILD | WS_VISIBLE,
		0, 0,
		// use a nonzero initial size just in case some control breaks with a zero initial size
		100, 100,
		utilWindow, NULL, instance, lpParam);
	if (hwnd == NULL) {
		logLastError(L"error creating window");
		// TODO return a decoy window
	}
	if (useStandardControlFont)
		SendMessageW(hwnd, WM_SETFONT, (WPARAM) hMessageFont, (LPARAM) TRUE);
	return hwnd;
}

uiWindowsControl *uiWindowsAllocControl(size_t n, uint32_t typesig, const char *typenamestr)
{
	return uiWindowsControl(uiAllocControl(n, uiWindowsControlSignature, typesig, typenamestr));
}

void uiprivControlMinimumSizeChanged(uiControl *c)
{
	if (c->OSSignature != uiWindowsControlSignature)
		return;
	uiWindowsControlMinimumSizeChanged(uiWindowsControl(c));
}

void uiprivControlMinimumSizeDestroyed(uiControl *c)
{
}

BOOL uiWindowsShouldStopSyncEnableState(uiWindowsControl *c, BOOL enabled)
{
	int ce;

	ce = uiControlEnabled(uiControl(c));
	// only stop if we're going from disabled back to enabled; don't stop under any other condition
	// (if we stop when going from enabled to disabled then enabled children of a disabled control won't get disabled at the OS level)
	if (!ce && enabled)
		return TRUE;
	return FALSE;
}

void uiWindowsControlAssignSoleControlIDZOrder(uiWindowsControl *c)
{
	LONG_PTR controlID;
	HWND insertAfter;

	controlID = 100;
	insertAfter = NULL;
	uiWindowsControlAssignControlIDZOrder(c, &controlID, &insertAfter);
}

BOOL uiWindowsControlTooSmall(uiWindowsControl *c)
{
	RECT r;
	int width, height;

	uiWindowsControlLayoutRect(c, &r);
	uiWindowsControlMinimumSize(c, &width, &height);
	if ((r.right - r.left) < width)
		return TRUE;
	if ((r.bottom - r.top) < height)
		return TRUE;
	return FALSE;
}

void uiWindowsControlNotifyVisibilityChanged(uiWindowsControl *c)
{
	uiControl *parent;

	parent = uiControlParent(uiControl(c));
	if (parent != NULL)
		uiWindowsControlChildVisibilityChanged(uiWindowsControl(parent));
}
