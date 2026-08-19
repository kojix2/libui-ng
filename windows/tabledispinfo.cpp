// 13 june 2018
#include "uipriv_windows.hpp"
#include "table.hpp"
#include <strsafe.h>

// further reading:
// - https://msdn.microsoft.com/en-us/library/ye4z8x58.aspx

static void copyDispInfoText(NMLVDISPINFOW *nm, const WCHAR *text)
{
	HRESULT hr;
	size_t len;

	if (nm->item.cchTextMax <= 0)
		return;
	hr = StringCchCopyW(nm->item.pszText, nm->item.cchTextMax, text);
	if (hr != STRSAFE_E_INSUFFICIENT_BUFFER)
		return;

	// StringCchCopyW truncates by UTF-16 code unit. Do not leave an
	// unmatched high surrogate at the end of the list view's buffer.
	len = wcslen(nm->item.pszText);
	if (len != 0 && IS_HIGH_SURROGATE(nm->item.pszText[len - 1]))
		nm->item.pszText[len - 1] = L'\0';
}

static HRESULT handleLVIF_TEXT(uiTable *t, NMLVDISPINFOW *nm, uiprivTableColumnParams *p)
{
	int strcol;
	uiTableValue *value;
	WCHAR *wstr;
	int progress;

	if ((nm->item.mask & LVIF_TEXT) == 0)
		return S_OK;

	strcol = -1;
	if (p->textModelColumn != -1)
		strcol = p->textModelColumn;
	else if (p->buttonModelColumn != -1)
		strcol = p->buttonModelColumn;
	if (strcol != -1) {
		value = uiprivTableModelCellValue(t->model, nm->item.iItem, strcol);
		wstr = toUTF16(uiTableValueString(value));
		uiFreeTableValue(value);
		// Copy into the list view's buffer so ownership remains with the
		// control; copyDispInfoText() keeps truncation valid UTF-16.
		copyDispInfoText(nm, wstr);
		uiprivFree(wstr);
		return S_OK;
	}

	if (p->progressBarModelColumn != -1) {
		progress = uiprivTableProgress(t, nm->item.iItem, nm->item.iSubItem, p->progressBarModelColumn, NULL);

		if (progress == -1) {
			// TODO either localize this or replace it with something that's language-neutral
			copyDispInfoText(nm, L"Indeterminate");
			return S_OK;
		}
		if (nm->item.cchTextMax > 0)
			StringCchPrintfW(nm->item.pszText, nm->item.cchTextMax, L"%d%%", progress);
		return S_OK;
	}

	return S_OK;
}

static HRESULT handleLVIF_IMAGE(uiTable *t, NMLVDISPINFOW *nm, uiprivTableColumnParams *p)
{
	if (nm->item.iSubItem == 0 && p->imageModelColumn == -1 && p->checkboxModelColumn == -1) {
		// Having an image list always leaves space for an image
		// on the main item :|
		// Other places on the internet imply that you should be
		// able to do this but that it shouldn't work, but it works
		// perfectly (and pixel-perfectly too) for me, so...
		nm->item.mask |= LVIF_INDENT;
		nm->item.iIndent = -1;
	}
	if ((nm->item.mask & LVIF_IMAGE) == 0)
		return S_OK;		// nothing to do here

	// Checkboxes can appear in any subitem and are custom-drawn, whereas
	// state images belong to the item as a whole.
	nm->item.iImage = I_IMAGENONE;
	if (p->imageModelColumn != -1 || p->checkboxModelColumn != -1)
		nm->item.iImage = 0;
	return S_OK;
}

HRESULT uiprivTableHandleLVN_GETDISPINFO(uiTable *t, NMLVDISPINFOW *nm, LRESULT *lResult)
{
	uiprivTableColumnParams *p;
	HRESULT hr;

	p = (*(t->columns))[nm->item.iSubItem];
	hr = handleLVIF_TEXT(t, nm, p);
	if (hr != S_OK)
		return hr;
	hr = handleLVIF_IMAGE(t, nm, p);
	if (hr != S_OK)
		return hr;
	*lResult = 0;
	return S_OK;
}
