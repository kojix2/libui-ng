#include "unit.h"

#define uiMultilineEntryFromState(s) \
	((uiMultilineEntry *) (((struct state *) *(s))->c))

static int multilineEntrySetup(void **state)
{
	struct state *s;

	if (unitTestSetup(state) != 0)
		return -1;
	s = (struct state *) *state;
	s->c = uiControl(uiNewMultilineEntry());
	return 0;
}

static int nonWrappingMultilineEntrySetup(void **state)
{
	struct state *s;

	if (unitTestSetup(state) != 0)
		return -1;
	s = (struct state *) *state;
	s->c = uiControl(uiNewNonWrappingMultilineEntry());
	return 0;
}

static void multilineEntryNew(void **state)
{
	uiMultilineEntry *entry = uiMultilineEntryFromState(state);
	char *text;

	assert_non_null(entry);
	text = uiMultilineEntryText(entry);
	assert_string_equal(text, "");
	uiFreeText(text);
	assert_int_equal(uiMultilineEntryReadOnly(entry), 0);
}

static void multilineEntrySetAndAppendText(void **state)
{
	uiMultilineEntry *entry = uiMultilineEntryFromState(state);
	char *text;

	uiMultilineEntrySetText(entry, "First");
	uiMultilineEntryAppend(entry, "\nSecond");
	text = uiMultilineEntryText(entry);
	assert_string_equal(text, "First\nSecond");
	uiFreeText(text);
}

static void multilineEntrySetReadOnly(void **state)
{
	uiMultilineEntry *entry = uiMultilineEntryFromState(state);

	uiMultilineEntrySetReadOnly(entry, 1);
	assert_int_equal(uiMultilineEntryReadOnly(entry), 1);
	uiMultilineEntrySetReadOnly(entry, 0);
	assert_int_equal(uiMultilineEntryReadOnly(entry), 0);
}

static void onChangedNoCall(uiMultilineEntry *entry, void *data)
{
	function_called();
}

static void multilineEntryProgrammaticChangesDoNotCallback(void **state)
{
	uiMultilineEntry *entry = uiMultilineEntryFromState(state);

	uiMultilineEntryOnChanged(entry, onChangedNoCall, NULL);
	uiMultilineEntrySetText(entry, "First");
	uiMultilineEntryAppend(entry, "Second");
}

#define wrappingUnitTest(f) UNIT_TEST_NAMED("wrapping/" #f, (f), \
		multilineEntrySetup, unitTestTeardown)
#define nonWrappingUnitTest(f) UNIT_TEST_NAMED("nonwrapping/" #f, (f), \
		nonWrappingMultilineEntrySetup, unitTestTeardown)
#define multilineEntryUnitTests(f) wrappingUnitTest(f), nonWrappingUnitTest(f)

int multilineEntryRunUnitTests(void)
{
	const struct CMUnitTest tests[] = {
		multilineEntryUnitTests(multilineEntryNew),
		multilineEntryUnitTests(multilineEntrySetAndAppendText),
		multilineEntryUnitTests(multilineEntrySetReadOnly),
		multilineEntryUnitTests(multilineEntryProgrammaticChangesDoNotCallback),
	};

	return cmocka_run_group_tests_name("uiMultilineEntry", tests,
		unitTestsSetup, unitTestsTeardown);
}
