#include "unit.h"

#define uiGroupFromState(s) ((uiGroup *) (((struct state *) *(s))->c))

static int groupSetup(void **state)
{
	struct state *s;

	if (unitTestSetup(state) != 0)
		return -1;
	s = (struct state *) *state;
	s->c = uiControl(uiNewGroup("Title"));
	return 0;
}

static void groupNew(void **state)
{
	uiGroup *group = uiGroupFromState(state);
	char *title;

	assert_non_null(group);
	title = uiGroupTitle(group);
	assert_string_equal(title, "Title");
	uiFreeText(title);
	assert_int_equal(uiGroupMargined(group), 0);
}

static void groupSetTitle(void **state)
{
	uiGroup *group = uiGroupFromState(state);
	char *title;

	uiGroupSetTitle(group, "Changed");
	title = uiGroupTitle(group);
	assert_string_equal(title, "Changed");
	uiFreeText(title);
}

static void groupSetMargined(void **state)
{
	uiGroup *group = uiGroupFromState(state);

	uiGroupSetMargined(group, 1);
	assert_int_equal(uiGroupMargined(group), 1);
	uiGroupSetMargined(group, 0);
	assert_int_equal(uiGroupMargined(group), 0);
}

static void groupSetChild(void **state)
{
	uiGroup *group = uiGroupFromState(state);
	uiControl *child;

	child = uiControl(uiNewButton("child"));
	uiGroupSetChild(group, child);
	assert_ptr_equal(uiControlParent(child), uiControl(group));
	uiGroupSetMargined(group, 1);
	assert_int_equal(uiGroupMargined(group), 1);
}

#define groupUnitTest(f) cmocka_unit_test_setup_teardown((f), \
	groupSetup, unitTestTeardown)

int groupRunUnitTests(void)
{
	const struct CMUnitTest tests[] = {
		groupUnitTest(groupNew),
		groupUnitTest(groupSetTitle),
		groupUnitTest(groupSetMargined),
		groupUnitTest(groupSetChild),
	};

	return cmocka_run_group_tests_name("uiGroup", tests,
		unitTestsSetup, unitTestsTeardown);
}
