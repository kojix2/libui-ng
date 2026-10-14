// 22 may 2015
#include "uipriv_windows.hpp"

// File and message dialogs are application-modal: disable the other libui
// windows while they are open.
// TODO apply the same application-modal behavior to font and color dialogs

// notes:
// - FOS_SUPPORTSTREAMABLEITEMS doesn't seem to be supported on windows vista, or at least not with the flags we use
// - even with FOS_NOVALIDATE the dialogs will reject invalid filenames (at least on Vista, anyway)
// - lack of FOS_NOREADONLYRETURN doesn't seem to matter on Windows 7

// TODO determine why an active dialog stops tab navigation in other windows

#define windowHWND(w) (w ? (HWND)uiControlHandle(uiControl(w)) : NULL)

static uiWindow *dialogParent(uiWindow *parent)
{
	if (parent != NULL)
		return parent;
	// Let Windows restore activation when a parentless dialog was opened
	// from a foreground libui window.
	return activeWindow();
}

static HRESULT configureItemDialog(IFileDialog *d, FILEOPENDIALOGOPTIONS optsadd)
{
	FILEOPENDIALOGOPTIONS opts;
	HRESULT hr;

	hr = d->GetOptions(&opts);
	if (hr != S_OK)
		return hr;
	opts |= optsadd;
	// the other platforms don't check read-only; we won't either
	opts &= ~(FOS_NOREADONLYRETURN | FOS_ALLNONSTORAGEITEMS);
	// GetDisplayName(SIGDN_FILESYSPATH) requires a filesystem item.
	opts |= FOS_FORCEFILESYSTEM;
	return d->SetOptions(opts);
}

char *commonItemDialog(HWND parent, REFCLSID clsid, REFIID iid, FILEOPENDIALOGOPTIONS optsadd)
{
	IFileDialog *d = NULL;
	IShellItem *result = NULL;
	WCHAR *wname = NULL;
	char *name = NULL;
	HRESULT hr;

	hr = CoCreateInstance(clsid,
		NULL, CLSCTX_INPROC_SERVER,
		iid, (LPVOID *) (&d));
	if (hr != S_OK) {
		logHRESULT(L"error creating common item dialog", hr);
		// always return NULL on error
		goto out;
	}
	hr = configureItemDialog(d, optsadd);
	if (hr != S_OK) {
		logHRESULT(L"error configuring dialog options", hr);
		goto out;
	}
	hr = d->Show(parent);
	if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED))
		// cancelled; return NULL like we have ready
		goto out;
	if (hr != S_OK) {
		logHRESULT(L"error showing dialog", hr);
		goto out;
	}
	hr = d->GetResult(&result);
	if (hr != S_OK) {
		logHRESULT(L"error getting dialog result", hr);
		goto out;
	}
	hr = result->GetDisplayName(SIGDN_FILESYSPATH, &wname);
	if (hr != S_OK) {
		logHRESULT(L"error getting filename", hr);
		goto out;
	}
	name = toUTF8(wname);

out:
	if (wname != NULL)
		CoTaskMemFree(wname);
	if (result != NULL)
		result->Release();
	if (d != NULL)
		d->Release();
	return name;
}

char *uiOpenFile(uiWindow *parent)
{
	char *res;

	parent = dialogParent(parent);
	disableAllWindowsExcept(parent);
	res = commonItemDialog(windowHWND(parent),
		CLSID_FileOpenDialog, IID_IFileOpenDialog,
		FOS_NOCHANGEDIR | FOS_NOVALIDATE | FOS_PATHMUSTEXIST | FOS_FILEMUSTEXIST | FOS_SHAREAWARE | FOS_NOTESTFILECREATE | FOS_NODEREFERENCELINKS | FOS_FORCESHOWHIDDEN | FOS_DEFAULTNOMINIMODE);
	enableAllWindowsExcept(parent);
	return res;
}

char *uiOpenFolder(uiWindow *parent)
{
	char *res;

	parent = dialogParent(parent);
	disableAllWindowsExcept(parent);
	res = commonItemDialog(windowHWND(parent),
		CLSID_FileOpenDialog, IID_IFileOpenDialog,
		FOS_NOCHANGEDIR | FOS_NOVALIDATE | FOS_PATHMUSTEXIST | FOS_PICKFOLDERS | FOS_SHAREAWARE | FOS_NOTESTFILECREATE | FOS_NODEREFERENCELINKS | FOS_FORCESHOWHIDDEN | FOS_DEFAULTNOMINIMODE);
	enableAllWindowsExcept(parent);
	return res;
}

