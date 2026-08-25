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

static void buildImages(uiToolbar *t)
{
	HWND hwnd = (HWND) t->native;
	HIMAGELIST images;
	HIMAGELIST oldImages;
	HDC dc;
	int iconSize;

	if (hwnd == NULL)
		return;
	dc = GetDC(hwnd);
	if (dc == NULL) {
		logLastError(L"error getting toolbar DC");
		return;
	}
	iconSize = MulDiv(16, GetDeviceCaps(dc, LOGPIXELSX), 96);
	if (iconSize < 1)
		iconSize = 16;
	images = ImageList_Create(iconSize, iconSize, ILC_COLOR32,
		t->len == 0 ? 1 : (int) t->len, 1);
	if (images == NULL) {
		logLastError(L"error creating toolbar image list");
		ReleaseDC(hwnd, dc);
		return;
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

static void syncItem(uiToolbarItem *item)
{
	HWND hwnd = (HWND) item->toolbar->native;
	TBBUTTONINFOW info;
	WCHAR *text;

	if (hwnd == NULL || item->nativeID == 0)
		return;
	text = toUTF16(item->text);
	ZeroMemory(&info, sizeof info);
	info.cbSize = sizeof info;
	info.dwMask = TBIF_TEXT | TBIF_STATE;
	info.pszText = item->toolbar->displayMode == uiToolbarDisplayModeIconOnly &&
		item->icon != NULL ? (WCHAR *) L"" : text;
	info.fsState = item->enabled ? TBSTATE_ENABLED : 0;
	if (item->type == uiprivToolbarItemToggleButton && item->checked)
		info.fsState |= TBSTATE_CHECKED;
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

void uiprivToolbarPlatformAttach(uiToolbar *t, uiWindow *w)
{
	HWND parent = (HWND) uiControlHandle(uiControl(w));
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
		WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_TRANSPARENT |
		TBSTYLE_TOOLTIPS | nativeToolbarStyle(t->displayMode) | CCS_NODIVIDER |
		CCS_NORESIZE | CCS_NOPARENTALIGN,
		0, 0, 100, 30, parent, NULL, hInstance, NULL);
	if (hwnd == NULL) {
		logLastError(L"error creating toolbar");
		return;
	}
	t->native = hwnd;
	SendMessageW(hwnd, WM_SETFONT, (WPARAM) hMessageFont, TRUE);
	SendMessageW(hwnd, TB_BUTTONSTRUCTSIZE, sizeof (TBBUTTON), 0);
	SendMessageW(hwnd, TB_SETEXTENDEDSTYLE, 0,
		nativeToolbarExtendedStyle(t->displayMode));
	buildImages(t);
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
		case uiprivToolbarItemSpace:
			button.fsStyle = BTNS_SEP;
			button.iBitmap = 8;
			break;
		case uiprivToolbarItemFlexibleSpace:
			button.fsStyle = BTNS_SEP;
			button.iBitmap = 0;
			break;
		}
		if (SendMessageW(hwnd, TB_ADDBUTTONSW, 1, (LPARAM) &button) == FALSE)
			logLastError(L"error adding toolbar item");
		uiprivFree(text);
		item->native = (void *) (UINT_PTR) i;
	}
	SendMessageW(hwnd, TB_AUTOSIZE, 0, 0);
}

void uiprivToolbarPlatformDetach(uiToolbar *t, uiWindow *w)
{
	destroyNative(t);
}

void uiprivToolbarPlatformSyncItem(uiToolbarItem *item)
{
	syncItem(item);
}

void uiprivToolbarPlatformSyncItemIcon(uiToolbarItem *item)
{
	updateImage(item);
	syncItem(item);
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

int uiprivToolbarWindowsLayout(uiToolbar *t, int width)
{
	HWND hwnd = (HWND) t->native;
	int fixedWidth = 0;
	int flexCount = 0;
	int toolbarHeight = 0;

	if (hwnd == NULL)
		return 0;
	for (size_t i = 0; i < t->len; i++) {
		RECT r;
		if (SendMessageW(hwnd, TB_GETITEMRECT, i, (LPARAM) &r) != 0) {
			if (t->items[i]->type == uiprivToolbarItemFlexibleSpace)
				flexCount++;
			else
				fixedWidth += r.right - r.left;
			if (r.bottom > toolbarHeight)
				toolbarHeight = r.bottom;
		}
	}
	if (flexCount != 0) {
		int flexWidth = (width - fixedWidth) / flexCount;
		if (flexWidth < 0)
			flexWidth = 0;
		if (flexWidth > 0xFFFF)
			flexWidth = 0xFFFF;
		for (size_t i = 0; i < t->len; i++)
			if (t->items[i]->type == uiprivToolbarItemFlexibleSpace) {
				TBBUTTONINFOW info;
				ZeroMemory(&info, sizeof info);
				info.cbSize = sizeof info;
				info.dwMask = TBIF_SIZE | TBIF_BYINDEX;
				info.cx = (WORD) flexWidth;
				SendMessageW(hwnd, TB_SETBUTTONINFOW, i, (LPARAM) &info);
			}
		SendMessageW(hwnd, TB_AUTOSIZE, 0, 0);
	}
	if (toolbarHeight == 0) {
		RECT r;
		GetWindowRect(hwnd, &r);
		toolbarHeight = r.bottom - r.top;
	}
	uiWindowsEnsureMoveWindowDuringResize(hwnd, 0, 0, width, toolbarHeight);
	return toolbarHeight;
}
