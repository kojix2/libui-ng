#include <time.h>

#include "unit.h"

#define uiDateTimePickerPtrFromState(s) uiControlPtrFromState(uiDateTimePicker, s)

static void onChangedNoCall(uiDateTimePicker *d, void *data)
{
	function_called();
}

static void dateTimePickerSetTimeNoCallback(void **state)
{
	uiDateTimePicker **d = uiDateTimePickerPtrFromState(state);
	struct tm time = { 0 };

	time.tm_year = 120;
	time.tm_mon = 5;
	time.tm_mday = 15;
	time.tm_hour = 12;
	time.tm_min = 34;
	time.tm_sec = 56;
	time.tm_isdst = -1;

	uiDateTimePickerOnChanged(*d, onChangedNoCall, NULL);
	uiDateTimePickerSetTime(*d, &time);
}

static int dateTimePickerTestSetup(void **state)
{
	int rv = unitTestSetup(state);
	uiDateTimePicker **d;

	if (rv != 0)
		return rv;
	d = uiDateTimePickerPtrFromState(state);
	*d = uiNewDateTimePicker();
	return 0;
}

static int datePickerTestSetup(void **state)
{
	int rv = unitTestSetup(state);
	uiDateTimePicker **d;

	if (rv != 0)
		return rv;
	d = uiDateTimePickerPtrFromState(state);
	*d = uiNewDatePicker();
	return 0;
}

static int timePickerTestSetup(void **state)
{
	int rv = unitTestSetup(state);
	uiDateTimePicker **d;

	if (rv != 0)
		return rv;
	d = uiDateTimePickerPtrFromState(state);
	*d = uiNewTimePicker();
	return 0;
}

int dateTimePickerRunUnitTests(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(dateTimePickerSetTimeNoCallback,
			dateTimePickerTestSetup, unitTestTeardown),
		cmocka_unit_test_setup_teardown(dateTimePickerSetTimeNoCallback,
			datePickerTestSetup, unitTestTeardown),
		cmocka_unit_test_setup_teardown(dateTimePickerSetTimeNoCallback,
			timePickerTestSetup, unitTestTeardown),
	};

	return cmocka_run_group_tests_name("uiDateTimePicker", tests,
		unitTestsSetup, unitTestsTeardown);
}
