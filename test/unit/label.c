#include "unit.h"
#include <float.h>
#include <math.h>

#define uiLabelPtrFromState(s) uiControlPtrFromState(uiLabel, s)

static void labelNew(void **state)
{
	uiLabel **l = uiLabelPtrFromState(state);

	*l = uiNewLabel("Text");
}

static void labelNewEmptyString(void **state)
{
	uiLabel **l = uiLabelPtrFromState(state);

	*l = uiNewLabel("");
}

static void labelText(void **state)
{
	uiLabel **l = uiLabelPtrFromState(state);
	const char *text = "Text";
	char *rv;

	*l = uiNewLabel(text);
	rv = uiLabelText(*l);
	assert_string_equal(text, rv);
	uiFreeText(rv);
}

static void labelTextEmptyString(void **state)
{
	uiLabel **l = uiLabelPtrFromState(state);
	const char *text = "";
	char *rv;

	*l = uiNewLabel(text);
	rv = uiLabelText(*l);
	assert_string_equal(text, rv);
	uiFreeText(rv);
}

static void labelSetText(void **state)
{
	uiLabel **l = uiLabelPtrFromState(state);
	const char *text = "SetText";
	char *rv;

	*l = uiNewLabel("Text");
	uiLabelSetText(*l, text);
	rv = uiLabelText(*l);
	assert_string_equal(text, rv);
	uiFreeText(rv);
}

static void labelSetTextEmptyString(void **state)
{
	uiLabel **l = uiLabelPtrFromState(state);
	const char *text = "";
	char *rv;

	*l = uiNewLabel("Text");
	uiLabelSetText(*l, text);
	rv = uiLabelText(*l);
	assert_string_equal(text, rv);
	uiFreeText(rv);
}

static void labelFontSize(void **state)
{
	uiLabel **l = uiLabelPtrFromState(state);
	double size;

	*l = uiNewLabel("Text");
	size = uiLabelFontSize(*l);
	assert_true(size > 0 && size <= DBL_MAX);
}

static void labelSetFontSize(void **state)
{
	uiLabel **l = uiLabelPtrFromState(state);

	*l = uiNewLabel("Text");
	uiLabelSetFontSize(*l, 18.5);
	assert_true(fabs(uiLabelFontSize(*l) - 18.5) < 0.000001);
}

static void labelSetTextPreservesFontSize(void **state)
{
	uiLabel **l = uiLabelPtrFromState(state);

	*l = uiNewLabel("Text");
	uiLabelSetFontSize(*l, 18.5);
	uiLabelSetText(*l, "Changed");
	assert_true(fabs(uiLabelFontSize(*l) - 18.5) < 0.000001);
}

static void labelResetFontSize(void **state)
{
	uiLabel **l = uiLabelPtrFromState(state);
	double initial;

	*l = uiNewLabel("Text");
	initial = uiLabelFontSize(*l);
	uiLabelSetFontSize(*l, initial + 5);
	uiLabelResetFontSize(*l);
	assert_true(fabs(uiLabelFontSize(*l) - initial) < 0.000001);
	uiLabelResetFontSize(*l);
	assert_true(fabs(uiLabelFontSize(*l) - initial) < 0.000001);
}

#define labelUnitTest(f) cmocka_unit_test_setup_teardown((f), \
		unitTestSetup, unitTestTeardown)

int labelRunUnitTests(void)
{
	const struct CMUnitTest tests[] = {
		labelUnitTest(labelNew),
		labelUnitTest(labelNewEmptyString),
		labelUnitTest(labelText),
		labelUnitTest(labelTextEmptyString),
		labelUnitTest(labelSetText),
		labelUnitTest(labelSetTextEmptyString),
		labelUnitTest(labelFontSize),
		labelUnitTest(labelSetFontSize),
		labelUnitTest(labelSetTextPreservesFontSize),
		labelUnitTest(labelResetFontSize),
	};

	return cmocka_run_group_tests_name("uiLabel", tests, unitTestsSetup, unitTestsTeardown);
}
