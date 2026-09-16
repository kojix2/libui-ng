#include "unit.h"
#include <float.h>
#include <math.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#include <commctrl.h>

static LRESULT CALLBACK countLabelLayout(HWND hwnd, UINT message,
	WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR data)
{
	int *requests = (int *) data;
	if (message == WM_WINDOWPOSCHANGING)
		(*requests)++;
	return DefSubclassProc(hwnd, message, wParam, lParam);
}

static void labelWindowsOnlyRelayoutsWhenMinimumChanges(void **state)
{
	struct state *s = *state;
	uiBox *box = uiNewVerticalBox();
	uiLabel *label = uiNewLabel("Ready");
	HWND native = (HWND) uiControlHandle(uiControl(label));
	HWND parent = (HWND) uiControlHandle(uiControl(s->w));
	int requests = 0;
	RECT before, after, windowBefore, windowAfter;
	char longText[512];

	uiControlSetMinimumSize(uiControl(label), 0, -1);
	uiBoxAppend(box, uiControl(label), 0);
	uiWindowSetChild(s->w, uiControl(box));
	GetWindowRect(native, &before);
	GetWindowRect(parent, &windowBefore);
	assert_true(SetWindowSubclass(native, countLabelLayout, 1, (DWORD_PTR) &requests));
	uiLabelSetText(label, "Ready");
	memset(longText, 'W', sizeof longText - 1);
	longText[sizeof longText - 1] = '\0';
	uiLabelSetText(label, longText);
	assert_int_equal(requests, 0);
	GetWindowRect(parent, &windowAfter);
	assert_true(EqualRect(&windowBefore, &windowAfter));
	uiLabelSetText(label, "First line\nSecond line");
	assert_true(requests > 0);
	GetWindowRect(native, &after);
	assert_true(after.bottom - after.top > before.bottom - before.top);
	RemoveWindowSubclass(native, countLabelLayout, 1);
}
#endif

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
#ifdef _WIN32
		labelUnitTest(labelWindowsOnlyRelayoutsWhenMinimumChanges),
#endif
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