static char **commonOpenMultiple(HWND parent, FILEOPENDIALOGOPTIONS optsadd,
	HRESULT *error, const WCHAR **operation)
{
	IFileOpenDialog *d = NULL;
	IShellItemArray *results = NULL;
	IShellItem *item = NULL;
	WCHAR *wname = NULL;
	char **paths = NULL;
	DWORD count, i;
	HRESULT hr;

	*error = S_OK;
	*operation = NULL;
	hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER,
		IID_IFileOpenDialog, (LPVOID *) (&d));
	if (hr != S_OK) {
		*operation = L"error creating file open dialog";
		goto fail;
	}
	hr = configureItemDialog(d, optsadd | FOS_ALLOWMULTISELECT);
	if (hr != S_OK) {
		*operation = L"error configuring file open dialog";
		goto fail;
	}
	hr = d->Show(parent);
	if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED))
		goto out;
	if (hr != S_OK) {
		*operation = L"error showing file open dialog";
		goto fail;
	}
	hr = d->GetResults(&results);
	if (hr != S_OK) {
		*operation = L"error getting file dialog results";
		goto fail;
	}
	hr = results->GetCount(&count);
	if (hr != S_OK) {
		*operation = L"error counting file dialog results";
		goto fail;
	}
	if (count == 0)
		goto out;
	paths = uiprivNewTextArray((size_t) count);
	if (paths == NULL)
		goto out;
	for (i = 0; i < count; i++) {
		hr = results->GetItemAt(i, &item);
		if (hr != S_OK) {
			*operation = L"error getting file dialog item";
			goto fail;
		}
		hr = item->GetDisplayName(SIGDN_FILESYSPATH, &wname);
		if (hr != S_OK) {
			*operation = L"error getting file dialog path";
			goto fail;
		}
		paths[i] = toUTF8(wname);
		CoTaskMemFree(wname);
		wname = NULL;
		item->Release();
		item = NULL;
	}
	goto out;

fail:
	*error = hr;
	uiFreeTextArray(paths);
	paths = NULL;
out:
	if (wname != NULL)
		CoTaskMemFree(wname);
	if (item != NULL)
		item->Release();
	if (results != NULL)
		results->Release();
	if (d != NULL)
		d->Release();
	return paths;
}

static char **openMultiple(uiWindow *parent, FILEOPENDIALOGOPTIONS opts)
{
	char **paths;
	HRESULT error;
	const WCHAR *operation;

	parent = dialogParent(parent);
	disableAllWindowsExcept(parent);
	paths = commonOpenMultiple(windowHWND(parent), opts, &error, &operation);
	enableAllWindowsExcept(parent);
	if (operation != NULL)
		logHRESULT(operation, error);
	return paths;
}

char **uiOpenFiles(uiWindow *parent)
{
	return openMultiple(parent,
		FOS_NOCHANGEDIR | FOS_NOVALIDATE | FOS_PATHMUSTEXIST | FOS_FILEMUSTEXIST | FOS_SHAREAWARE | FOS_NOTESTFILECREATE | FOS_NODEREFERENCELINKS | FOS_FORCESHOWHIDDEN | FOS_DEFAULTNOMINIMODE);
}

char **uiOpenFolders(uiWindow *parent)
{
	return openMultiple(parent,
		FOS_NOCHANGEDIR | FOS_NOVALIDATE | FOS_PATHMUSTEXIST | FOS_PICKFOLDERS | FOS_SHAREAWARE | FOS_NOTESTFILECREATE | FOS_NODEREFERENCELINKS | FOS_FORCESHOWHIDDEN | FOS_DEFAULTNOMINIMODE);
}

char *uiSaveFile(uiWindow *parent)
{
	char *res;

	parent = dialogParent(parent);
	disableAllWindowsExcept(parent);
	res = commonItemDialog(windowHWND(parent),
		CLSID_FileSaveDialog, IID_IFileSaveDialog,
		FOS_OVERWRITEPROMPT | FOS_NOCHANGEDIR | FOS_NOVALIDATE | FOS_SHAREAWARE | FOS_NOTESTFILECREATE | FOS_NODEREFERENCELINKS | FOS_FORCESHOWHIDDEN | FOS_DEFAULTNOMINIMODE);
	enableAllWindowsExcept(parent);
	return res;
}

static void msgbox(HWND parent, const char *title, const char *description, TASKDIALOG_COMMON_BUTTON_FLAGS buttons, PCWSTR icon)
{
	WCHAR *wtitle, *wdescription;
	HRESULT hr;

	wtitle = toUTF16(title);
	wdescription = toUTF16(description);

	hr = TaskDialog(parent, NULL, NULL, wtitle, wdescription, buttons, icon, NULL);
	if (hr != S_OK)
		logHRESULT(L"error showing task dialog", hr);

	uiprivFree(wdescription);
	uiprivFree(wtitle);
}

void uiMsgBox(uiWindow *parent, const char *title, const char *description)
{
	parent = dialogParent(parent);
	disableAllWindowsExcept(parent);
	msgbox(windowHWND(parent), title, description, TDCBF_OK_BUTTON, NULL);
	enableAllWindowsExcept(parent);
}

void uiMsgBoxError(uiWindow *parent, const char *title, const char *description)
{
	parent = dialogParent(parent);
	disableAllWindowsExcept(parent);
	msgbox(windowHWND(parent), title, description, TDCBF_OK_BUTTON, TD_ERROR_ICON);
	enableAllWindowsExcept(parent);
}
