#include "unit.h"

#ifdef _UI_STATIC
#if !defined(_WIN32) && !defined(__APPLE__)
#include "../../unix/uipriv_unix.h"
#else
#include "../../common/uipriv.h"
#endif
#include "../../common/toolbar.h"
#endif

#ifdef _WIN32
#include <windows.h>
#include <commctrl.h>
#include "../../ui_windows.h"
#endif

static void countToolbarClicks(uiToolbarItem *item, void *data)
{
	int *calls = data;

	(*calls)++;
}

static void toolbarDefaults(void **state)
{
	uiToolbar *toolbar = uiNewToolbar();
	uiToolbarItem *button;
	uiToolbarItem *toggle;
	char *text;

	button = uiToolbarAppendButton(toolbar, "Open", NULL);
	toggle = uiToolbarAppendToggleButton(toolbar, "Pinned", NULL);
	uiToolbarAppendSeparator(toolbar);

	assert_int_equal(uiToolbarGetDisplayMode(toolbar),
		uiToolbarDisplayModeIconAndTextVertical);
	text = uiToolbarItemText(button);
	assert_string_equal(text, "Open");
	uiFreeText(text);
	assert_true(uiToolbarItemEnabled(button));
	assert_false(uiToolbarItemChecked(toggle));

	uiFreeToolbar(toolbar);
}

static void toolbarSetters(void **state)
{
	uiToolbar *toolbar = uiNewToolbar();
	uiToolbarItem *item = uiToolbarAppendToggleButton(toolbar, "Pin", NULL);
	char *text;

	uiToolbarSetDisplayMode(toolbar, uiToolbarDisplayModeIconOnly);
	assert_int_equal(uiToolbarGetDisplayMode(toolbar),
		uiToolbarDisplayModeIconOnly);
	uiToolbarSetDisplayMode(toolbar, uiToolbarDisplayModeIconAndTextHorizontal);
	assert_int_equal(uiToolbarGetDisplayMode(toolbar),
		uiToolbarDisplayModeIconAndTextHorizontal);
	uiToolbarSetDisplayMode(toolbar, uiToolbarDisplayModeTextOnly);
	assert_int_equal(uiToolbarGetDisplayMode(toolbar),
		uiToolbarDisplayModeTextOnly);
	uiToolbarSetDisplayMode(toolbar, uiToolbarDisplayModeIconAndTextVertical);

	uiToolbarItemSetText(item, "Pinned");
	uiToolbarItemSetTooltip(item, "Keep this window visible");
	uiToolbarItemDisable(item);
	uiToolbarItemSetChecked(item, 1);

	text = uiToolbarItemText(item);
	assert_string_equal(text, "Pinned");
	uiFreeText(text);
	text = uiToolbarItemTooltip(item);
	assert_string_equal(text, "Keep this window visible");
	uiFreeText(text);
	assert_false(uiToolbarItemEnabled(item));
	assert_true(uiToolbarItemChecked(item));

	uiFreeToolbar(toolbar);
}

static void toolbarAttachDetach(void **state)
{
	struct state *s = *state;
	uiToolbar *toolbar = uiNewToolbar();

	uiToolbarAppendButton(toolbar, "Action", NULL);
	assert_null(uiWindowToolbar(s->w));
	uiWindowSetToolbar(s->w, toolbar);
	assert_ptr_equal(uiWindowToolbar(s->w), toolbar);
	uiWindowSetToolbar(s->w, NULL);
	assert_null(uiWindowToolbar(s->w));
	uiFreeToolbar(toolbar);
}

