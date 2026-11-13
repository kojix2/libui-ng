// 6 april 2015
#include "uipriv_windows.hpp"
#include "attrstr.hpp"

HINSTANCE hInstance;
int nCmdShow;

HFONT hMessageFont;

// LONGTERM needed?
HBRUSH hollowBrush;

// the returned pointer is actually to the second character
// if the first character is - then free, otherwise don't
static const char *initerr(const char *message, const WCHAR *label, DWORD value)
{
	WCHAR *sysmsg;
	BOOL hassysmsg;
	WCHAR *wmessage;
	WCHAR *wout;
	char *out;

	hassysmsg = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, value, 0, (LPWSTR) (&sysmsg), 0, NULL) != 0;
	if (!hassysmsg)
		// FormatMessage() can fail; omit the system-provided text in that case.
		sysmsg = (WCHAR *) L"";
	wmessage = toUTF16(message + 1);
	wout = strf(L"-error initializing libui: %s; code %I32d (0x%08I32X) %s",
		wmessage,
		value, value,
		sysmsg);
	uiprivFree(wmessage);
	if (hassysmsg)
		LocalFree(sysmsg);		// ignore error
	out = toUTF8(wout);
	uiprivFree(wout);
	return out + 1;
}

#define ieLastErr(msg) initerr("=" msg, L"GetLastError() ==", GetLastError())
#define ieHRESULT(msg, hr) initerr("=" msg, L"HRESULT", (DWORD) hr)

// LONGTERM put this declaration in a common file
uiInitOptions uiprivOptions;

#define wantedICCClasses ( \
	ICC_STANDARD_CLASSES |	/* user32.dll controls */		\
	ICC_PROGRESS_CLASS |		/* progress bars */			\
	ICC_TAB_CLASSES |			/* tabs */					\
	ICC_LISTVIEW_CLASSES |		/* table headers */			\
	ICC_UPDOWN_CLASS |		/* spinboxes */			\
	ICC_BAR_CLASSES |			/* trackbar */				\
	ICC_DATE_CLASSES |		/* date/time picker */		\
	0)

