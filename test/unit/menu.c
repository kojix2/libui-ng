#include "unit.h"

int menuTestSetup(void **state);
int menuTestTeardown(void **state);

static void menuNewDoesNotCrash(void **state)
{
	uiMenu *m;

	m = uiNewMenu("Menu");
	(void)m;
}

static void menuNewAcrossInitCyclesDoesNotCrash(void **state)
{
	uiMenu *m;

	menuTestSetup(state);
	m = uiNewMenu("Menu 1");
	menuTestTeardown(state);

	menuTestSetup(state);
	m = uiNewMenu("Menu 2");
	menuTestTeardown(state);

	(void)m;
}

static void menuNewEmptyStringDoesNotCrash(void **_state)
{
	uiMenu *m;

	m = uiNewMenu("");
	(void)m;
}

static void menuAppendItemDoesNotCrash(void **_state)
{
	uiMenu *m;

	m = uiNewMenu("Menu");
	uiMenuAppendItem(m, "Item");
}

static void menuAppendItemsDoesNotCrash(void **_state)
{
	uiMenu *m;

	m = uiNewMenu("Menu");
	uiMenuAppendItem(m, "Item 1");
	uiMenuAppendItem(m, "Item 2");
}

static void menuAppendCheckItemDoesNotCrash(void **_state)
{
	uiMenu *m;

	m = uiNewMenu("Menu");
	uiMenuAppendCheckItem(m, "Item");
}

static void menuAppendCheckItemsDoesNotCrash(void **_state)
{
	uiMenu *m;

	m = uiNewMenu("Menu");
	uiMenuAppendCheckItem(m, "Item 1");
	uiMenuAppendCheckItem(m, "Item 2");
}

static void menuAppendAboutItemDoesNotCrash(void **_state)
{
	uiMenu *m;

	m = uiNewMenu("Menu");
	uiMenuAppendAboutItem(m);
}

static void menuAppendPreferencesItemDoesNotCrash(void **_state)
{
	uiMenu *m;

	m = uiNewMenu("Menu");
	uiMenuAppendPreferencesItem(m);
}

static void menuAppendQuitItemDoesNotCrash(void **_state)
{
	uiMenu *m;

	m = uiNewMenu("Menu");
	uiMenuAppendQuitItem(m);
}

static void menuAppendSeparatorDoesNotCrash(void **_state)
{
	uiMenu *m;

	m = uiNewMenu("Menu");
	uiMenuAppendSeparator(m);
}

static void menuAppendMixedItemsDoesNotCrash(void **_state)
{
	uiMenu *m;

	m = uiNewMenu("Menu");
	uiMenuAppendItem(m, "Item");
	uiMenuAppendSeparator(m);
	uiMenuAppendCheckItem(m, "Check Item");
	uiMenuAppendAboutItem(m);
	uiMenuAppendPreferencesItem(m);
	uiMenuAppendQuitItem(m);
}

static void menuItemEnableDoesNotCrash(void **_state)
{
	uiMenu *m;
	uiMenuItem *i;

	m = uiNewMenu("Menu");
	i = uiMenuAppendItem(m, "Item");
	uiMenuItemEnable(i);
}

static void menuItemDisableDoesNotCrash(void **_state)
{
	uiMenu *m;
	uiMenuItem *i;

	m = uiNewMenu("Menu");
	i = uiMenuAppendItem(m, "Item");
	uiMenuItemDisable(i);
}

static void menuItemCheckedDefaultFalse(void **_state)
{
	uiMenu *m;
	uiMenuItem *i;

	m = uiNewMenu("Menu");
	i = uiMenuAppendCheckItem(m, "Item");
	assert_int_equal(uiMenuItemChecked(i), 0);
}

static void menuItemSetChecked(void **_state)
{
	uiMenu *m;
	uiMenuItem *i;

	m = uiNewMenu("Menu");
	i = uiMenuAppendCheckItem(m, "Item");
	uiMenuItemSetChecked(i, 1);
	assert_int_equal(uiMenuItemChecked(i), 1);
	uiMenuItemSetChecked(i, 0);
	assert_int_equal(uiMenuItemChecked(i), 0);
}

static void onClickedNoCall(uiMenuItem *i, uiWindow *w, void *data)
{
	function_called();
}

static void menuItemRegisterOnClickedDoesNotInvokeCallback(void **_state)
{
	uiMenu *m;
	uiMenuItem *i;

	m = uiNewMenu("Menu");
	i = uiMenuAppendItem(m, "Item");
	uiMenuItemOnClicked(i, onClickedNoCall, NULL);
}

int menuTestSetup(void **_state)
{
	uiInitOptions o = {0};

	assert_null(uiInit(&o));
	return 0;
}

int menuTestTeardown(void **_state)
{
	struct state *state = *_state;

	state->w = uiNewWindow("Menu Test", UNIT_TEST_WINDOW_WIDTH, UNIT_TEST_WINDOW_HEIGHT, 1);
	uiWindowOnClosing(state->w, unitWindowOnClosingQuit, NULL);

	uiControlShow(uiControl(state->w));
	uiMainSteps();
	uiMainStep(1);
	uiControlDestroy(uiControl(state->w));
	uiUninit();
	return 0;
}

#define menuUnitTest(f) cmocka_unit_test_setup_teardown((f), \
		menuTestSetup, menuTestTeardown)

int menuRunUnitTests(void)
{
	const struct CMUnitTest tests[] = {
		menuUnitTest(menuNewDoesNotCrash),
		cmocka_unit_test(menuNewAcrossInitCyclesDoesNotCrash),
		menuUnitTest(menuNewEmptyStringDoesNotCrash),
		menuUnitTest(menuAppendItemDoesNotCrash),
		menuUnitTest(menuAppendItemsDoesNotCrash),
		menuUnitTest(menuAppendCheckItemDoesNotCrash),
		menuUnitTest(menuAppendCheckItemsDoesNotCrash),
		menuUnitTest(menuAppendAboutItemDoesNotCrash),
		menuUnitTest(menuAppendPreferencesItemDoesNotCrash),
		menuUnitTest(menuAppendQuitItemDoesNotCrash),
		menuUnitTest(menuAppendSeparatorDoesNotCrash),
		menuUnitTest(menuAppendMixedItemsDoesNotCrash),
		menuUnitTest(menuItemEnableDoesNotCrash),
		menuUnitTest(menuItemDisableDoesNotCrash),
		menuUnitTest(menuItemCheckedDefaultFalse),
		menuUnitTest(menuItemSetChecked),
		menuUnitTest(menuItemRegisterOnClickedDoesNotInvokeCallback),
	};

	return cmocka_run_group_tests_name("uiMenu", tests, unitTestsSetup, unitTestsTeardown);
}
