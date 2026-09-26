#include "unit.h"

static void tooltipEntryLifecycle(void **state)
{
	uiEntry **entry = uiControlPtrFromState(uiEntry, state);

	*entry = uiNewEntry();
	uiControlSetTooltip(uiControl(*entry), "first tooltip");
	uiControlSetTooltip(uiControl(*entry), "日本語 tooltip\nsecond line");
	uiControlSetTooltip(uiControl(*entry), "");
	uiControlSetTooltip(uiControl(*entry), NULL);
	uiControlSetTooltip(uiControl(*entry), NULL);
	uiControlSetTooltip(uiControl(*entry), "restored tooltip");
}

static void tooltipCompositeControls(void **state)
{
	uiBox **box = uiControlPtrFromState(uiBox, state);
	uiCombobox *combobox;
	uiEditableCombobox *editable;
	uiRadioButtons *radio;
	uiSpinbox *spinbox;

	*box = uiNewVerticalBox();

	combobox = uiNewCombobox();
	uiComboboxAppend(combobox, "item");
	uiControlSetTooltip(uiControl(combobox), "combobox tooltip");
	uiBoxAppend(*box, uiControl(combobox), 0);

	editable = uiNewEditableCombobox();
	uiEditableComboboxAppend(editable, "item");
	uiControlSetTooltip(uiControl(editable), "editable tooltip");
	uiBoxAppend(*box, uiControl(editable), 0);

	spinbox = uiNewSpinbox(0, 100);
	uiControlSetTooltip(uiControl(spinbox), "spinbox tooltip");
	uiBoxAppend(*box, uiControl(spinbox), 0);

	radio = uiNewRadioButtons();
	uiControlSetTooltip(uiControl(radio), "radio tooltip");
	uiRadioButtonsAppend(radio, "added after tooltip");
	uiRadioButtonsAppend(radio, "second item");
	uiControlSetTooltip(uiControl(radio), "updated radio tooltip");
	uiBoxAppend(*box, uiControl(radio), 0);
}

static void tooltipContainerDoesNotOwnChildTooltip(void **state)
{
	uiBox **box = uiControlPtrFromState(uiBox, state);
	uiEntry *entry;

	*box = uiNewVerticalBox();
	uiControlSetTooltip(uiControl(*box), "container surface");
	entry = uiNewEntry();
	uiControlSetTooltip(uiControl(entry), "child tooltip");
	uiBoxAppend(*box, uiControl(entry), 0);
	uiControlSetTooltip(uiControl(*box), NULL);
}

static void tooltipWindow(void **state)
{
	struct state *s = *state;
	uiEntry **entry = uiControlPtrFromState(uiEntry, state);

	uiControlSetTooltip(uiControl(s->w), "window surface");
	uiControlSetTooltip(uiControl(s->w), NULL);
	*entry = uiNewEntry();
}

static void tooltipSliderRestoresValueSetting(void **state)
{
	uiSlider **slider = uiControlPtrFromState(uiSlider, state);

	*slider = uiNewSlider(0, 100);
	assert_int_equal(uiSliderHasToolTip(*slider), 1);

	uiControlSetTooltip(uiControl(*slider), "descriptive tooltip");
	assert_int_equal(uiSliderHasToolTip(*slider), 1);
	uiSliderSetHasToolTip(*slider, 0);
	assert_int_equal(uiSliderHasToolTip(*slider), 0);
	uiSliderSetValue(*slider, 50);
	uiControlSetTooltip(uiControl(*slider), NULL);
	assert_int_equal(uiSliderHasToolTip(*slider), 0);

	uiSliderSetHasToolTip(*slider, 1);
	uiControlSetTooltip(uiControl(*slider), "replacement");
	uiControlSetTooltip(uiControl(*slider), "replacement 2");
	uiControlSetTooltip(uiControl(*slider), NULL);
	assert_int_equal(uiSliderHasToolTip(*slider), 1);
}

#define tooltipUnitTest(f) cmocka_unit_test_setup_teardown((f), \
	unitTestSetup, unitTestTeardown)

int tooltipRunUnitTests(void)
{
	const struct CMUnitTest tests[] = {
		tooltipUnitTest(tooltipEntryLifecycle),
		tooltipUnitTest(tooltipCompositeControls),
		tooltipUnitTest(tooltipContainerDoesNotOwnChildTooltip),
		tooltipUnitTest(tooltipWindow),
		tooltipUnitTest(tooltipSliderRestoresValueSetting),
	};

	return cmocka_run_group_tests_name("uiControlSetTooltip", tests,
		unitTestsSetup, unitTestsTeardown);
}
