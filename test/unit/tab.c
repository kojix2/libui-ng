#include "unit.h"

#define uiTabPtrFromState(s) uiControlPtrFromState(uiTab, s)

static void onSelectedNoCall(uiTab *t, void *data)
{
	function_called();
}

static void tabNew(void **state)
{
	uiTab **t = uiTabPtrFromState(state);

	*t = uiNewTab();
	assert_non_null(*t);
	assert_int_equal(uiTabNumPages(*t), 0);
}

static void tabAppendInsertDelete(void **state)
{
	uiTab **t = uiTabPtrFromState(state);
	uiControl *first;
	uiControl *middle;
	uiControl *last;

	*t = uiNewTab();
	first = uiControl(uiNewLabel("first"));
	middle = uiControl(uiNewLabel("middle"));
	last = uiControl(uiNewLabel("last"));
	uiTabAppend(*t, "First", first);
	uiTabAppend(*t, "Last", last);
	uiTabInsertAt(*t, "Middle", 1, middle);
	assert_int_equal(uiTabNumPages(*t), 3);
	assert_ptr_equal(uiControlParent(first), uiControl(*t));
	assert_ptr_equal(uiControlParent(middle), uiControl(*t));
	assert_ptr_equal(uiControlParent(last), uiControl(*t));

	uiTabDelete(*t, 1);
	assert_int_equal(uiTabNumPages(*t), 2);
	assert_null(uiControlParent(middle));
	uiControlDestroy(middle);
}

static void tabMargined(void **state)
{
	uiTab **t = uiTabPtrFromState(state);

	*t = uiNewTab();
	uiTabAppend(*t, "Page", uiControl(uiNewLabel("content")));
	assert_int_equal(uiTabMargined(*t, 0), 0);
	uiTabSetMargined(*t, 0, 1);
	assert_int_equal(uiTabMargined(*t, 0), 1);
	uiTabSetMargined(*t, 0, 0);
	assert_int_equal(uiTabMargined(*t, 0), 0);
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
		tabUnitTest(tabNew),
		tabUnitTest(tabAppendInsertDelete),
		tabUnitTest(tabMargined),
		tabUnitTest(tabSetSelectedNoCallback),
	};

	return cmocka_run_group_tests_name("uiTab", tests, unitTestsSetup, unitTestsTeardown);
}
