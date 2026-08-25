#include "unit.h"

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
		cmocka_unit_test(toolbarWindowDestroyDetaches),
	};

	return cmocka_run_group_tests_name("uiToolbar", tests,
		unitTestsSetup, unitTestsTeardown);
}