static void toolbarLiveDisplayModes(void **state)
{
	struct state *s = *state;
	uiToolbar *toolbar = uiNewToolbar();
	uiToolbarItem *item = uiToolbarAppendToggleButton(toolbar, "Pinned", NULL);
	int calls = 0;

	uiToolbarItemSetTooltip(item, "Keep pinned");
	uiToolbarItemDisable(item);
	uiToolbarItemSetChecked(item, 1);
	uiToolbarItemOnClicked(item, countToolbarClicks, &calls);
	uiWindowSetToolbar(s->w, toolbar);
	for (int from = 0; from < 4; from++) {
		for (int to = 0; to < 4; to++) {
			uiToolbarSetDisplayMode(toolbar, (uiToolbarDisplayMode) from);
			uiToolbarSetDisplayMode(toolbar, (uiToolbarDisplayMode) to);
			assert_int_equal(uiToolbarGetDisplayMode(toolbar), to);
			assert_ptr_equal(uiWindowToolbar(s->w), toolbar);
			assert_true(uiToolbarItemChecked(item));
			assert_false(uiToolbarItemEnabled(item));
			char *text = uiToolbarItemText(item);
			assert_string_equal(text, "Pinned");
			uiFreeText(text);
			text = uiToolbarItemTooltip(item);
			assert_string_equal(text, "Keep pinned");
			uiFreeText(text);
#if defined(_UI_STATIC) && !defined(_WIN32) && !defined(__APPLE__)
			static const GtkToolbarStyle styles[] = {
				GTK_TOOLBAR_ICONS, GTK_TOOLBAR_BOTH_HORIZ,
				GTK_TOOLBAR_BOTH, GTK_TOOLBAR_TEXT,
			};
			assert_int_equal(gtk_toolbar_get_style(GTK_TOOLBAR(toolbar->native)), styles[to]);
			assert_true(gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(item->native)));
#endif
			assert_int_equal(calls, 0);
		}
	}
	uiWindowSetToolbar(s->w, NULL);
	uiToolbarSetDisplayMode(toolbar, uiToolbarDisplayModeIconOnly);
	uiWindowSetToolbar(s->w, toolbar);
	assert_int_equal(uiToolbarGetDisplayMode(toolbar), uiToolbarDisplayModeIconOnly);
	assert_true(uiToolbarItemChecked(item));
	uiWindowSetToolbar(s->w, NULL);
	uiFreeToolbar(toolbar);
}

#if defined(_UI_STATIC) && !defined(_WIN32) && !defined(__APPLE__)
static void toolbarProgrammaticToggleDoesNotClick(void **state)
{
	struct state *s = *state;
	uiToolbar *toolbar = uiNewToolbar();
	uiToolbarItem *item;
	GtkToolItem *native;
	int calls = 0;

	item = uiToolbarAppendToggleButton(toolbar, "Pinned", NULL);
	uiToolbarItemOnClicked(item, countToolbarClicks, &calls);
	uiToolbarItemSetChecked(item, 1);
	uiWindowSetToolbar(s->w, toolbar);
	assert_int_equal(calls, 0);

	native = gtk_toolbar_get_nth_item(GTK_TOOLBAR(uiprivToolbarWidget(toolbar)), 0);
	assert_non_null(native);
	assert_true(gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(native)));

	uiToolbarItemSetChecked(item, 0);
	assert_int_equal(calls, 0);
	assert_false(gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(native)));

	gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(native), TRUE);
	assert_int_equal(calls, 1);
	assert_true(uiToolbarItemChecked(item));

	uiWindowSetToolbar(s->w, NULL);
	uiFreeToolbar(toolbar);
}
#endif

#ifdef _UI_STATIC
static void toolbarPendingWindowSuppressesClick(void **state)
{
	uiInitOptions options = {0};
	uiWindow *window;
	uiToolbar *toolbar;
	uiToolbarItem *item;
	int calls = 0;

	assert_null(uiInit(&options));
	window = uiNewWindow("Toolbar callback", 320, 200, 0);
	toolbar = uiNewToolbar();
	item = uiToolbarAppendToggleButton(toolbar, "Pinned", NULL);
	uiToolbarItemOnClicked(item, countToolbarClicks, &calls);
	uiWindowSetToolbar(window, toolbar);

	uiprivUserCallbackEnter(NULL);
	uiControlDestroy(uiControl(window));
	uiprivToolbarItemClicked(item, 1);
	assert_int_equal(calls, 0);
	assert_false(uiToolbarItemChecked(item));
	uiprivUserCallbackLeave();
	uiprivControlDestroyFlushPending();

	uiFreeToolbar(toolbar);
	uiUninit();
}
#endif

#ifdef _WIN32
static uiImage *newTestImage(void)
{
	unsigned char pixels[16 * 16 * 4];
	uiImage *image;

	for (size_t i = 0; i < sizeof pixels; i += 4) {
		pixels[i] = 0xFF;
		pixels[i + 1] = 0;
		pixels[i + 2] = 0;
		pixels[i + 3] = 0xFF;
	}
	image = uiNewImage(16, 16);
	uiImageAppend(image, pixels, 16, 16, 16 * 4);
	return image;
}