const char *uiInit(uiInitOptions *o)
{
	STARTUPINFOW si;
	const char *ce;
	HICON hDefaultIcon;
	HCURSOR hDefaultCursor;
	NONCLIENTMETRICSW ncm;
	INITCOMMONCONTROLSEX icc;
	HRESULT hr;

	uiprivOptions = *o;

	initAlloc();

	nCmdShow = SW_SHOWDEFAULT;
	GetStartupInfoW(&si);
	if ((si.dwFlags & STARTF_USESHOWWINDOW) != 0)
		nCmdShow = si.wShowWindow;

	// LONGTERM set DPI awareness

	hDefaultIcon = LoadIconW(NULL, IDI_APPLICATION);
	if (hDefaultIcon == NULL)
		return ieLastErr("loading default icon for window classes");
	hDefaultCursor = LoadCursorW(NULL, IDC_ARROW);
	if (hDefaultCursor == NULL)
		return ieLastErr("loading default cursor for window classes");

	ce = initUtilWindow(hDefaultIcon, hDefaultCursor);
	if (ce != NULL)
		return initerr(ce, L"GetLastError() ==", GetLastError());

	if (registerWindowClass(hDefaultIcon, hDefaultCursor) == 0) {
		ce = ieLastErr("registering uiWindow window class");
		goto failUtilWindow;
	}

	ZeroMemory(&ncm, sizeof (NONCLIENTMETRICSW));
	ncm.cbSize = sizeof (NONCLIENTMETRICSW);
	if (SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof (NONCLIENTMETRICSW), &ncm, sizeof (NONCLIENTMETRICSW)) == 0) {
		ce = ieLastErr("getting default fonts");
		goto failWindow;
	}
	hMessageFont = CreateFontIndirectW(&(ncm.lfMessageFont));
	if (hMessageFont == NULL) {
		ce = ieLastErr("loading default messagebox font; this is the default UI font");
		goto failWindow;
	}

	if (initContainer(hDefaultIcon, hDefaultCursor) == 0) {
		ce = ieLastErr("initializing uiWindowsMakeContainer() window class");
		goto failFont;
	}

	hollowBrush = (HBRUSH) GetStockObject(HOLLOW_BRUSH);
	if (hollowBrush == NULL) {
		ce = ieLastErr("getting hollow brush");
		goto failContainer;
	}

	ZeroMemory(&icc, sizeof (INITCOMMONCONTROLSEX));
	icc.dwSize = sizeof (INITCOMMONCONTROLSEX);
	icc.dwICC = wantedICCClasses;
	if (InitCommonControlsEx(&icc) == 0) {
		ce = ieLastErr("initializing Common Controls");
		goto failContainer;
	}

	hr = OleInitialize(NULL);
	if (hr != S_OK && hr != S_FALSE) {
		ce = ieHRESULT("initializing OLE on the UI thread (STA required)", hr);
		goto failContainer;
	}
	// LONGTERM initialize COM security
	// LONGTERM (windows vista) turn off COM exception handling

	hr = initDraw();
	if (hr != S_OK) {
		ce = ieHRESULT("initializing Direct2D", hr);
		goto failOLE;
	}

	hr = uiprivInitDrawText();
	if (hr != S_OK) {
		ce = ieHRESULT("initializing DirectWrite", hr);
		goto failDraw;
	}

	if (registerAreaClass(hDefaultIcon, hDefaultCursor) == 0) {
		ce = ieLastErr("registering uiArea window class");
		goto failDrawText;
	}

	if (registerMessageFilter() == 0) {
		ce = ieLastErr("registering libui message filter");
		goto failArea;
	}

	if (registerD2DScratchClass(hDefaultIcon, hDefaultCursor) == 0) {
		ce = ieLastErr("initializing D2D scratch window class");
		goto failFilter;
	}

	hr = uiprivInitImage();
	if (hr != S_OK) {
		ce = ieHRESULT("initializing WIC", hr);
		goto failScratch;
	}

	return NULL;

failScratch:
	unregisterD2DScratchClass();
failFilter:
	unregisterMessageFilter();
failArea:
	unregisterArea();
failDrawText:
	uiprivUninitDrawText();
failDraw:
	uninitDraw();
failOLE:
	// Both S_OK and S_FALSE own an OLE initialization count.
	OleUninitialize();
failContainer:
	hollowBrush = NULL;
	uninitContainer();
failFont:
	if (DeleteObject(hMessageFont) == 0)
		logLastError(L"error deleting control font");
	hMessageFont = NULL;
failWindow:
	unregisterWindowClass();
failUtilWindow:
	uninitUtilWindow();
	// The returned error is still owned by the caller, so do not run the
	// allocation leak check until a successful initialization is uninitialized.
	return ce;
}

void uiUninit(void)
{
	uiprivUninitShouldQuit();
	uiprivControlDestroyUninit();
	uiprivUninitTimers();
	uiprivUninitImageView();
	uiprivUninitImage();
	uninitMenus();
	unregisterD2DScratchClass();
	unregisterMessageFilter();
	unregisterArea();
	uiprivUninitDrawText();
	uninitDraw();
	OleUninitialize();
	hollowBrush = NULL;
	uninitContainer();
	if (DeleteObject(hMessageFont) == 0)
		logLastError(L"error deleting control font");
	unregisterWindowClass();
	// no need to delete the default icon or cursor; see http://stackoverflow.com/questions/30603077/
	uninitUtilWindow();
	uninitAlloc();
}

void uiFreeInitError(const char *err)
{
	if (*(err - 1) == '-')
		uiprivFree((void *) (err - 1));
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
		hInstance = hinstDLL;
	return TRUE;
}
