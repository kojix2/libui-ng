#include "unit.h"

#if !defined(_WIN32) && !defined(__APPLE__)
#include <gtk/gtk.h>
#endif

#define uiEntryPtrFromState(s) uiControlPtrFromState(uiEntry, s)

static void entryNew(void **state)
{
}

static void entryTextDefault(void **state)
{
	uiEntry **e = uiEntryPtrFromState(state);
	const char *text = "";
	char *rv;

	rv = uiEntryText(*e);
	assert_string_equal(rv, text);
	uiFreeText(rv);
}

static void entrySetText(void **state)
{
	uiEntry **e = uiEntryPtrFromState(state);
	const char *text1 = "Text 1";
	const char *text2 = "Text 2";
	char *rv;

	uiEntrySetText(*e, text1);
	rv = uiEntryText(*e);
	assert_string_equal(rv, text1);
	uiFreeText(rv);
	uiEntrySetText(*e, text2);
	rv = uiEntryText(*e);
	assert_string_equal(rv, text2);
	uiFreeText(rv);
}

static void onChangedNoCall(uiEntry *e, void *data)
{
	function_called();
}

static void entrySetTextNoCallback(void **state)
{

	uiEntry **e = uiEntryPtrFromState(state);

	uiEntryOnChanged(*e, onChangedNoCall, NULL);
	uiEntrySetText(*e, "Text 1");
	uiEntrySetText(*e, "Text 2");
}

#if !defined(_WIN32) && !defined(__APPLE__)
struct searchChangedState {
	int calls;
	int normalize;
};

static void countSearchChanged(uiEntry *e, void *data)
{
	struct searchChangedState *changed = data;

	changed->calls++;
	if (changed->normalize)
		uiEntrySetText(e, "normalized");
}

static void runSearchEventsFor(gint64 milliseconds)
{
	gint64 deadline;

	deadline = g_get_monotonic_time() + milliseconds * 1000;
	while (g_get_monotonic_time() < deadline) {
		while (g_main_context_iteration(NULL, FALSE))
			;
		g_usleep(1000);
	}
}

static void searchEntrySetTextDoesNotNotifyLater(void **state)
{
	uiEntry **e = uiEntryPtrFromState(state);
	struct searchChangedState changed = {0, 1};
	GtkEditable *editable;
	int position;

	uiEntryOnChanged(*e, countSearchChanged, &changed);
	uiEntrySetText(*e, "query");
	runSearchEventsFor(300);
	assert_int_equal(changed.calls, 0);

	editable = GTK_EDITABLE(uiControlHandle(uiControl(*e)));
	position = 0;
	gtk_editable_insert_text(editable, "x", 1, &position);
	runSearchEventsFor(300);
	assert_int_equal(changed.calls, 1);
	runSearchEventsFor(300);
	assert_int_equal(changed.calls, 1);
}

static void searchEntrySetterCancelsPendingUserChange(void **state)
{
	uiEntry **e = uiEntryPtrFromState(state);
	struct searchChangedState changed = {0, 0};
	GtkEditable *editable;
	int position;

	uiEntryOnChanged(*e, countSearchChanged, &changed);
	editable = GTK_EDITABLE(uiControlHandle(uiControl(*e)));
	position = 0;
	gtk_editable_insert_text(editable, "x", 1, &position);
	uiEntrySetText(*e, "query");
	runSearchEventsFor(300);
	assert_int_equal(changed.calls, 0);

	position = 0;
	gtk_editable_insert_text(editable, "y", 1, &position);
	runSearchEventsFor(300);
	assert_int_equal(changed.calls, 1);
}
#endif

static void entryReadOnlyDefault(void **state)
{
	uiEntry **e = uiEntryPtrFromState(state);

	assert_int_equal(uiEntryReadOnly(*e), 0);
}

static void entrySetReadOnly(void **state)
{
	uiEntry **e = uiEntryPtrFromState(state);

	uiEntrySetReadOnly(*e, 1);
	assert_int_equal(uiEntryReadOnly(*e), 1);
	uiEntrySetReadOnly(*e, 0);
	assert_int_equal(uiEntryReadOnly(*e), 0);
}

static int entryTestSetup(void **state)
{
	int rv = unitTestSetup(state);
	if (rv != 0)
		return rv;

	uiEntry **e = uiEntryPtrFromState(state);
	*e = uiNewEntry();

	return 0;
}

static int passwordEntryTestSetup(void **state)
{
	int rv = unitTestSetup(state);
	if (rv != 0)
		return rv;

	uiEntry **e = uiEntryPtrFromState(state);
	*e = uiNewPasswordEntry();

	return 0;
}

static int searchEntryTestSetup(void **state)
{
	int rv = unitTestSetup(state);
	if (rv != 0)
		return rv;

	uiEntry **e = uiEntryPtrFromState(state);
	*e = uiNewSearchEntry();

	return 0;
}

#define entryUnitTest(f) UNIT_TEST_NAMED("uiEntry/" #f, (f), \
		entryTestSetup, unitTestTeardown)

#define passwordEntryUnitTest(f) UNIT_TEST_NAMED("uiPasswordEntry/" #f, (f), \
		passwordEntryTestSetup, unitTestTeardown)

#define searchEntryUnitTest(f) UNIT_TEST_NAMED("uiSearchEntry/" #f, (f), \
		searchEntryTestSetup, unitTestTeardown)

#define entryUnitTests(f) entryUnitTest(f), passwordEntryUnitTest(f), \
		searchEntryUnitTest(f)

int entryRunUnitTests(void)
{
	const struct CMUnitTest tests[] = {
		entryUnitTests(entryNew),
		entryUnitTests(entryTextDefault),
		entryUnitTests(entrySetText),
		entryUnitTests(entrySetTextNoCallback),
#if !defined(_WIN32) && !defined(__APPLE__)
		searchEntryUnitTest(searchEntrySetTextDoesNotNotifyLater),
		searchEntryUnitTest(searchEntrySetterCancelsPendingUserChange),
#endif
		entryUnitTests(entryReadOnlyDefault),
		entryUnitTests(entrySetReadOnly),
	};

	return cmocka_run_group_tests_name("uiEntry", tests, unitTestsSetup, unitTestsTeardown);
}
