#include "uipriv_windows.hpp"
#include "../common/toolbar.h"

static DWORD nativeToolbarStyle(uiToolbarDisplayMode mode)
{
	return mode == uiToolbarDisplayModeIconAndTextHorizontal ? TBSTYLE_LIST : 0;
}

static DWORD nativeToolbarExtendedStyle(uiToolbarDisplayMode mode)
{
	DWORD nativeStyle = TBSTYLE_EX_DOUBLEBUFFER;

	if (mode == uiToolbarDisplayModeIconAndTextHorizontal)
		nativeStyle |= TBSTYLE_EX_MIXEDBUTTONS;
	return nativeStyle;
}

static BYTE nativeButtonStyle(uiToolbarItem *item)
{
	BYTE nativeStyle = BTNS_AUTOSIZE |
		(item->type == uiprivToolbarItemToggleButton ? BTNS_CHECK : BTNS_BUTTON);

	if (item->toolbar->displayMode == uiToolbarDisplayModeIconAndTextHorizontal)
		nativeStyle |= BTNS_SHOWTEXT;
	return nativeStyle;
}

static uiToolbarItem *itemForCommand(uiToolbar *t, UINT command)
{
	for (size_t i = 0; i < t->len; i++)
		if (t->items[i]->nativeID == command)
			return t->items[i];
	return NULL;
}

static void destroyNative(uiToolbar *t)
{
	if (t->native != NULL) {
		uiWindowsEnsureDestroyWindow((HWND) t->native);
		t->native = NULL;
	}
	if (t->nativeAux != NULL) {
		ImageList_Destroy((HIMAGELIST) t->nativeAux);
		t->nativeAux = NULL;
	}
	for (size_t i = 0; i < t->len; i++) {
		t->items[i]->native = NULL;
		t->items[i]->nativeID = 0;
		t->items[i]->nativeImage = I_IMAGENONE;
	}
}

static HBITMAP itemBitmap(uiToolbarItem *item, HDC dc, int iconSize)
{
	IWICBitmap *wic;
	HBITMAP bitmap = NULL;

	if (item->icon == NULL || (item->type != uiprivToolbarItemButton &&
		item->type != uiprivToolbarItemToggleButton))
		return NULL;
	wic = uiprivImageAppropriateForDC(item->icon, dc);
	if (wic != NULL && uiprivWICToGDI(wic, dc, iconSize, iconSize, &bitmap) != S_OK)
		bitmap = NULL;
	return bitmap;
}

static int buildImages(uiToolbar *t)
{
	HWND hwnd = (HWND) t->native;
	HIMAGELIST images;
	HIMAGELIST oldImages;
	HDC dc;
	int iconSize;

	if (hwnd == NULL)
		return 0;
	dc = GetDC(hwnd);
	if (dc == NULL) {
		logLastError(L"error getting toolbar DC");
		return 0;
	}
	iconSize = MulDiv(16, GetDeviceCaps(dc, LOGPIXELSX), 96);
	if (iconSize < 1)
		iconSize = 16;
	images = ImageList_Create(iconSize, iconSize, ILC_COLOR32,
		t->len == 0 ? 1 : (int) t->len, 1);
	if (images == NULL) {
		logLastError(L"error creating toolbar image list");
		ReleaseDC(hwnd, dc);
		return 0;
	}
	// Attach the image list before adding buttons so BTNS_AUTOSIZE accounts
	// for both the final image and text.
	oldImages = (HIMAGELIST) SendMessageW(hwnd, TB_SETIMAGELIST, 0,
		(LPARAM) images);
	t->nativeAux = images;
	for (size_t i = 0; i < t->len; i++) {
		uiToolbarItem *item = t->items[i];
		HBITMAP bitmap = itemBitmap(item, dc, iconSize);
		item->nativeImage = I_IMAGENONE;
		if (bitmap != NULL) {
			int imageIndex = ImageList_Add(images, bitmap, NULL);
			DeleteObject(bitmap);
			if (imageIndex == -1) {
				logLastError(L"error adding toolbar image");
			} else
				item->nativeImage = imageIndex;
		}
	}
	ReleaseDC(hwnd, dc);
	if (oldImages != NULL)
		ImageList_Destroy(oldImages);
	return 1;
}

