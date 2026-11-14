#include "unit.h"
#ifdef _WIN32
#include <windows.h>
#endif

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

static void formChildMinimumSizeCanChangeAfterAppend(void **state)
{
	uiForm *form = uiFormFromState(state);
	uiControl *child;

	child = uiControl(uiNewEntry());
	uiFormAppend(form, "Minimum", child, 0);
	uiControlSetMinimumSize(child, 60, -1);
	uiControlHide(child);
	uiControlShow(child);
	uiControlSetMinimumSize(child, -1, -1);
	uiFormDelete(form, 0);
	uiControlDestroy(child);
}

#ifdef _WIN32
static void formLabelsFollowEnabledState(void **state)
{
	struct state *s = *state;
	uiForm *form = uiFormFromState(state);
	uiBox *box;
	HWND hwnd, firstLabel, secondLabel;

	box = uiNewVerticalBox();
	uiBoxAppend(box, uiControl(form), 1);
	s->c = uiControl(box);
	uiWindowSetChild(s->w, s->c);
	uiFormAppend(form, "First", uiControl(uiNewButton("first")), 0);
	hwnd = (HWND) uiControlHandle(uiControl(form));
	firstLabel = GetDlgItem(hwnd, 100);
	assert_non_null(firstLabel);
	assert_true(IsWindowEnabled(firstLabel));
	uiControlDisable(uiControl(form));
	assert_false(IsWindowEnabled(firstLabel));
	uiFormAppend(form, "Second", uiControl(uiNewButton("second")), 0);
	secondLabel = GetDlgItem(hwnd, 102);
	assert_non_null(secondLabel);
	assert_false(IsWindowEnabled(secondLabel));
	uiControlEnable(uiControl(form));
	assert_true(IsWindowEnabled(firstLabel));
	assert_true(IsWindowEnabled(secondLabel));
	uiControlDisable(uiControl(box));
	assert_false(IsWindowEnabled(firstLabel));
	assert_false(IsWindowEnabled(secondLabel));
	uiControlEnable(uiControl(box));
	assert_true(IsWindowEnabled(firstLabel));
	assert_true(IsWindowEnabled(secondLabel));
}
#endif

#define formUnitTest(f) cmocka_unit_test_setup_teardown((f), \
	formSetup, unitTestTeardown)

int formRunUnitTests(void)
{
	const struct CMUnitTest tests[] = {
		formUnitTest(formNew),
		formUnitTest(formSetPadded),
		formUnitTest(formAppendAndDelete),
		formUnitTest(formChildVisibilityDoesNotCrash),
		formUnitTest(formChildMinimumSizeCanChangeAfterAppend),
#ifdef _WIN32
		cmocka_unit_test_setup_teardown(formLabelsFollowEnabledState,
			formSetup, unitTestTeardownHidden),
#endif
	};

	return cmocka_run_group_tests_name("uiForm", tests,
		unitTestsSetup, unitTestsTeardown);
}