static HWND nativeToolbarForWindow(uiWindow *window)
{
	return FindWindowExW((HWND) uiControlHandle(uiControl(window)),
		NULL, TOOLBARCLASSNAMEW, NULL);
}

static void toolbarWindowsLeadingInset(void **state)
{
	struct state *s = *state;
	uiToolbar *toolbar = uiNewToolbar();
	uiWindowsSizing sizing;
	int expected = 3;

	uiWindowsGetSizing((HWND) uiControlHandle(uiControl(s->w)), &sizing);
	uiWindowsSizingDlgUnitsToPixels(&sizing, &expected, NULL);
	uiToolbarAppendButton(toolbar, "Open", NULL);
	uiWindowSetToolbar(s->w, toolbar);
	for (int mode = 0; mode < 4; mode++) {
		RECT button;
		uiToolbarSetDisplayMode(toolbar, (uiToolbarDisplayMode) mode);
		assert_true(SendMessageW(nativeToolbarForWindow(s->w),
			TB_GETITEMRECT, 0, (LPARAM) &button));
		assert_int_equal(button.left, expected);
	}
	uiWindowSetToolbar(s->w, NULL);
	uiFreeToolbar(toolbar);
}

static void toolbarWindowsMeasurementDoesNotResize(void **state)
{
	struct state *s = *state;
	uiToolbar *toolbar = uiNewToolbar();
	uiLabel *label = uiNewLabel("Ready");
	HWND parent = (HWND) uiControlHandle(uiControl(s->w));
	MINMAXINFO minimum = {0};
	RECT before, after, client;
	int width, height;

	uiToolbarAppendButton(toolbar, "Zoom in", NULL);
	uiWindowSetChild(s->w, uiControl(label));
	uiWindowSetToolbar(s->w, toolbar);
	for (int mode = 0; mode < 4; mode++) {
		uiToolbarSetDisplayMode(toolbar, (uiToolbarDisplayMode) mode);
		HWND native = nativeToolbarForWindow(s->w);
		GetWindowRect(native, &before);
		uiWindowsControlMinimumSize(uiWindowsControl(s->w), &width, &height);
		SendMessageW(parent, WM_GETMINMAXINFO, 0, (LPARAM) &minimum);
		GetWindowRect(native, &after);
		assert_true(EqualRect(&before, &after));
		uiLabelSetText(label, "Loading a region");
		GetClientRect(parent, &client);
		GetWindowRect(native, &after);
		assert_int_equal(after.right - after.left, client.right - client.left);
		uiLabelSetText(label, "Ready");
	}
	uiWindowSetToolbar(s->w, NULL);
	uiFreeToolbar(toolbar);
}

static LRESULT CALLBACK countToolbarUpdates(HWND hwnd, UINT message,
	WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR data)
{
	int *updates = (int *) data;
	if (message == TB_SETBUTTONINFOW || message == TB_ENABLEBUTTON ||
		message == TB_CHECKBUTTON)
		(*updates)++;
	return DefSubclassProc(hwnd, message, wParam, lParam);
}