static void updateImage(uiToolbarItem *item)
{
	uiToolbar *t = item->toolbar;
	HWND hwnd = (HWND) t->native;
	HIMAGELIST images = (HIMAGELIST) t->nativeAux;
	HDC dc;
	HBITMAP bitmap;
	TBBUTTONINFOW info;
	int iconSize;
	int imageIndex = I_IMAGENONE;

	if (hwnd == NULL || images == NULL || item->nativeID == 0)
		return;
	dc = GetDC(hwnd);
	if (dc == NULL) {
		logLastError(L"error getting toolbar DC");
		return;
	}
	iconSize = MulDiv(16, GetDeviceCaps(dc, LOGPIXELSX), 96);
	if (iconSize < 1)
		iconSize = 16;
	bitmap = NULL;
	if (t->displayMode != uiToolbarDisplayModeTextOnly)
		bitmap = itemBitmap(item, dc, iconSize);
	if (bitmap != NULL) {
		if (item->nativeImage == I_IMAGENONE) {
			item->nativeImage = ImageList_Add(images, bitmap, NULL);
			if (item->nativeImage == -1) {
				logLastError(L"error adding toolbar image");
				item->nativeImage = I_IMAGENONE;
			}
		} else if (ImageList_Replace(images, item->nativeImage, bitmap, NULL) == FALSE) {
			logLastError(L"error replacing toolbar image");
			item->nativeImage = I_IMAGENONE;
		}
		DeleteObject(bitmap);
		if (item->nativeImage != I_IMAGENONE)
			imageIndex = item->nativeImage;
	}
	ReleaseDC(hwnd, dc);

	ZeroMemory(&info, sizeof info);
	info.cbSize = sizeof info;
	info.dwMask = TBIF_IMAGE;
	info.iImage = imageIndex;
	SendMessageW(hwnd, TB_SETBUTTONINFOW, item->nativeID, (LPARAM) &info);
}

static void syncText(uiToolbarItem *item)
{
	HWND hwnd = (HWND) item->toolbar->native;
	TBBUTTONINFOW info;
	WCHAR *text;

	if (hwnd == NULL || item->nativeID == 0)
		return;
	text = toUTF16(item->text);
	ZeroMemory(&info, sizeof info);
	info.cbSize = sizeof info;
	info.dwMask = TBIF_TEXT;
	info.pszText = item->toolbar->displayMode == uiToolbarDisplayModeIconOnly &&
		item->icon != NULL ? (WCHAR *) L"" : text;
	SendMessageW(hwnd, TB_SETBUTTONINFOW, item->nativeID, (LPARAM) &info);
	uiprivFree(text);
}

void uiprivToolbarPlatformNew(uiToolbar *t)
{
	/* Native objects are created while attaching to their parent HWND. */
}

void uiprivToolbarPlatformFree(uiToolbar *t)
{
	destroyNative(t);
}

