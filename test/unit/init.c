#include "unit.h"

#if !defined(_WIN32)
#include <pthread.h>
#endif

#if !defined(_WIN32) && !defined(__APPLE__)
#include <glib.h>
#endif

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
#endif

struct destroyAndQuitState {
	uiControl *control;
	int destroyed;
};

static void countDestroyedOnMainExit(uiControl *c, void *data)
{
	struct destroyAndQuitState *state = data;

	state->destroyed++;
}

static void destroyAndQuit(void *data)
{
	struct destroyAndQuitState *state = data;

	uiControlDestroy(state->control);
	uiQuit();
}

static void mainFlushesDeferredDestroysBeforeReturning(void **state)
{
	uiInitOptions o = {0};
	struct destroyAndQuitState destroyState = { NULL, 0 };

	assert_null(uiInit(&o));
	destroyState.control = uiControl(uiNewWindow("Deferred destroy", 100, 100, 0));
	uiControlOnDestroyed(destroyState.control, countDestroyedOnMainExit,
		&destroyState);
	uiQueueMain(destroyAndQuit, &destroyState);
	uiMain();
	assert_int_equal(destroyState.destroyed, 1);
	uiUninit();
}

static void mainStepsFlushDeferredDestroysBeforeReturning(void **state)
{
	uiInitOptions o = {0};
	struct destroyAndQuitState destroyState = { NULL, 0 };

	assert_null(uiInit(&o));
	destroyState.control = uiControl(uiNewWindow("Deferred destroy", 100, 100, 0));
	uiControlOnDestroyed(destroyState.control, countDestroyedOnMainExit,
		&destroyState);
	uiMainSteps();
	uiQueueMain(destroyAndQuit, &destroyState);
	while (uiMainStep(1))
		;
	assert_int_equal(destroyState.destroyed, 1);
	uiUninit();
}

#if !defined(_WIN32)
static void queuedCallbacksDoNotSurviveUninit(void **state)
{
	uiInitOptions o = {0};
	int calls = 0;
	int i;

	assert_null(uiInit(&o));
	for (i = 0; i < 1000; i++)
		uiQueueMain(queuedCallback, &calls);
	uiUninit();

	assert_null(uiInit(&o));
	uiQueueMain(queuedCallbackAndQuit, &calls);
	uiMain();
	assert_int_equal(calls, 1);
	uiUninit();
}

struct queueOrderState {
	int received[4];
	int count;
};

struct queueOrderItem {
	struct queueOrderState *state;
	int value;
	struct queueOrderItem *next;
};

static void recordQueuedOrder(void *data)
{
	struct queueOrderItem *item = data;
	struct queueOrderState *state = item->state;

	state->received[state->count++] = item->value;
	if (item->next != NULL)
		uiQueueMain(recordQueuedOrder, item->next);
	if (item->value == 3)
		uiQuit();
}

static void queuedCallbacksKeepOrderWhenReentered(void **state)
{
	uiInitOptions o = {0};
	struct queueOrderState order = {{0}, 0};
	struct queueOrderItem items[4] = {
		{&order, 0, NULL},
		{&order, 1, NULL},
		{&order, 2, NULL},
		{&order, 3, NULL},
	};
	int i;

	assert_null(uiInit(&o));
	items[0].next = &items[3];
	for (i = 0; i < 3; i++)
		uiQueueMain(recordQueuedOrder, &items[i]);
	uiMain();
	assert_int_equal(order.count, 4);
	for (i = 0; i < 4; i++)
		assert_int_equal(order.received[i], i);
	uiUninit();
}

struct queueProducer {
	int *calls;
	int count;
	int *finished;
};

static void queuedProducerFinished(void *data)
{
	int *finished = data;

	(*finished)++;
	if (*finished == 2)
		uiQuit();
}

static void *enqueueFromThread(void *data)
{
	struct queueProducer *producer = data;
	int i;

	for (i = 0; i < producer->count; i++)
		uiQueueMain(queuedCallback, producer->calls);
	uiQueueMain(queuedProducerFinished, producer->finished);
	return NULL;
}

