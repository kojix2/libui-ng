#include "unit.h"

#define uiFontButtonFromState(s) \
	((uiFontButton *) (((struct state *) *(s))->c))

static int fontButtonSetup(void **state)
{
	struct state *s;

	if (unitTestSetup(state) != 0)
		return -1;
	s = (struct state *) *state;
	s->c = uiControl(uiNewFontButton());
	return 0;
}

static void fontButtonNew(void **state)
{
	assert_non_null(uiFontButtonFromState(state));
}

static void fontButtonFont(void **state)
{
	uiFontButton *button = uiFontButtonFromState(state);
	uiFontDescriptor font;

	uiFontButtonFont(button, &font);
	assert_non_null(font.Family);
	assert_true(font.Size > 0);
	uiFreeFontButtonFont(&font);
}

static void onChangedNoCall(uiFontButton *button, void *data)
{
	function_called();
}

static void fontButtonRegisterCallback(void **state)
{
	uiFontButton *button = uiFontButtonFromState(state);
	uiFontDescriptor font;

	uiFontButtonOnChanged(button, onChangedNoCall, NULL);
	uiFontButtonFont(button, &font);
	uiFreeFontButtonFont(&font);
}

#define fontButtonUnitTest(f) cmocka_unit_test_setup_teardown((f), \
	fontButtonSetup, unitTestTeardown)

int fontButtonRunUnitTests(void)
{
	const struct CMUnitTest tests[] = {
		fontButtonUnitTest(fontButtonNew),
		fontButtonUnitTest(fontButtonFont),
		fontButtonUnitTest(fontButtonRegisterCallback),
	};

	return cmocka_run_group_tests_name("uiFontButton", tests,
		unitTestsSetup, unitTestsTeardown);
}