static void toolbarWindowsUpdatesPreserveNativeState(void **state)
{
	struct state *s = *state;
	uiToolbar *toolbar = uiNewToolbar();
	uiToolbarItem *item = uiToolbarAppendToggleButton(toolbar, "Pin", NULL);
	TBBUTTON button;
	int updates = 0;
	WCHAR text[32];

	uiWindowSetToolbar(s->w, toolbar);
	HWND native = nativeToolbarForWindow(s->w);
	assert_true(SendMessageW(native, TB_GETBUTTON, 0, (LPARAM) &button));
	SendMessageW(native, TB_PRESSBUTTON, button.idCommand, TRUE);
	assert_true(SetWindowSubclass(native, countToolbarUpdates, 1, (DWORD_PTR) &updates));
	uiToolbarItemEnable(item);
	uiToolbarItemSetChecked(item, 0);
	uiToolbarItemSetText(item, "Pin");
	uiToolbarItemSetTooltip(item, "Keep pinned");
	assert_int_equal(updates, 0);
	assert_true(SendMessageW(native, TB_ISBUTTONPRESSED, button.idCommand, 0));
	uiToolbarItemSetChecked(item, 1);
	assert_true(SendMessageW(native, TB_ISBUTTONCHECKED, button.idCommand, 0));
	assert_true(SendMessageW(native, TB_ISBUTTONPRESSED, button.idCommand, 0));
	uiToolbarItemSetText(item, "Pinned");
	assert_true(SendMessageW(native, TB_ISBUTTONCHECKED, button.idCommand, 0));
	assert_true(SendMessageW(native, TB_ISBUTTONPRESSED, button.idCommand, 0));
	assert_int_equal(SendMessageW(native, TB_GETBUTTONTEXTW, button.idCommand, (LPARAM) text), 6);
	assert_true(wcscmp(text, L"Pinned") == 0);
	uiToolbarItemDisable(item);
	assert_false(SendMessageW(native, TB_ISBUTTONENABLED, button.idCommand, 0));
	assert_true(SendMessageW(native, TB_ISBUTTONCHECKED, button.idCommand, 0));
	updates = 0;
	uiToolbarItemDisable(item);
	uiToolbarItemSetChecked(item, 1);
	uiToolbarItemSetText(item, "Pinned");
	uiToolbarItemSetTooltip(item, "Keep pinned");
	assert_int_equal(updates, 0);
	RemoveWindowSubclass(native, countToolbarUpdates, 1);
	uiWindowSetToolbar(s->w, NULL);
	uiFreeToolbar(toolbar);
}

static void toolbarButtonHasIconAndText(void **state)
{
	struct state *s = *state;
	uiImage *image = newTestImage();
	uiToolbar *toolbar;
	HWND nativeToolbar;
	HIMAGELIST images;
	TBBUTTON button;
	WCHAR text[16];

	toolbar = uiNewToolbar();
	uiToolbarAppendButton(toolbar, "Icon", image);
	uiWindowSetToolbar(s->w, toolbar);

	nativeToolbar = nativeToolbarForWindow(s->w);
	assert_non_null(nativeToolbar);
	assert_false(GetWindowLongPtrW(nativeToolbar, GWL_STYLE) & TBSTYLE_LIST);
	images = (HIMAGELIST) SendMessageW(nativeToolbar, TB_GETIMAGELIST, 0, 0);
	assert_non_null(images);
	assert_int_equal(ImageList_GetImageCount(images), 1);
	ZeroMemory(&button, sizeof button);
	assert_true(SendMessageW(nativeToolbar, TB_GETBUTTON, 0,
		(LPARAM) &button));
	assert_int_equal(button.iBitmap, 0);
	assert_false(button.fsStyle & BTNS_SHOWTEXT);
	assert_int_equal(SendMessageW(nativeToolbar, TB_GETBUTTONTEXTW,
		button.idCommand, (LPARAM) text), 4);
	assert_true(wcscmp(text, L"Icon") == 0);

	uiWindowSetToolbar(s->w, NULL);
	uiFreeToolbar(toolbar);
	uiFreeImage(image);
}

static void toolbarIconOnlyFallsBackToText(void **state)
{
	struct state *s = *state;
	uiImage *image = newTestImage();
	uiToolbar *toolbar = uiNewToolbar();
	HWND nativeToolbar;
	TBBUTTON button;
	WCHAR text[16];

	uiToolbarSetDisplayMode(toolbar, uiToolbarDisplayModeIconOnly);
	uiToolbarAppendButton(toolbar, "Icon", image);
	uiToolbarAppendButton(toolbar, "Text", NULL);
	uiWindowSetToolbar(s->w, toolbar);
	nativeToolbar = nativeToolbarForWindow(s->w);
	assert_non_null(nativeToolbar);
	assert_true(SendMessageW(nativeToolbar, TB_GETBUTTON, 0,
		(LPARAM) &button));
	assert_int_equal(button.iBitmap, 0);
	assert_int_equal(SendMessageW(nativeToolbar, TB_GETBUTTONTEXTW,
		button.idCommand, (LPARAM) text), -1);
	assert_true(SendMessageW(nativeToolbar, TB_GETBUTTON, 1,
		(LPARAM) &button));
	assert_int_equal(button.iBitmap, I_IMAGENONE);
	assert_int_equal(SendMessageW(nativeToolbar, TB_GETBUTTONTEXTW,
		button.idCommand, (LPARAM) text), 4);
	assert_true(wcscmp(text, L"Text") == 0);

	uiWindowSetToolbar(s->w, NULL);
	uiFreeToolbar(toolbar);
	uiFreeImage(image);
}

