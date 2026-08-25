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
	uiToolbarAppendSpace(toolbar);
	uiToolbarAppendFlexibleSpace(toolbar);

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
static void toolbarButtonHasIconAndText(void **state)
{
	struct state *s = *state;
	unsigned char pixels[16 * 16 * 4];
	uiImage *image;
	uiToolbar *toolbar;
	HWND nativeToolbar;
	HIMAGELIST images;
	TBBUTTON button;
	WCHAR text[16];

	for (size_t i = 0; i < sizeof pixels; i += 4) {
		pixels[i] = 0xFF;
		pixels[i + 1] = 0;
		pixels[i + 2] = 0;
		pixels[i + 3] = 0xFF;
	}
	image = uiNewImage(16, 16);
	uiImageAppend(image, pixels, 16, 16, 16 * 4);
	toolbar = uiNewToolbar();
	uiToolbarAppendButton(toolbar, "Icon", image);
	uiWindowSetToolbar(s->w, toolbar);

	nativeToolbar = FindWindowExW((HWND) uiControlHandle(uiControl(s->w)),
		NULL, TOOLBARCLASSNAMEW, NULL);
	assert_non_null(nativeToolbar);
	images = (HIMAGELIST) SendMessageW(nativeToolbar, TB_GETIMAGELIST, 0, 0);
	assert_non_null(images);
	assert_int_equal(ImageList_GetImageCount(images), 1);
	ZeroMemory(&button, sizeof button);
	assert_true(SendMessageW(nativeToolbar, TB_GETBUTTON, 0,
		(LPARAM) &button));
	assert_int_equal(button.iBitmap, 0);
	assert_int_equal(SendMessageW(nativeToolbar, TB_GETBUTTONTEXTW,
		button.idCommand, (LPARAM) text), 4);
	assert_true(wcscmp(text, L"Icon") == 0);

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
#endif
		cmocka_unit_test(toolbarWindowDestroyDetaches),
	};

	return cmocka_run_group_tests_name("uiToolbar", tests,
		unitTestsSetup, unitTestsTeardown);
}
