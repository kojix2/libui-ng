#include "unit.h"

#define uiColorButtonFromState(s) \
	((uiColorButton *) (((struct state *) *(s))->c))

static int colorButtonSetup(void **state)
{
	struct state *s;

	if (unitTestSetup(state) != 0)
		return -1;
	s = (struct state *) *state;
	s->c = uiControl(uiNewColorButton());
	return 0;
}

static void colorButtonNew(void **state)
{
	assert_non_null(uiColorButtonFromState(state));
}

static void assertColorClose(double actual, double expected)
{
	assert_true(actual > expected - 0.001);
	assert_true(actual < expected + 0.001);
}

static void colorButtonSetColor(void **state)
{
	uiColorButton *button = uiColorButtonFromState(state);
	double r, g, b, a;

	uiColorButtonSetColor(button, 0.2, 0.4, 0.6, 0.8);
	uiColorButtonColor(button, &r, &g, &b, &a);
	assertColorClose(r, 0.2);
	assertColorClose(g, 0.4);
	assertColorClose(b, 0.6);
	assertColorClose(a, 0.8);
}

static void onChangedNoCall(uiColorButton *button, void *data)
{
	function_called();
}

static void colorButtonSetColorNoCallback(void **state)
{
	uiColorButton *button = uiColorButtonFromState(state);

	uiColorButtonOnChanged(button, onChangedNoCall, NULL);
	uiColorButtonSetColor(button, 0.1, 0.2, 0.3, 0.4);
	uiColorButtonSetColor(button, 0.9, 0.8, 0.7, 0.6);
}

#define colorButtonUnitTest(f) cmocka_unit_test_setup_teardown((f), \
	colorButtonSetup, unitTestTeardown)

int colorButtonRunUnitTests(void)
{
	const struct CMUnitTest tests[] = {
		colorButtonUnitTest(colorButtonNew),
		colorButtonUnitTest(colorButtonSetColor),
		colorButtonUnitTest(colorButtonSetColorNoCallback),
	};

	return cmocka_run_group_tests_name("uiColorButton", tests,
		unitTestsSetup, unitTestsTeardown);
}