static int createNative(uiToolbar *t, uiWindow *w)
{
	HWND parent = (HWND) uiControlHandle(uiControl(w));
	HWND old = (HWND) t->native;
	void *oldImages = t->nativeAux;
	HWND hwnd;
	UINT commandID = 1;
	size_t commandItems = 0;

	for (size_t i = 0; i < t->len; i++)
		if (t->items[i]->type == uiprivToolbarItemButton ||
			t->items[i]->type == uiprivToolbarItemToggleButton)
			commandItems++;
	if (commandItems > 0xFFFF)
		uiprivUserBug("You cannot create a uiToolbar with more than 65535 command items on Windows.");

	hwnd = CreateWindowExW(0, TOOLBARCLASSNAMEW, NULL,
		WS_CHILD | TBSTYLE_FLAT | TBSTYLE_TRANSPARENT |
		TBSTYLE_TOOLTIPS | nativeToolbarStyle(t->displayMode) | CCS_NODIVIDER |
		CCS_NORESIZE | CCS_NOPARENTALIGN,
		0, 0, 100, 30, parent, NULL, hInstance, NULL);
	if (hwnd == NULL) {
		logLastError(L"error creating toolbar");
		return 0;
	}
	t->native = hwnd;
	t->nativeAux = NULL;
	SendMessageW(hwnd, WM_SETFONT, (WPARAM) hMessageFont, TRUE);
	SendMessageW(hwnd, TB_BUTTONSTRUCTSIZE, sizeof (TBBUTTON), 0);
	SendMessageW(hwnd, TB_SETEXTENDEDSTYLE, 0,
		nativeToolbarExtendedStyle(t->displayMode));
	if (!buildImages(t))
		goto fail;
	for (size_t i = 0; i < t->len; i++) {
		uiToolbarItem *item = t->items[i];
		TBBUTTON button;
		WCHAR *text = toUTF16(item->text);

		ZeroMemory(&button, sizeof button);
		button.iBitmap = t->displayMode == uiToolbarDisplayModeTextOnly ?
			I_IMAGENONE : item->nativeImage;
		button.fsState = item->enabled ? TBSTATE_ENABLED : 0;
		button.dwData = (DWORD_PTR) item;
		switch (item->type) {
		case uiprivToolbarItemButton:
		case uiprivToolbarItemToggleButton:
			// WM_COMMAND identifies the sending control in lParam, so these IDs
			// only need to be unique within this toolbar.
			item->nativeID = commandID++;
			button.idCommand = (int) item->nativeID;
			button.fsStyle = nativeButtonStyle(item);
			button.iString = t->displayMode == uiToolbarDisplayModeIconOnly &&
				item->icon != NULL ? (INT_PTR) -1 : (INT_PTR) text;
			if (item->checked)
				button.fsState |= TBSTATE_CHECKED;
			break;
		case uiprivToolbarItemSeparator:
			button.fsStyle = BTNS_SEP;
			button.iBitmap = 6;
			break;
		}
		if (SendMessageW(hwnd, TB_ADDBUTTONSW, 1, (LPARAM) &button) == FALSE) {
			logLastError(L"error adding toolbar item");
			uiprivFree(text);
			goto fail;
		}
		uiprivFree(text);
		item->native = (void *) (UINT_PTR) i;
	}
	SendMessageW(hwnd, TB_AUTOSIZE, 0, 0);
	return 1;

fail:
	destroyNative(t);
	t->native = old;
	t->nativeAux = oldImages;
	if (old != NULL) {
		for (size_t i = 0; i < t->len; i++) {
			TBBUTTON button;
			if (SendMessageW(old, TB_GETBUTTON, i, (LPARAM) &button)) {
				t->items[i]->native = (void *) (UINT_PTR) i;
				t->items[i]->nativeID = button.idCommand;
				t->items[i]->nativeImage = button.iBitmap;
			}
		}
	}
	return 0;
}

void uiprivToolbarPlatformAttach(uiToolbar *t, uiWindow *w)
{
	if (createNative(t, w))
		ShowWindow((HWND) t->native, SW_SHOWNA);
}

int uiprivToolbarPlatformSetDisplayMode(uiToolbar *t, uiToolbarDisplayMode previous)
{
	HWND old = (HWND) t->native;
	HIMAGELIST oldImages = (HIMAGELIST) t->nativeAux;
	BOOL focused;
	BOOL visible;
	BOOL enabled;
	LRESULT hotItem;

	if (t->window == NULL)
		return 1;
	if (old != NULL && t->displayMode == previous)
		return 1;
	focused = old != NULL && GetFocus() == old;
	visible = old == NULL || (GetWindowLongPtrW(old, GWL_STYLE) & WS_VISIBLE) != 0;
	enabled = old == NULL || IsWindowEnabled(old);
	hotItem = old == NULL ? -1 : SendMessageW(old, TB_GETHOTITEM, 0, 0);
	// Build the replacement while hidden. Keep the old control alive until
	// creation succeeds; the public toolbar and item objects never change.
	if (!createNative(t, t->window))
		return 0;
	if (old != NULL)
		uiWindowsEnsureDestroyWindow(old);
	if (oldImages != NULL)
		ImageList_Destroy(oldImages);
	EnableWindow((HWND) t->native, enabled);
	SendMessageW((HWND) t->native, TB_SETHOTITEM, hotItem, 0);
	uiWindowsControlMinimumSizeChanged(uiWindowsControl(t->window));
	if (visible)
		ShowWindow((HWND) t->native, SW_SHOWNA);
	if (focused)
		SetFocus((HWND) t->native);
	return 1;
}