static void toolbarHorizontalShowsIconAndText(void **state)
{
	struct state *s = *state;
	uiImage *image = newTestImage();
	uiToolbar *toolbar = uiNewToolbar();
	HWND nativeToolbar;
	TBBUTTON button;

	uiToolbarSetDisplayMode(toolbar,
		uiToolbarDisplayModeIconAndTextHorizontal);
	uiToolbarAppendButton(toolbar, "Icon", image);
	uiWindowSetToolbar(s->w, toolbar);
	nativeToolbar = nativeToolbarForWindow(s->w);
	assert_non_null(nativeToolbar);
	assert_true(GetWindowLongPtrW(nativeToolbar, GWL_STYLE) & TBSTYLE_LIST);
	assert_true(SendMessageW(nativeToolbar, TB_GETEXTENDEDSTYLE, 0, 0) &
		TBSTYLE_EX_MIXEDBUTTONS);
	assert_true(SendMessageW(nativeToolbar, TB_GETBUTTON, 0,
		(LPARAM) &button));
	assert_int_equal(button.iBitmap, 0);
	assert_true(button.fsStyle & BTNS_SHOWTEXT);

	uiWindowSetToolbar(s->w, NULL);
	uiFreeToolbar(toolbar);
	uiFreeImage(image);
}

static void toolbarTextOnlyHidesIcon(void **state)
{
	struct state *s = *state;
	uiImage *image = newTestImage();
	uiToolbar *toolbar = uiNewToolbar();
	HWND nativeToolbar;
	TBBUTTON button;
	WCHAR text[16];

	uiToolbarSetDisplayMode(toolbar, uiToolbarDisplayModeTextOnly);
	uiToolbarAppendButton(toolbar, "Text", image);
	uiWindowSetToolbar(s->w, toolbar);
	nativeToolbar = nativeToolbarForWindow(s->w);
	assert_non_null(nativeToolbar);
	assert_true(SendMessageW(nativeToolbar, TB_GETBUTTON, 0,
		(LPARAM) &button));
	assert_int_equal(button.iBitmap, I_IMAGENONE);
	assert_int_equal(SendMessageW(nativeToolbar, TB_GETBUTTONTEXTW,
		button.idCommand, (LPARAM) text), 4);
	assert_true(wcscmp(text, L"Text") == 0);

	uiWindowSetToolbar(s->w, NULL);
	uiFreeToolbar(toolbar);
	uiFreeImage(image);
}

