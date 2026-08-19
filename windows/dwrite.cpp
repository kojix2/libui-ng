// 14 april 2016
#include "uipriv_windows.hpp"
#include "attrstr.hpp"

IDWriteFactory *dwfactory = NULL;

// Initialize the DirectWrite factory used by text layout and font handling.
HRESULT uiprivInitDrawText(void)
{
	// A shared factory reuses DirectWrite caches and is recommended for normal
	// in-process application components.
	return DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
		__uuidof (IDWriteFactory),
		(IUnknown **) (&dwfactory));
}

void uiprivUninitDrawText(void)
{
	dwfactory->Release();
}

fontCollection *uiprivLoadFontCollection(void)
{
	fontCollection *fc;
	HRESULT hr;

	fc = uiprivNew(fontCollection);
	// always get the latest available font information
	hr = dwfactory->GetSystemFontCollection(&(fc->fonts), TRUE);
	if (hr != S_OK) {
		logHRESULT(L"error getting system font collection", hr);
		uiprivFree(fc);
		return NULL;
	}
	fc->userLocaleSuccess = GetUserDefaultLocaleName(fc->userLocale, LOCALE_NAME_MAX_LENGTH);
	return fc;
}

void uiprivFontCollectionFree(fontCollection *fc)
{
	if (fc == NULL)
		return;
	fc->fonts->Release();
	uiprivFree(fc);
}

WCHAR *uiprivFontCollectionFamilyName(fontCollection *fc, IDWriteFontFamily *family)
{
	IDWriteLocalizedStrings *names = NULL;
	WCHAR *str;
	HRESULT hr;

	if (family == NULL)
		return emptyUTF16();

	hr = family->GetFamilyNames(&names);
	if (hr != S_OK) {
		logHRESULT(L"error getting names of font out", hr);
		return emptyUTF16();
	}
	str = uiprivFontCollectionCorrectString(fc, names);
	names->Release();
	return str;
}

WCHAR *uiprivFontCollectionCorrectString(fontCollection *fc, IDWriteLocalizedStrings *names)
{
	UINT32 index;
	BOOL exists;
	UINT32 length;
	WCHAR *wname;
	HRESULT hr;

	if (names == NULL)
		return emptyUTF16();

	// If locale lookup fails, use the first localized name as the fallback.
	index = 0;

	// this is complex, but we ignore failure conditions to allow fallbacks
	// 1) If the user locale name was successfully retrieved, try it
	// 2) If the user locale name was not successfully retrieved, or that locale's string does not exist, or an error occurred, try L"en-us", the US English locale
	// 3) And if that fails, assume the first one
	// This algorithm is straight from MSDN: https://msdn.microsoft.com/en-us/library/windows/desktop/dd368214%28v=vs.85%29.aspx
	// For step 2 to work, start by setting hr to S_OK and exists to FALSE.
	hr = S_OK;
	exists = FALSE;
	if (fc != NULL && fc->userLocaleSuccess != 0)
		hr = names->FindLocaleName(fc->userLocale, &index, &exists);
	// Retry with US English if the user-locale lookup fails or has no match.
	if (hr != S_OK || !exists)
		hr = names->FindLocaleName(L"en-us", &index, &exists);
	// FindLocaleName() initializes exists to FALSE even when it fails.
	if (!exists)
		index = 0;

	hr = names->GetStringLength(index, &length);
	if (hr != S_OK) {
		logHRESULT(L"error getting length of font name", hr);
		return emptyUTF16();
	}
	// GetStringLength() does not include the null terminator, but GetString() does
	wname = (WCHAR *) uiprivAlloc((length + 1) * sizeof (WCHAR), "WCHAR[]");
	hr = names->GetString(index, wname, length + 1);
	if (hr != S_OK) {
		logHRESULT(L"error getting font name", hr);
		uiprivFree(wname);
		return emptyUTF16();
	}

	return wname;
}
