#include "unit.h"

#define uiLabelPtrFromState(s) uiControlPtrFromState(uiLabel, s)

static void tooltipEntry(void **state)
{
	uiEntry **e = uiControlPtrFromState(uiEntry, state);
	*e = uiNewEntry();
	uiControlSetTooltip(uiControl(*e), "tooltip");
}

static void tooltipEntryNull(void **state)
{
	uiEntry **e = uiControlPtrFromState(uiEntry, state);
	*e = uiNewEntry();
	uiControlSetTooltip(uiControl(*e), NULL);
}

static void tooltipSpinbox(void **state)
{
	uiSpinbox **s = uiControlPtrFromState(uiSpinbox, state);
	*s = uiNewSpinbox(0, 100);
	uiControlSetTooltip(uiControl(*s), "tooltip");
}

static void tooltipRadioButtons(void **state)
{
	uiRadioButtons **r = uiControlPtrFromState(uiRadioButtons, state);
	*r = uiNewRadioButtons();
	uiRadioButtonsAppend(*r, "item 1");
	uiRadioButtonsAppend(*r, "item 2");
	uiControlSetTooltip(uiControl(*r), "tooltip");
}

#define tooltipUnitTest(f) cmocka_unit_test_setup_teardown((f), \
		unitTestSetup, unitTestTeardown)

int tooltipRunUnitTests(void)
{
	const struct CMUnitTest tests[] = {
		tooltipUnitTest(tooltipEntry),
		tooltipUnitTest(tooltipEntryNull),
		tooltipUnitTest(tooltipSpinbox),
		tooltipUnitTest(tooltipRadioButtons),
	};

	return cmocka_run_group_tests_name("uiControlSetTooltip", tests, unitTestsSetup, unitTestsTeardown);
}