static void toolbarWindowsLiveDisplayModes(void **state)
{
	struct state *s = *state;
	uiImage *image = newTestImage();
	uiToolbar *toolbar = uiNewToolbar();
	uiToolbarItem *item = uiToolbarAppendToggleButton(toolbar, "Icon", image);
	uiToolbarAppendButton(toolbar, "Text", NULL);
	uiToolbarAppendSeparator(toolbar);
	uiToolbarItemDisable(item);
	uiToolbarItemSetChecked(item, 1);
	uiToolbarItemSetTooltip(item, "Tooltip");
	uiWindowSetToolbar(s->w, toolbar);
	uiControlShow(uiControl(s->w));
	uiLabel *child = uiNewLabel("Content");
	uiWindowSetChild(s->w, uiControl(child));
	SetFocus(nativeToolbarForWindow(s->w));

	for (int from = 0; from < 4; from++) {
		for (int to = 0; to < 4; to++) {
			uiToolbarSetDisplayMode(toolbar, (uiToolbarDisplayMode) from);
			uiToolbarSetDisplayMode(toolbar, (uiToolbarDisplayMode) to);
			HWND native = nativeToolbarForWindow(s->w);
			assert_non_null(native);
			assert_ptr_equal(GetFocus(), native);
			assert_int_equal(SendMessageW(native, TB_BUTTONCOUNT, 0, 0), 3);
			assert_int_equal((GetWindowLongPtrW(native, GWL_STYLE) & TBSTYLE_LIST) != 0, to == 1);
			TBBUTTON button;
			assert_true(SendMessageW(native, TB_GETBUTTON, 0, (LPARAM) &button));
			assert_true(button.fsState & TBSTATE_CHECKED);
			assert_false(button.fsState & TBSTATE_ENABLED);
			assert_true(button.fsStyle & BTNS_CHECK);
			assert_int_equal((button.fsStyle & BTNS_SHOWTEXT) != 0, to == 1);
			assert_int_equal(button.iBitmap, to == 3 ? I_IMAGENONE : 0);
			WCHAR text[32];
			LRESULT len = SendMessageW(native, TB_GETBUTTONTEXTW, button.idCommand, (LPARAM) text);
			if (to == 0)
				assert_true(len <= 0);
			else {
				assert_int_equal(len, 4);
				assert_true(wcscmp(text, L"Icon") == 0);
			}
			NMTBGETINFOTIPW tip;
			ZeroMemory(&tip, sizeof tip);
			tip.hdr.hwndFrom = native;
			tip.hdr.code = TBN_GETINFOTIPW;
			tip.iItem = button.idCommand;
			tip.pszText = text;
			tip.cchTextMax = 32;
			SendMessageW((HWND) uiControlHandle(uiControl(s->w)), WM_NOTIFY, 0, (LPARAM) &tip);
			assert_true(wcscmp(text, L"Tooltip") == 0);
			assert_true(SendMessageW(native, TB_GETBUTTON, 1, (LPARAM) &button));
			assert_int_equal(SendMessageW(native, TB_GETBUTTONTEXTW, button.idCommand, (LPARAM) text), 4);
			assert_true(wcscmp(text, L"Text") == 0);

			// Switching must produce the same dimensions as a fresh toolbar.
			uiWindow *referenceWindow = uiNewWindow("Reference", 640, 400, 0);
			uiToolbar *reference = uiNewToolbar();
			uiToolbarSetDisplayMode(reference, (uiToolbarDisplayMode) to);
			uiToolbarAppendToggleButton(reference, "Icon", image);
			uiToolbarAppendButton(reference, "Text", NULL);
			uiToolbarAppendSeparator(reference);
			uiWindowSetToolbar(referenceWindow, reference);
			for (int i = 0; i < 3; i++) {
				RECT actual, expected;
				assert_true(SendMessageW(native, TB_GETITEMRECT, i, (LPARAM) &actual));
				assert_true(SendMessageW(nativeToolbarForWindow(referenceWindow), TB_GETITEMRECT, i, (LPARAM) &expected));
				assert_memory_equal(&actual, &expected, sizeof actual);
			}
			RECT toolbarRect, childRect;
			GetWindowRect(native, &toolbarRect);
			GetWindowRect((HWND) uiControlHandle(uiControl(child)), &childRect);
			assert_int_equal(toolbarRect.bottom, childRect.top);
			uiControlDestroy(uiControl(referenceWindow));
			uiFreeToolbar(reference);
			uiToolbarSetDisplayMode(toolbar, (uiToolbarDisplayMode) to);
			assert_ptr_equal(nativeToolbarForWindow(s->w), native);
		}
	}
	uiWindowSetToolbar(s->w, NULL);
	uiFreeToolbar(toolbar);
	uiFreeImage(image);
}

struct toolbarSwitchOnClick {
	uiToolbar *toolbar;
	int calls;
};

static void toolbarWindowsRecreationResources(void **state)
{
	struct state *s = *state;
	uiImage *image = newTestImage();
	uiToolbar *toolbar = uiNewToolbar();
	uiToolbarAppendButton(toolbar, "Icon", image);
	uiWindowSetToolbar(s->w, toolbar);
	// Warm up all modes before counting resources cached by the OS.
	for (int i = 0; i < 8; i++)
		uiToolbarSetDisplayMode(toolbar, (uiToolbarDisplayMode) (i % 4));
	DWORD gdi = GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS);
	DWORD user = GetGuiResources(GetCurrentProcess(), GR_USEROBJECTS);
	assert_true(gdi > 0);
	assert_true(user > 0);
	for (int i = 0; i < 256; i++)
		uiToolbarSetDisplayMode(toolbar, (uiToolbarDisplayMode) (i % 4));
	assert_int_equal(GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS), gdi);
	assert_int_equal(GetGuiResources(GetCurrentProcess(), GR_USEROBJECTS), user);
	uiWindowSetToolbar(s->w, NULL);
	uiFreeToolbar(toolbar);
	uiFreeImage(image);
}

