#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#import <AppKit/AppKit.h>

#include "../../ui.h"
#include "../../common/toolbar.h"

static void countClicks(uiToolbarItem *item, void *data)
{
	int *calls = data;
	(*calls)++;
}

static void toolbarButtonKeepsActionAcrossDisplayModes(void **state)
{
	uiInitOptions options = {0};
	uiWindow *window;
	uiToolbar *toolbar;
	uiToolbarItem *item;
	uiImage *icon;
	unsigned char pixels[16 * 16 * 4];
	NSToolbarItem *native;
	id target;
	SEL action;
	int calls = 0;

	(void) state;
	assert_null(uiInit(&options));
	window = uiNewWindow("Toolbar test", 320, 200, 0);
	toolbar = uiNewToolbar();
	for (size_t i = 0; i < sizeof pixels; i += 4) {
		pixels[i] = 0xFF;
		pixels[i + 1] = 0;
		pixels[i + 2] = 0;
		pixels[i + 3] = 0xFF;
	}
	icon = uiNewImage(16, 16);
	uiImageAppend(icon, pixels, 16, 16, 16 * 4);
	item = uiToolbarAppendButton(toolbar, "Open", icon);
	uiToolbarItemOnClicked(item, countClicks, &calls);
	uiWindowSetToolbar(window, toolbar);
	uiControlShow(uiControl(window));
	native = (NSToolbarItem *) item->native;
	target = [native target];
	action = [native action];
	assert_non_null(target);
	assert_true(action == @selector(itemClicked:));

	for (int i = 0; i < 2; i++) {
		NSButton *button;

		uiToolbarSetDisplayMode(toolbar, uiToolbarDisplayModeIconOnly);
		button = (NSButton *) [native view];
		assert_non_null(button);
		assert_ptr_equal([button target], target);
		assert_true([button action] == action);
		[button performClick:nil];
		assert_int_equal(calls, i + 1);

		uiToolbarSetDisplayMode(toolbar, uiToolbarDisplayModeIconAndTextVertical);
		assert_null([native view]);
		assert_ptr_equal([native target], target);
		assert_true([native action] == action);
	}

	uiWindowSetToolbar(window, NULL);
	uiFreeToolbar(toolbar);
	uiFreeImage(icon);
	uiControlDestroy(uiControl(window));
	uiUninit();
}

int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(toolbarButtonKeepsActionAcrossDisplayModes),
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
