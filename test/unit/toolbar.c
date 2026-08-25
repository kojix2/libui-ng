#include "unit.h"

#ifdef _WIN32
#include <windows.h>
#include <commctrl.h>
#endif

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
#ifdef _WIN32
		toolbarUnitTest(toolbarButtonHasIconAndText),
		toolbarUnitTest(toolbarIconOnlyFallsBackToText),
		toolbarUnitTest(toolbarHorizontalShowsIconAndText),
		toolbarUnitTest(toolbarTextOnlyHidesIcon),
#endif
		cmocka_unit_test(toolbarWindowDestroyDetaches),
	};

	return cmocka_run_group_tests_name("uiToolbar", tests,
		unitTestsSetup, unitTestsTeardown);
}