static void switchToolbarOnClick(uiToolbarItem *item, void *data)
{
	struct toolbarSwitchOnClick *s = data;
	s->calls++;
	uiToolbarSetDisplayMode(s->toolbar, uiToolbarDisplayModeIconOnly);
}

static void toolbarWindowsSwitchInCallback(void **state)
{
	struct state *s = *state;
	uiToolbar *toolbar = uiNewToolbar();
	uiToolbarItem *item = uiToolbarAppendToggleButton(toolbar, "Pinned", NULL);
	struct toolbarSwitchOnClick click = {toolbar, 0};
	uiToolbarItemOnClicked(item, switchToolbarOnClick, &click);
	uiWindowSetToolbar(s->w, toolbar);
	HWND native = nativeToolbarForWindow(s->w);
	TBBUTTON button;
	assert_true(SendMessageW(native, TB_GETBUTTON, 0, (LPARAM) &button));
	SendMessageW(native, TB_CHECKBUTTON, button.idCommand, TRUE);
	SendMessageW((HWND) uiControlHandle(uiControl(s->w)), WM_COMMAND,
		MAKEWPARAM(button.idCommand, 0), (LPARAM) native);
	assert_int_equal(click.calls, 1);
	assert_true(uiToolbarItemChecked(item));
	assert_int_equal(uiToolbarGetDisplayMode(toolbar), uiToolbarDisplayModeIconOnly);
	assert_true(SendMessageW(nativeToolbarForWindow(s->w), TB_ISBUTTONCHECKED, button.idCommand, 0));
	uiWindowSetToolbar(s->w, NULL);
	uiFreeToolbar(toolbar);
}
#endif

static void toolbarWindowDestroyDetaches(void **state)
{
	uiInitOptions options = {0};
	uiWindow *window;
	uiToolbar *toolbar;

	assert_null(uiInit(&options));
	window = uiNewWindow("Toolbar lifetime", 320, 200, 0);
	toolbar = uiNewToolbar();
	uiToolbarAppendButton(toolbar, "Action", NULL);
	uiWindowSetToolbar(window, toolbar);
	uiControlDestroy(uiControl(window));
	uiFreeToolbar(toolbar);
	uiUninit();
}

#define toolbarUnitTest(f) cmocka_unit_test_setup_teardown((f), \
	unitTestSetup, unitTestTeardown)

int toolbarRunUnitTests(void)
{
	const struct CMUnitTest tests[] = {
		toolbarUnitTest(toolbarDefaults),
		toolbarUnitTest(toolbarSetters),
		toolbarUnitTest(toolbarAttachDetach),
		toolbarUnitTest(toolbarLiveDisplayModes),
#if defined(_UI_STATIC) && !defined(_WIN32) && !defined(__APPLE__)
		toolbarUnitTest(toolbarProgrammaticToggleDoesNotClick),
#endif
#ifdef _UI_STATIC
		cmocka_unit_test(toolbarPendingWindowSuppressesClick),
#endif
#ifdef _WIN32
		toolbarUnitTest(toolbarWindowsLeadingInset),
		toolbarUnitTest(toolbarWindowsMeasurementDoesNotResize),
		toolbarUnitTest(toolbarWindowsUpdatesPreserveNativeState),
		toolbarUnitTest(toolbarButtonHasIconAndText),
		toolbarUnitTest(toolbarIconOnlyFallsBackToText),
		toolbarUnitTest(toolbarHorizontalShowsIconAndText),
		toolbarUnitTest(toolbarTextOnlyHidesIcon),
		toolbarUnitTest(toolbarWindowsLiveDisplayModes),
		toolbarUnitTest(toolbarWindowsSwitchInCallback),
		toolbarUnitTest(toolbarWindowsRecreationResources),
#endif
		cmocka_unit_test(toolbarWindowDestroyDetaches),
	};

	return cmocka_run_group_tests_name("uiToolbar", tests,
		unitTestsSetup, unitTestsTeardown);
}
