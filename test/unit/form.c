#include "unit.h"

#define uiFormFromState(s) ((uiForm *) (((struct state *) *(s))->c))

static int formSetup(void **state)
{
	struct state *s;

	if (unitTestSetup(state) != 0)
		return -1;
	s = (struct state *) *state;
	s->c = uiControl(uiNewForm());
	return 0;
}

static void formNew(void **state)
{
	uiForm *form = uiFormFromState(state);

	assert_non_null(form);
	assert_int_equal(uiFormNumChildren(form), 0);
	assert_int_equal(uiFormPadded(form), 0);
}

static void formSetPadded(void **state)
{
	uiForm *form = uiFormFromState(state);

	uiFormSetPadded(form, 1);
	assert_int_equal(uiFormPadded(form), 1);
	uiFormSetPadded(form, 0);
	assert_int_equal(uiFormPadded(form), 0);
}

static void formAppendAndDelete(void **state)
{
	uiForm *form = uiFormFromState(state);
	uiControl *first;
	uiControl *second;

	first = uiControl(uiNewEntry());
	second = uiControl(uiNewMultilineEntry());
	uiFormAppend(form, "First", first, 0);
	uiFormAppend(form, "Second", second, 1);
	assert_int_equal(uiFormNumChildren(form), 2);
	assert_ptr_equal(uiControlParent(first), uiControl(form));
	assert_ptr_equal(uiControlParent(second), uiControl(form));

	uiFormDelete(form, 0);
	assert_int_equal(uiFormNumChildren(form), 1);
	assert_null(uiControlParent(first));
	uiControlDestroy(first);
}

static void formChildVisibilityDoesNotCrash(void **state)
{
	uiForm *form = uiFormFromState(state);
	uiControl *child;

	child = uiControl(uiNewButton("child"));
	uiFormAppend(form, "Visible", child, 0);
	uiControlHide(child);
	uiControlShow(child);
}

#define formUnitTest(f) cmocka_unit_test_setup_teardown((f), \
	formSetup, unitTestTeardown)

int formRunUnitTests(void)
{
	const struct CMUnitTest tests[] = {
		formUnitTest(formNew),
		formUnitTest(formSetPadded),
		formUnitTest(formAppendAndDelete),
		formUnitTest(formChildVisibilityDoesNotCrash),
	};

	return cmocka_run_group_tests_name("uiForm", tests,
		unitTestsSetup, unitTestsTeardown);
}
