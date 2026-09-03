#include "unit.h"

static void versionIsAvailableWithoutInitialization(void **state)
{
	const char *version = uiVersion();

	assert_non_null(version);
	assert_true(version[0] != '\0');
	assert_ptr_equal(uiVersion(), version);
}

static void initUninit(void **state)
{
	uiInitOptions o = {0};

	assert_null(uiInit(&o));
	uiUninit();
}

static void initUninitTwice(void **state)
{
	uiInitOptions o = {0};

	assert_null(uiInit(&o));
	uiUninit();

	assert_null(uiInit(&o));
	uiUninit();
}

#if !defined(_WIN32)
static void queuedCallback(void *data)
{
	int *calls = data;

	(*calls)++;
}

static void queuedCallbackAndQuit(void *data)
{
	queuedCallback(data);
	uiQuit();
}

static void queuedCallbacksDoNotSurviveUninit(void **state)
{
	uiInitOptions o = {0};
	int calls = 0;

	assert_null(uiInit(&o));
	uiQueueMain(queuedCallback, &calls);
	uiUninit();

	assert_null(uiInit(&o));
	uiQueueMain(queuedCallbackAndQuit, &calls);
	uiMain();
	assert_int_equal(calls, 1);
	uiUninit();
}
#endif

#if !defined(_WIN32) && !defined(__APPLE__)
static void mainStepsResetAfterQuit(void **state)
{
	uiInitOptions o = {0};
	int i;

	assert_null(uiInit(&o));
	uiMainSteps();
	uiQuit();
	for (i = 0; i < 100; i++)
		if (!uiMainStep(0))
			break;
	assert_in_range(i, 0, 99);

	uiMainSteps();
	assert_true(uiMainStep(0));
	uiUninit();
}
#endif

#if !defined(__APPLE__)
struct timerState {
	int count;
	int stopAfter;
};

static int repeatTimer(void *data)
{
	struct timerState *state = data;

	state->count++;
	if (state->count >= state->stopAfter) {
		uiQuit();
		return 0;
	}
	return 1;
}

static void timerOneShot(void **state)
{
	uiInitOptions o = {0};
	struct timerState timerState = { 0, 1 };

	assert_null(uiInit(&o));
	uiTimer(1, repeatTimer, &timerState);
	uiMain();
	assert_int_equal(timerState.count, 1);
	uiUninit();
}

static void timerRepeatThenStop(void **state)
{
	uiInitOptions o = {0};
	struct timerState timerState = { 0, 3 };

	assert_null(uiInit(&o));
	uiTimer(1, repeatTimer, &timerState);
	uiMain();
	assert_int_equal(timerState.count, 3);
	uiUninit();
}
#endif

int initRunUnitTests(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(versionIsAvailableWithoutInitialization),
		cmocka_unit_test(initUninit),
		cmocka_unit_test(initUninitTwice),
#if !defined(_WIN32)
		cmocka_unit_test(queuedCallbacksDoNotSurviveUninit),
#endif
#if !defined(_WIN32) && !defined(__APPLE__)
		cmocka_unit_test(mainStepsResetAfterQuit),
#endif
#if !defined(__APPLE__)
		cmocka_unit_test(timerOneShot),
		cmocka_unit_test(timerRepeatThenStop),
#endif
	};

	return cmocka_run_group_tests_name("uiInit", tests, NULL, NULL);
}