static void queuedCallbacksFromMultipleThreads(void **state)
{
	uiInitOptions o = {0};
	struct queueProducer producer;
	pthread_t threads[2];
	int calls = 0;
	int finished = 0;
	int i;

	assert_null(uiInit(&o));
	producer.calls = &calls;
	producer.count = 1000;
	producer.finished = &finished;
	for (i = 0; i < 2; i++)
		assert_int_equal(pthread_create(&threads[i], NULL, enqueueFromThread, &producer), 0);
	uiMain();
	for (i = 0; i < 2; i++)
		assert_int_equal(pthread_join(threads[i], NULL), 0);
	assert_int_equal(finished, 2);
	assert_int_equal(calls, 2 * producer.count);
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

#if !defined(_WIN32) && !defined(__APPLE__)
struct timerYieldState {
	uiAreaHandler handler;
	uiArea *area;
	gint64 deadline;
	int running;
	int ticks;
	int draws;
	int queued;
};

static void timerYieldDraw(uiAreaHandler *handler, uiArea *area,
	uiAreaDrawParams *params)
{
	struct timerYieldState *state = (struct timerYieldState *) handler;

	if (state->running && state->ticks > 0)
		state->draws++;
}

static void timerYieldMouseEvent(uiAreaHandler *handler, uiArea *area,
	uiAreaMouseEvent *event)
{
}

static void timerYieldMouseCrossed(uiAreaHandler *handler, uiArea *area, int left)
{
}

static void timerYieldDragBroken(uiAreaHandler *handler, uiArea *area)
{
}

static int timerYieldKeyEvent(uiAreaHandler *handler, uiArea *area,
	uiAreaKeyEvent *event)
{
	return 0;
}

static void timerYieldQueued(void *data)
{
	struct timerYieldState *state = data;

	if (state->running)
		state->queued++;
}

static int timerYieldTick(void *data)
{
	struct timerYieldState *state = data;

	state->ticks++;
	if ((state->draws > 0 && state->queued > 0) ||
		g_get_monotonic_time() >= state->deadline) {
		state->running = 0;
		uiQuit();
		return 0;
	}
	uiAreaQueueRedrawAll(state->area);
	uiQueueMain(timerYieldQueued, state);
	// Keep this timer overdue without consuming CPU or assuming a frame rate.
	g_usleep(30000);
	return 1;
}

static void timerYieldsToRedraw(void **state)
{
	uiInitOptions o = {0};
	struct timerYieldState timerState = {0};
	uiWindow *window;

	assert_null(uiInit(&o));
	timerState.handler.Draw = timerYieldDraw;
	timerState.handler.MouseEvent = timerYieldMouseEvent;
	timerState.handler.MouseCrossed = timerYieldMouseCrossed;
	timerState.handler.DragBroken = timerYieldDragBroken;
	timerState.handler.KeyEvent = timerYieldKeyEvent;
	timerState.area = uiNewArea(&timerState.handler);
	window = uiNewWindow("Timer redraw fairness", 100, 100, 0);
	uiWindowSetChild(window, uiControl(timerState.area));
	uiControlShow(uiControl(window));
	timerState.running = 1;
	timerState.deadline = g_get_monotonic_time() + 2 * G_TIME_SPAN_SECOND;
	uiTimer(1, timerYieldTick, &timerState);
	uiMain();
	uiControlDestroy(uiControl(window));
	uiUninit();
	assert_true(timerState.ticks > 1);
	assert_true(timerState.draws > 0);
	assert_true(timerState.queued > 0);
}

static int countTimerUntilUninit(void *data)
{
	int *ticks = data;

	(*ticks)++;
	return 1;
}

static void timerAwaitingRearmDoesNotSurviveUninit(void **state)
{
	uiInitOptions o = {0};
	struct timerState stopState = { 0, 1 };
	int ticks = 0;

	assert_null(uiInit(&o));
	uiTimer(1, countTimerUntilUninit, &ticks);
	// A single step returns after the tick, before its new idle can rearm it.
	while (ticks == 0)
		uiMainStep(1);
	uiUninit();

	assert_null(uiInit(&o));
	uiTimer(20, repeatTimer, &stopState);
	uiMain();
	uiUninit();
	assert_int_equal(ticks, 1);
	assert_int_equal(stopState.count, 1);
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
		cmocka_unit_test(queuedCallbacksKeepOrderWhenReentered),
		cmocka_unit_test(queuedCallbacksFromMultipleThreads),
#endif
		cmocka_unit_test(mainFlushesDeferredDestroysBeforeReturning),
		cmocka_unit_test(mainStepsFlushDeferredDestroysBeforeReturning),
#if !defined(_WIN32) && !defined(__APPLE__)
		cmocka_unit_test(mainStepsResetAfterQuit),
#endif
#if !defined(__APPLE__)
		cmocka_unit_test(timerOneShot),
		cmocka_unit_test(timerRepeatThenStop),
#endif
#if !defined(_WIN32) && !defined(__APPLE__)
		cmocka_unit_test(timerYieldsToRedraw),
		cmocka_unit_test(timerAwaitingRearmDoesNotSurviveUninit),
#endif
	};

	return cmocka_run_group_tests_name("uiInit", tests, NULL, NULL);
}
