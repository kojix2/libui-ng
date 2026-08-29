#include "unit.h"

#define uiEditableComboboxFromState(s) \
	((uiEditableCombobox *) (((struct state *) *(s))->c))

static int editableComboboxSetup(void **state)
{
	struct state *s;

	if (unitTestSetup(state) != 0)
		return -1;
	s = (struct state *) *state;
	s->c = uiControl(uiNewEditableCombobox());
	return 0;
}

static void editableComboboxNew(void **state)
{
	assert_non_null(uiEditableComboboxFromState(state));
}

static void editableComboboxSetText(void **state)
{
	uiEditableCombobox *combobox = uiEditableComboboxFromState(state);
	char *text;

	uiEditableComboboxSetText(combobox, "Text");
	text = uiEditableComboboxText(combobox);
	assert_string_equal(text, "Text");
	uiFreeText(text);

	uiEditableComboboxSetText(combobox, "");
	text = uiEditableComboboxText(combobox);
	assert_string_equal(text, "");
	uiFreeText(text);
}

static void editableComboboxAppend(void **state)
{
	uiEditableCombobox *combobox = uiEditableComboboxFromState(state);

	uiEditableComboboxAppend(combobox, "One");
	uiEditableComboboxAppend(combobox, "Two");
	uiEditableComboboxSetText(combobox, "Two");
}

static void onChangedNoCall(uiEditableCombobox *combobox, void *data)
{
	function_called();
}

static void editableComboboxSetTextNoCallback(void **state)
{
	uiEditableCombobox *combobox = uiEditableComboboxFromState(state);

	uiEditableComboboxOnChanged(combobox, onChangedNoCall, NULL);
	uiEditableComboboxSetText(combobox, "First");
	uiEditableComboboxSetText(combobox, "Second");
}

#define editableComboboxUnitTest(f) cmocka_unit_test_setup_teardown((f), \
	editableComboboxSetup, unitTestTeardown)

int editableComboboxRunUnitTests(void)
{
	const struct CMUnitTest tests[] = {
		editableComboboxUnitTest(editableComboboxNew),
		editableComboboxUnitTest(editableComboboxSetText),
		editableComboboxUnitTest(editableComboboxAppend),
		editableComboboxUnitTest(editableComboboxSetTextNoCallback),
	};

	return cmocka_run_group_tests_name("uiEditableCombobox", tests,
		unitTestsSetup, unitTestsTeardown);
}