void uiprivToolbarPlatformDetach(uiToolbar *t, uiWindow *w)
{
	destroyNative(t);
}

void uiprivToolbarPlatformSyncItem(uiToolbarItem *item, uiprivToolbarItemProperty property)
{
	HWND hwnd = (HWND) item->toolbar->native;
	if (hwnd == NULL || item->nativeID == 0)
		return;
	switch (property) {
	case uiprivToolbarItemText:
		syncText(item);
		uiWindowsControlMinimumSizeChanged(uiWindowsControl(item->toolbar->window));
		break;
	case uiprivToolbarItemEnabled:
		SendMessageW(hwnd, TB_ENABLEBUTTON, item->nativeID, MAKELONG(item->enabled, 0));
		break;
	case uiprivToolbarItemChecked:
		SendMessageW(hwnd, TB_CHECKBUTTON, item->nativeID, MAKELONG(item->checked, 0));
		break;
	case uiprivToolbarItemTooltip:
		// TBN_GETINFOTIPW reads the current tooltip directly from the item.
		break;
	}
}

void uiprivToolbarPlatformSyncItemIcon(uiToolbarItem *item)
{
	updateImage(item);
	if (item->toolbar->displayMode == uiToolbarDisplayModeIconOnly)
		syncText(item);
	if (item->toolbar->window != NULL)
		uiWindowsControlMinimumSizeChanged(uiWindowsControl(item->toolbar->window));
}

char *uiprivToolbarPlatformDupText(const char *text)
{
	WCHAR *wide = toUTF16(text);
	char *copy = toUTF8(wide);
	uiprivFree(wide);
	return copy;
}

BOOL uiprivToolbarWindowsCommand(uiToolbar *t, UINT command)
{
	uiToolbarItem *item = itemForCommand(t, command);
	int checked = 0;

	if (item == NULL)
		return FALSE;
	if (item->type == uiprivToolbarItemToggleButton)
		checked = SendMessageW((HWND) t->native, TB_ISBUTTONCHECKED,
			item->nativeID, 0) != 0;
	uiprivToolbarItemClicked(item, checked);
	return TRUE;
}

BOOL uiprivToolbarWindowsNotify(uiToolbar *t, NMHDR *hdr, LRESULT *result)
{
	if (hdr->code == TBN_GETINFOTIPW) {
		NMTBGETINFOTIPW *tip = (NMTBGETINFOTIPW *) hdr;
		uiToolbarItem *item = itemForCommand(t, tip->iItem);
		if (item != NULL) {
			WCHAR *text = toUTF16(item->tooltip);
			lstrcpynW(tip->pszText, text, tip->cchTextMax);
			uiprivFree(text);
			*result = 0;
			return TRUE;
		}
	}
	return FALSE;
}

int uiprivToolbarWindowsHeight(uiToolbar *t)
{
	// Minimum-size queries must not resize the live toolbar. Only the layout
	// path may apply the measured height to the native window.
	HWND hwnd = (HWND) t->native;
	int toolbarHeight = 0;

	if (hwnd == NULL)
		return 0;
	for (size_t i = 0; i < t->len; i++) {
		RECT r;
		if (SendMessageW(hwnd, TB_GETITEMRECT, i, (LPARAM) &r) != 0)
			if (r.bottom > toolbarHeight)
				toolbarHeight = r.bottom;
	}
	if (toolbarHeight == 0) {
		RECT r;
		GetWindowRect(hwnd, &r);
		toolbarHeight = r.bottom - r.top;
	}
	return toolbarHeight;
}

int uiprivToolbarWindowsLayout(uiToolbar *t, int width)
{
	if (t->native == NULL)
		return 0;
	int toolbarHeight = uiprivToolbarWindowsHeight(t);
	uiWindowsEnsureMoveWindowDuringResize((HWND) t->native, 0, 0, width, toolbarHeight);
	return toolbarHeight;
}
