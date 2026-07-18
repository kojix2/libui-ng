#include "unit.h"

#define uiTabPtrFromState(s) uiControlPtrFromState(uiTab, s)

static void onSelectedNoCall(uiTab *t, void *data)
{
	function_called();
}

static void tabSetSelectedNoCallback(void **state)
{
	uiTab **t = uiTabPtrFromState(state);

	*t = uiNewTab();
	uiTabAppend(*t, "Page 0", uiControl(uiNewLabel("Page 0")));
	uiTabAppend(*t, "Page 1", uiControl(uiNewLabel("Page 1")));
	uiTabOnSelected(*t, onSelectedNoCall, NULL);
	uiTabSetSelected(*t, 1);
	assert_int_equal(uiTabSelected(*t), 1);
	uiTabSetSelected(*t, 1);
	assert_int_equal(uiTabSelected(*t), 1);
	uiTabSetSelected(*t, 0);
	assert_int_equal(uiTabSelected(*t), 0);
}

#define tabUnitTest(f) cmocka_unit_test_setup_teardown((f), \
		unitTestSetup, unitTestTeardown)

int tabRunUnitTests(void)
{
	const struct CMUnitTest tests[] = {
		tabUnitTest(tabSetSelectedNoCallback),
	};

	return cmocka_run_group_tests_name("uiTab", tests, unitTestsSetup, unitTestsTeardown);
}
