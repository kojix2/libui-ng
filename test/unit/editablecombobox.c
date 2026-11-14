#include "unit.h"
#ifdef _WIN32
#include <windows.h>
#include <string.h>
#endif

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

#ifdef _WIN32
static void editableComboboxLongUserInput(void **state)
{
	struct state *s = *state;
	uiEditableCombobox *combobox = uiEditableComboboxFromState(state);
	WCHAR input[40001];
	HWND edit;
	char *actual;
	size_t length;
	int i;

	uiWindowSetChild(s->w, uiControl(combobox));
	edit = FindWindowExW((HWND) uiControlHandle(uiControl(combobox)), NULL, L"Edit", NULL);
	assert_non_null(edit);
	for (i = 0; i < 40000; i++)
		input[i] = L'x';
	input[40000] = 0;
	SendMessageW(edit, EM_REPLACESEL, TRUE, (LPARAM) input);
	actual = uiEditableComboboxText(combobox);
	length = strlen(actual);
	uiFreeText(actual);
	assert_int_equal(length, 40000);
}
#endif

#define editableComboboxUnitTest(f) cmocka_unit_test_setup_teardown((f), \
	editableComboboxSetup, unitTestTeardown)

int editableComboboxRunUnitTests(void)
{
	const struct CMUnitTest tests[] = {
		editableComboboxUnitTest(editableComboboxNew),
		editableComboboxUnitTest(editableComboboxSetText),
		editableComboboxUnitTest(editableComboboxAppend),
		editableComboboxUnitTest(editableComboboxSetTextNoCallback),
#ifdef _WIN32
		UNIT_TEST_NAMED("editableComboboxLongUserInput", editableComboboxLongUserInput,
			editableComboboxSetup, unitTestTeardownHidden),
#endif
	};

	return cmocka_run_group_tests_name("uiEditableCombobox", tests,
		unitTestsSetup, unitTestsTeardown);
}
