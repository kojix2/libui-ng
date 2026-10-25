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

#if !defined(_WIN32) && !defined(__APPLE__)
struct tabSelectionState {
	int calls;
	int pages;
	int selected;
	int margin;
};

static void onSelectedConsistent(uiTab *t, void *data)
{
	struct tabSelectionState *selection = data;

	selection->calls++;
	assert_int_equal(uiTabNumPages(t), selection->pages);
	assert_int_equal(uiTabSelected(t), selection->selected);
	if (selection->selected >= 0)
		assert_int_equal(uiTabMargined(t, selection->selected), selection->margin);
}

static void tabStructureSelectionConsistent(void **state)
{
	uiTab **t = uiTabPtrFromState(state);
	struct tabSelectionState selection = {0, 1, 0, 0};
	uiControl *first;
	uiControl *second;

	*t = uiNewTab();
	uiTabOnSelected(*t, onSelectedConsistent, &selection);
	first = uiControl(uiNewLabel("first"));
	second = uiControl(uiNewLabel("second"));
	uiTabAppend(*t, "First", first);
	assert_int_equal(selection.calls, 1);
	uiTabSetMargined(*t, 0, 1);
	uiTabAppend(*t, "Second", second);
	assert_int_equal(selection.calls, 1);

	selection.pages = 1;
	selection.margin = 0;
	uiTabDelete(*t, 0);
	assert_int_equal(selection.calls, 2);
	uiControlDestroy(first);

	selection.pages = 0;
	selection.selected = -1;
	uiTabDelete(*t, 0);
	assert_int_equal(selection.calls, 3);
	uiControlDestroy(second);

	selection.pages = 1;
	selection.selected = 0;
	uiTabAppend(*t, "Again", uiControl(uiNewLabel("again")));
	assert_int_equal(selection.calls, 4);
}

static void tabInsertBeforeSelectionDoesNotNotify(void **state)
{
	uiTab **t = uiTabPtrFromState(state);
	struct tabSelectionState selection = {0, 0, 0, 0};
	uiControl *inserted;

	*t = uiNewTab();
	uiTabAppend(*t, "First", uiControl(uiNewLabel("first")));
	uiTabAppend(*t, "Second", uiControl(uiNewLabel("second")));
	uiTabSetSelected(*t, 1);
	uiTabOnSelected(*t, onSelectedConsistent, &selection);
	inserted = uiControl(uiNewLabel("inserted"));
	uiTabInsertAt(*t, "Inserted", 0, inserted);
	assert_int_equal(uiTabSelected(*t), 2);
	assert_int_equal(selection.calls, 0);
	uiTabDelete(*t, 0);
	assert_int_equal(uiTabSelected(*t), 1);
	assert_int_equal(selection.calls, 0);
	uiControlDestroy(inserted);
}

static void onSelectedReenter(uiTab *t, void *data)
{
	int *calls = data;

	(*calls)++;
	assert_int_equal(uiTabNumPages(t), 1);
	assert_int_equal(uiTabSelected(t), 0);
	uiTabAppend(t, "Nested", uiControl(uiNewLabel("nested")));
}

static void tabSelectionCanReenter(void **state)
{
	uiTab **t = uiTabPtrFromState(state);
	int calls = 0;

	*t = uiNewTab();
	uiTabOnSelected(*t, onSelectedReenter, &calls);
	uiTabAppend(*t, "First", uiControl(uiNewLabel("first")));
	assert_int_equal(calls, 1);
	assert_int_equal(uiTabNumPages(*t), 2);
}

static void onSelectedDestroy(uiTab *t, void *data)
{
	uiTab **owner = data;

	assert_int_equal(uiTabNumPages(t), 1);
	uiControlDestroy(uiControl(t));
	*owner = NULL;
}

static void tabSelectionCanDestroy(void **state)
{
	uiTab **t = uiTabPtrFromState(state);

	*t = uiNewTab();
	uiTabOnSelected(*t, onSelectedDestroy, t);
	uiTabAppend(*t, "First", uiControl(uiNewLabel("first")));
	assert_null(*t);
}
#endif

#define tabUnitTest(f) cmocka_unit_test_setup_teardown((f), \
		unitTestSetup, unitTestTeardown)

int tabRunUnitTests(void)
{
	const struct CMUnitTest tests[] = {
		tabUnitTest(tabNew),
		tabUnitTest(tabAppendInsertDelete),
		tabUnitTest(tabMargined),
		tabUnitTest(tabSetSelectedNoCallback),
#if !defined(_WIN32) && !defined(__APPLE__)
		tabUnitTest(tabStructureSelectionConsistent),
		tabUnitTest(tabInsertBeforeSelectionDoesNotNotify),
		tabUnitTest(tabSelectionCanReenter),
		tabUnitTest(tabSelectionCanDestroy),
#endif
	};

	return cmocka_run_group_tests_name("uiTab", tests, unitTestsSetup, unitTestsTeardown);
}
